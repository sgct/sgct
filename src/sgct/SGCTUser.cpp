/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTUser.h>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace sgct_core {

/*!
    Default contructor
*/
SGCTUser::SGCTUser(std::string name) : mName(std::move(name)) {}

/*!
    Sets user's head position
*/
void SGCTUser::setPos(float x, float y, float z) {
    mPos[Frustum::MonoEye] = glm::vec3(x, y, z);
    updateEyeSeparation();
}

/*!
    Sets user's head position
*/
void SGCTUser::setPos(glm::vec3 pos) {
    mPos[Frustum::MonoEye] = std::move(pos);
    updateEyeSeparation();
}

/*!
    Sets user's head position
*/
void SGCTUser::setPos(glm::dvec4 pos) {
    mPos[Frustum::MonoEye] = glm::vec3(pos);
    updateEyeSeparation();
}

/*!
    Sets user's head position
    @param pos double array with 3 slots for x, y & z
*/
void SGCTUser::setPos(float* pos) {
    mPos[Frustum::MonoEye] = glm::vec3(pos[0], pos[1], pos[2]);
    updateEyeSeparation();
}

/*!
    Set if the user's head position & orientation should be managed by a VRPN tracking device.
    This is normally done using the XML configuration file.

    @param trackerName the pointer to the tracker
    @param deviceName the name of the device which is mapped to the tracker
*/
void SGCTUser::setHeadTracker(const char* trackerName, const char* deviceName) {
    mHeadTrackerDeviceName = deviceName;
    mHeadTrackerName = trackerName;
}

/*!
    Set the user's head transform matrix.
    @param transform the transform matrix
*/
void SGCTUser::setTransform(glm::dmat4 transform) {
    mTransform = glm::mat4(std::move(transform));
    updateEyeTransform();
}

/*!
    Set the user's head transform matrix.
    @param transform the transform matrix
*/
void SGCTUser::setTransform(glm::mat4 transform) {
    mTransform = std::move(transform);
    updateEyeTransform();
}

/*!
    Set the user's head orientation using euler angles.
    Note that rotations are dependent of each other, total rotation = xRot * yRot * zRot
    @param xRot the rotation around the x-azis
    @param yRot the rotation around the y-azis
    @param zRot the rotation around the z-azis
*/
void SGCTUser::setOrientation(float xRot, float yRot, float zRot) {
    //create rotation quaternion based on x, y, z rotations
    /*glm::quat rotQuat;
    rotQuat = glm::rotate( rotQuat, glm::radians(xRot), glm::vec3(1.0f, 0.0f, 0.0f) );
    rotQuat = glm::rotate( rotQuat, glm::radians(yRot), glm::vec3(0.0f, 1.0f, 0.0f) );
    rotQuat = glm::rotate( rotQuat, glm::radians(zRot), glm::vec3(0.0f, 0.0f, 1.0f) );*/

    //create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPos[Frustum::MonoEye]);
    
    //calculate transform
    //mTransform = transMat * glm::mat4_cast(rotQuat);

    mTransform = transMat *
        glm::eulerAngleX(xRot) * glm::eulerAngleY(yRot) * glm::eulerAngleZ(zRot);

    updateEyeTransform();
}

/*!
Set the user's head orientation using a quaternion
*/
void SGCTUser::setOrientation(glm::quat q) {
    //create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPos[Frustum::MonoEye]);

    mTransform = transMat * glm::mat4_cast(q);

    updateEyeTransform();
}

/*!
    Changes the interocular distance and recalculates the user's eye positions.
*/
void SGCTUser::setEyeSeparation(float eyeSeparation) {
    mEyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

/*!
    Recalculates the user's eye positions based on head position and eye separation
    (interocular distance).
*/
void SGCTUser::updateEyeSeparation() {
    glm::vec3 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f );
    mPos[Frustum::StereoLeftEye] = mPos[Frustum::MonoEye] - eyeOffsetVec;
    mPos[Frustum::StereoRightEye] = mPos[Frustum::MonoEye] + eyeOffsetVec;
}

/*!
    Recalculates the user's eye orientations based on head position and eye
    separation (interocular distance).
*/
void SGCTUser::updateEyeTransform() {
    glm::vec4 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f, 0.f );
    
    glm::vec4 pos[3];
    pos[Frustum::MonoEye] = glm::vec4(0.f, 0.f, 0.f, 1.f);
    pos[Frustum::StereoLeftEye] = pos[Frustum::MonoEye] - eyeOffsetVec;
    pos[Frustum::StereoRightEye] = pos[Frustum::MonoEye] + eyeOffsetVec;

    mPos[Frustum::MonoEye] = glm::vec3(mTransform * pos[Frustum::MonoEye]);
    mPos[Frustum::StereoLeftEye] = glm::vec3(mTransform * pos[Frustum::StereoLeftEye]);
    mPos[Frustum::StereoRightEye] = glm::vec3(mTransform * pos[Frustum::StereoRightEye]);
}

/*!
    Get the users position (eye position)
    @param fm which eye/projection
    @returns position vector
*/
const glm::vec3& SGCTUser::getPos(Frustum::FrustumMode fm) const {
    return mPos[fm];
}

float SGCTUser::getEyeSeparation() const {
    return mEyeSeparation;
}

float SGCTUser::getHalfEyeSeparation() const {
    return mEyeSeparation / 2.f;
}

float SGCTUser::getXPos() const {
    return mPos[Frustum::MonoEye].x;
}

float SGCTUser::getYPos() const {
    return mPos[Frustum::MonoEye].y;
}

float SGCTUser::getZPos() const {
    return mPos[Frustum::MonoEye].z;
}

const char* SGCTUser::getHeadTrackerName() const {
    return mHeadTrackerName.c_str();
}

const char* SGCTUser::getHeadTrackerDeviceName() const {
    return mHeadTrackerDeviceName.c_str();
}

/*!
Get the users name
@returns users name
*/
std::string SGCTUser::getName() const {
    return mName;
}

/*!
@returns true if user is tracked
*/
bool SGCTUser::isTracked() const {    
    return !(mHeadTrackerDeviceName.empty() || mHeadTrackerName.empty());
}

} // namespace sgct_core
