/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__OPENVR__H__
#define __SGCT__OPENVR__H__

#ifdef SGCT_HAS_OPENVR

#include <string>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sgct/Frustum.h>
#include <openvr.h>

class Window;

namespace sgct::openvr {
    /// Init OpenVR
    void initialize(float nearClip, float farClip);

    /// Shutdown OpenVR
    void shutdown();

    bool isHMDActive();

    void copyWindowToHMD(Window* win);

    glm::mat4 getHMDCurrentViewProjectionMatrix(sgct_core::Frustum::Mode nEye);

    // Updates pose matrices for all tracked OpenVR devices
    void updatePoses();

    /// Updates matrices for both eyes of tracked HMD.
    void updateHMDMatrices(float nearClip, float farClip);

    std::string getTrackedDeviceString(vr::IVRSystem* pHmd,
        vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
        vr::TrackedPropertyError* peError = nullptr);
    glm::mat4 getHMDEyeProjectionMatrix(vr::Hmd_Eye nEye, float nearClip, float farClip);
    glm::mat4 getHMDEyeToHeadTransform(vr::Hmd_Eye nEye);
    
    glm::mat4 getHMDPoseMatrix();
    glm::quat getInverseRotation(glm::mat4 poseMat);
} // namespace sgct::openvr

#endif // SGCT_HAS_OPENVR

#endif // __SGCT__OPENVR__H__
