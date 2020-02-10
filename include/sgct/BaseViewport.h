/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _BASE_VIEWPORT_H
#define _BASE_VIEWPORT_H

#include <string>
#include "SGCTUser.h"
#include "SGCTProjection.h"

namespace sgct_core
{

/*!
    This class holds and manages viewportdata and calculates frustums
*/
class BaseViewport
{
public:
    BaseViewport();
    virtual ~BaseViewport() {;}

    void setName(const std::string & name);
    void setPos(float x, float y);
    void setSize(float x, float y);
    void setEnabled(bool state);
    void setUser(SGCTUser * user);
    void setUserName(std::string userName);
    void setEye(Frustum::FrustumMode eye);
    
    std::string getName();
    float getX();
    float getY();
    float getXSize();
    float getYSize();
    float getHorizontalFieldOfViewDegrees();
    
    inline SGCTUser * getUser() { return mUser; }
    inline Frustum::FrustumMode getEye() { return mEye; }
    inline SGCTProjection * getProjection(Frustum::FrustumMode frustumMode) { return &mProjections[frustumMode]; }
    inline SGCTProjection * getProjection() { return &mProjections[mEye]; }
    inline SGCTProjectionPlane * getProjectionPlane() { return &mProjectionPlane; }
    inline glm::quat getRotation() { return mRot; }
    inline glm::vec4 getFOV() { return mFOV; }
    inline float getDistance() { return mDistance; }
    
    bool isEnabled();
    void linkUserName();

    void calculateFrustum(const Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane);
    void calculateNonLinearFrustum(const Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right, glm::quat rot, float dist = 10.0f);
    void setViewPlaneCoordsFromUnTransformedCoords(glm::vec3 untransformedCoords[3], glm::quat rot);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);
    void setHorizontalFieldOfView(float horizFovDeg);

    
protected:
    SGCTProjection mProjections[3];
    SGCTProjectionPlane mProjectionPlane;
    Frustum::FrustumMode mEye;

    SGCTUser * mUser;
    std::string mName;
    std::string mUserName;
    bool mEnabled = true;
    float mX = 0.f;
    float mY = 0.f;
    float mXSize = 1.f;
    float mYSize = 1.f;

    glm::vec3 mUnTransformedViewPlaneCoords[3];
    glm::quat mRot = glm::quat(1.f, 0.f, 0.f, 0.f);
    float mDistance = 0.f;
    glm::vec4 mFOV = glm::vec4(0.f);
};

}

#endif
