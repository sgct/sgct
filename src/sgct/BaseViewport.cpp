/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/BaseViewport.h>

#include <sgct/ClusterManager.h>
#include <sgct/SGCTUser.h>

namespace sgct_core {

BaseViewport::BaseViewport()
    : mUser(ClusterManager::instance()->getDefaultUser())
{}

void BaseViewport::setName(std::string name) {
    mName = std::move(name);
}

void BaseViewport::setPos(glm::vec2 position) {
    mPosition = std::move(position);
}

void BaseViewport::setSize(glm::vec2 size) {
    mSize = std::move(size);
}

void BaseViewport::setEnabled(bool state) {
    mEnabled = state;
}

bool BaseViewport::isEnabled() const {
    return mEnabled;
}

void BaseViewport::setEye(Frustum::FrustumMode eye) {
    mEye = eye;
}

const std::string& BaseViewport::getName() const {
    return mName;
}

const glm::vec2& BaseViewport::getPosition() const {
    return mPosition;
}

const glm::vec2& BaseViewport::getSize() const {
    return mSize;
}

void BaseViewport::setUser(SGCTUser& user) {
    mUser = user;
}

SGCTUser& BaseViewport::getUser() const {
    return mUser;
}

Frustum::FrustumMode BaseViewport::getEye() const {
    return mEye;
}

SGCTProjection& BaseViewport::getProjection(Frustum::FrustumMode frustumMode) {
    switch (frustumMode) {
        default:
        case Frustum::FrustumMode::MonoEye:
            return mProjections.mono;
        case Frustum::FrustumMode::StereoLeftEye:
            return mProjections.stereoLeft;
        case Frustum::FrustumMode::StereoRightEye:
            return mProjections.stereoRight;
    }
}

const SGCTProjection& BaseViewport::getProjection(Frustum::FrustumMode frustumMode) const
{
    switch (frustumMode) {
        default:
        case Frustum::FrustumMode::MonoEye:
            return mProjections.mono;
        case Frustum::FrustumMode::StereoLeftEye:
            return mProjections.stereoLeft;
        case Frustum::FrustumMode::StereoRightEye:
            return mProjections.stereoRight;
    }
}

SGCTProjection& BaseViewport::getProjection() {
    return getProjection(mEye);
}

SGCTProjectionPlane& BaseViewport::getProjectionPlane() {
    return mProjectionPlane;
}

glm::quat BaseViewport::getRotation() const {
    return mRot;
}

glm::vec4 BaseViewport::getFOV() const {
    return mFOV;
}

float BaseViewport::getDistance() const {
    return mDistance;
}

void BaseViewport::setUserName(std::string userName) {
    mUserName = std::move(userName);
    linkUserName();
}

void BaseViewport::linkUserName() {
    SGCTUser* user = ClusterManager::instance()->getUser(mUserName);
    if (user) {
        mUser = *user;
    }
}

void BaseViewport::calculateFrustum(Frustum::FrustumMode frustumMode,
                                    float nearClippingPlane,
                                    float farClippingPlane)
{
    switch (frustumMode) {
        case Frustum::FrustumMode::MonoEye:
            mProjections.mono.calculateProjection(
                mUser.getPosMono(),
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane
            );
            break;
        case Frustum::FrustumMode::StereoLeftEye:
            mProjections.stereoLeft.calculateProjection(
                mUser.getPosLeftEye(),
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane
            );
            break;
        case Frustum::FrustumMode::StereoRightEye:
            mProjections.stereoRight.calculateProjection(
                mUser.getPosRightEye(),
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane
            );
            break;
    }
}

void BaseViewport::calculateNonLinearFrustum(Frustum::FrustumMode frustumMode,
                                             float nearClippingPlane,
                                             float farClippingPlane)
{
    glm::vec3 eyePos = mUser.getPosMono();

    switch (frustumMode) {
        case Frustum::FrustumMode::MonoEye:
            mProjections.mono.calculateProjection(
                eyePos,
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane,
                mUser.getPosMono() - eyePos
            );
            break;
        case Frustum::FrustumMode::StereoLeftEye:
            mProjections.stereoLeft.calculateProjection(
                eyePos,
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane,
                mUser.getPosLeftEye() - eyePos
            );
            break;
        case Frustum::FrustumMode::StereoRightEye:
            mProjections.stereoRight.calculateProjection(
                eyePos,
                mProjectionPlane,
                nearClippingPlane,
                farClippingPlane,
                mUser.getPosRightEye() - eyePos
            );
            break;
    }
}

void BaseViewport::setViewPlaneCoordsUsingFOVs(float up, float down, float left,
                                               float right, glm::quat rot, float dist)
{
    mRot = rot;

    mFOV = glm::vec4(up, down, left, right);
    mDistance = dist;

    mUnTransformedViewPlaneCoords.lowerLeft.x = dist * tanf(glm::radians<float>(left));
    mUnTransformedViewPlaneCoords.lowerLeft.y = dist * tanf(glm::radians<float>(down));
    mUnTransformedViewPlaneCoords.lowerLeft.z = -dist;

    mUnTransformedViewPlaneCoords.upperLeft.x = dist * tanf(glm::radians<float>(left));
    mUnTransformedViewPlaneCoords.upperLeft.y = dist * tanf(glm::radians<float>(up));
    mUnTransformedViewPlaneCoords.upperLeft.z = -dist;

    mUnTransformedViewPlaneCoords.upperRight.x = dist * tanf(glm::radians<float>(right));
    mUnTransformedViewPlaneCoords.upperRight.y = dist * tanf(glm::radians<float>(up));
    mUnTransformedViewPlaneCoords.upperRight.z = -dist;

    setViewPlaneCoordsFromUnTransformedCoords(
        mUnTransformedViewPlaneCoords.lowerLeft,
        mUnTransformedViewPlaneCoords.upperLeft,
        mUnTransformedViewPlaneCoords.upperRight,
        rot
    );
}

void BaseViewport::setViewPlaneCoordsFromUnTransformedCoords(glm::vec3 lowerLeft,
                                                             glm::vec3 upperLeft,
                                                             glm::vec3 upperRight,
                                                             const glm::quat& rot)
{
    mProjectionPlane.setCoordinateLowerLeft(rot * std::move(lowerLeft));
    mProjectionPlane.setCoordinateUpperLeft(rot * std::move(upperLeft));
    mProjectionPlane.setCoordinateUpperRight(rot * std::move(upperRight));
}

void BaseViewport::updateFovToMatchAspectRatio(float oldRatio, float newRatio) {
    mUnTransformedViewPlaneCoords.lowerLeft.x *= newRatio / oldRatio;
    mUnTransformedViewPlaneCoords.upperLeft.x *= newRatio / oldRatio;
    mUnTransformedViewPlaneCoords.upperRight.x *= newRatio / oldRatio;
    setViewPlaneCoordsFromUnTransformedCoords(
        mUnTransformedViewPlaneCoords.lowerLeft,
        mUnTransformedViewPlaneCoords.upperLeft,
        mUnTransformedViewPlaneCoords.upperRight,
        mRot
    );
}

float BaseViewport::getHorizontalFieldOfViewDegrees() const {
    const float xDist = (mProjectionPlane.getCoordinateUpperRight().x -
        mProjectionPlane.getCoordinateUpperLeft().x) / 2;
    const float zDist = mProjectionPlane.getCoordinateUpperRight().z;
    return (glm::degrees(atan(abs(xDist / zDist)))) * 2;
}

void BaseViewport::setHorizontalFieldOfView(float horizFovDeg, float aspectRatio) {
    const float zDist = abs(mProjectionPlane.getCoordinateUpperRight().z);

    const float projDimX = zDist * tan(glm::radians<float>(horizFovDeg) / 2.f);
    const float projDimY = projDimX / aspectRatio;
    float vertAngle = glm::degrees(atan(projDimY / zDist));

    setViewPlaneCoordsUsingFOVs(
        vertAngle,
        -vertAngle,
        -horizFovDeg / 2.f,
         horizFovDeg / 2.f,
         mRot,
         zDist
    );
}

} // namespace sgct_core
