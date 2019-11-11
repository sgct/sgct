/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/BaseViewport.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>

sgct_core::BaseViewport::BaseViewport()
{
    mX = 0.0f;
    mY = 0.0f;
    mXSize = 1.0f;
    mYSize = 1.0f;
    mEnabled = true;
    mEye = Frustum::MonoEye;
    mUser = ClusterManager::instance()->getDefaultUserPtr();
    mName.assign("NoName");
}

bool sgct_core::BaseViewport::isEnabled()
{
    return mEnabled;
}

/*!
Name this viewport
*/
void sgct_core::BaseViewport::setName(const std::string & name)
{
    mName = name;
}

void sgct_core::BaseViewport::setPos(float x, float y)
{
    mX = x;
    mY = y;
}

void sgct_core::BaseViewport::setSize(float x, float y)
{
    mXSize = x;
    mYSize = y;
}

void sgct_core::BaseViewport::setEnabled(bool state)
{
    mEnabled = state;
}

void sgct_core::BaseViewport::setEye(sgct_core::Frustum::FrustumMode eye)
{
    mEye = eye;
}

std::string sgct_core::BaseViewport::getName()
{
    return mName;
}

float sgct_core::BaseViewport::getX()
{
    return mX;
}

float sgct_core::BaseViewport::getY()
{
    return mY;
}

float sgct_core::BaseViewport::getXSize()
{
    return mXSize;
}

float sgct_core::BaseViewport::getYSize()
{
    return mYSize;
}

void sgct_core::BaseViewport::setUser(sgct_core::SGCTUser * user)
{
    mUser = user;
}

void sgct_core::BaseViewport::setUserName(std::string userName)
{
    mUserName.assign(userName);
    linkUserName();
}

void sgct_core::BaseViewport::linkUserName()
{
    SGCTUser * user = ClusterManager::instance()->getUserPtr(mUserName);
    if (user != NULL)
    {
        mUser = user;
    }
    //else
    //    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Viewport cannot find user '%s'!\n", mUserName.c_str());
}

void sgct_core::BaseViewport::calculateFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane)
{
    glm::vec3 eyePos = mUser->getPos(frustumMode);
    mProjections[frustumMode].calculateProjection(eyePos, &mProjectionPlane, near_clipping_plane, far_clipping_plane);
}

/*!
   Make projection symmetric relative to user
*/
void sgct_core::BaseViewport::calculateNonLinearFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane)
{
    glm::vec3 eyePos = mUser->getPos();
    glm::vec3 offset = mUser->getPos(frustumMode) - eyePos;
    mProjections[frustumMode].calculateProjection(eyePos, &mProjectionPlane, near_clipping_plane, far_clipping_plane, offset);
}

void sgct_core::BaseViewport::setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right, glm::quat rot, float dist)
{
    mRot = rot;

    mFOV = glm::vec4(up, down, left, right);
    mDistance = dist;

    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].x = dist * tanf(glm::radians<float>(left));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].y = dist * tanf(glm::radians<float>(down));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].z = -dist;

    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperLeft].x = dist * tanf(glm::radians<float>(left));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperLeft].y = dist * tanf(glm::radians<float>(up));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperLeft].z = -dist;

    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperRight].x = dist * tanf(glm::radians<float>(right));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperRight].y = dist * tanf(glm::radians<float>(up));
    mUnTransformedViewPlaneCoords[SGCTProjectionPlane::UpperRight].z = -dist;

    setViewPlaneCoordsFromUnTransformedCoords(mUnTransformedViewPlaneCoords, rot);
}

void sgct_core::BaseViewport::setViewPlaneCoordsFromUnTransformedCoords(glm::vec3 untransformedCoords[3], glm::quat rot)
{
    for (std::size_t i = 0; i < 3; i++)
        mProjectionPlane.setCoordinate(i, rot * untransformedCoords[i]);
}

void sgct_core::BaseViewport::updateFovToMatchAspectRatio(float oldRatio, float newRatio)
{
    for (unsigned int cornerNum = (unsigned int)SGCTProjectionPlane::LowerLeft;
         cornerNum <= (unsigned int)SGCTProjectionPlane::UpperRight;
         ++cornerNum)
    {
        mUnTransformedViewPlaneCoords[cornerNum].x *= newRatio / oldRatio;
    }
    setViewPlaneCoordsFromUnTransformedCoords(mUnTransformedViewPlaneCoords, mRot);
}

float sgct_core::BaseViewport::getHorizontalFieldOfViewDegrees()
{
    float xDist = (mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperRight).x -
        mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperLeft).x) / 2;
    float zDist = mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperRight).z;
    return (glm::degrees(atanf(fabs(xDist / zDist)))) * 2;
}

void sgct_core::BaseViewport::setHorizontalFieldOfView(float horizFovDeg)
{
    const float tanLeftAngle =
        mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperLeft).x /
        -mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperLeft).z;

    const float tanRightAngle =
        mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperRight).x /
        -mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperRight).z;

    const float tanUpAngle =
        mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperLeft).y /
        -mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperLeft).z;

    const float tanDownAngle =
        mProjectionPlane.getCoordinate(SGCTProjectionPlane::LowerLeft).y /
        -mProjectionPlane.getCoordinate(SGCTProjectionPlane::LowerLeft).z;

    const float currentHorizontalFieldOfView = getHorizontalFieldOfViewDegrees();
    const float ratio = horizFovDeg / currentHorizontalFieldOfView;

    float zDist = mProjectionPlane.getCoordinate(SGCTProjectionPlane::UpperRight).z;

    setViewPlaneCoordsUsingFOVs(
        glm::degrees(atanf(ratio * tanUpAngle)),
        glm::degrees(atanf(ratio * tanDownAngle)),
        glm::degrees(atanf(ratio * tanLeftAngle)),
        glm::degrees(atanf(ratio * tanRightAngle)),
        mRot,
        fabs(zDist)
    );
}
