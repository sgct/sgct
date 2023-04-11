/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__OPENVR__H__
#define __SGCT__OPENVR__H__

#ifdef SGCT_HAS_OPENVR

#include <sgct/sgctexports.h>
#include <sgct/frustum.h>
#include <glm/glm.hpp>
#include <openvr.h>
#include <string>

class Window;

namespace sgct::openvr {
    /// Init OpenVR
    SGCT_EXPORT void initialize(float nearClip, float farClip);

    /// Shutdown OpenVR
    SGCT_EXPORT void shutdown();

    SGCT_EXPORT bool isHMDActive();

    SGCT_EXPORT void copyWindowToHMD(Window* win);

    SGCT_EXPORT glm::mat4 currentViewProjectionMatrix(sgct::Frustum::Mode nEye);

    // Updates pose matrices for all tracked OpenVR devices
    SGCT_EXPORT void updatePoses();

    /// Updates matrices for both eyes of tracked HMD.
    SGCT_EXPORT void updateHMDMatrices(float nearClip, float farClip);

    SGCT_EXPORT std::string trackedDeviceString(vr::IVRSystem* pHmd,
        vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
        vr::TrackedPropertyError* peError = nullptr);
    SGCT_EXPORT glm::mat4 eyeProjectionMatrix(vr::Hmd_Eye nEye, float nearClip, float farClip);
    SGCT_EXPORT glm::mat4 eyeToHeadTransform(vr::Hmd_Eye nEye);

    SGCT_EXPORT glm::mat4 poseMatrix();
    SGCT_EXPORT glm::quat inverseRotation(glm::mat4 poseMat);
} // namespace sgct::openvr

#endif // SGCT_HAS_OPENVR
#endif // __SGCT__OPENVR__H__
