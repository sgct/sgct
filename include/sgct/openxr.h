/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__OPENXR__H__
#define __SGCT__OPENXR__H__

#ifdef SGCT_HAS_OPENXR

#include <sgct/definitions.h>
#include <sgct/sgctexports.h>
#include <glm/glm.hpp>

namespace sgct { class Window; }

namespace sgct::openxr {
    /// Initialize OpenXR using the current OpenGL context.
    SGCT_EXPORT void initialize(float nearClip, float farClip);

    /// Shutdown OpenXR and release runtime resources.
    SGCT_EXPORT void shutdown();

    SGCT_EXPORT bool isHMDActive();

    SGCT_EXPORT ivec2 eyeResolution(FrustumMode eye);

    /// Submit a side-by-side stereo SGCT window render target to the OpenXR runtime.
    SGCT_EXPORT void copyWindowToHMD(Window* win);

    SGCT_EXPORT glm::mat4 currentViewProjectionMatrix(FrustumMode eye);

    /// Locate predicted headset and eye poses for the next frame.
    SGCT_EXPORT void updatePoses();

    /// Updates projection matrices for both eyes.
    SGCT_EXPORT void updateHMDMatrices(float nearClip, float farClip);

    SGCT_EXPORT glm::mat4 eyeProjectionMatrix(FrustumMode eye, float nearClip, float farClip);
    SGCT_EXPORT glm::mat4 eyeToHeadTransform(FrustumMode eye);

    SGCT_EXPORT glm::mat4 poseMatrix();
    SGCT_EXPORT glm::quat inverseRotation(glm::mat4 poseMat);
} // namespace sgct::openxr

#endif // SGCT_HAS_OPENXR
#endif // __SGCT__OPENXR__H__
