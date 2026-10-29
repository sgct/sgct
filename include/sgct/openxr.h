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
#include <sgct/vulkan/context.h>
#include <glm/glm.hpp>

namespace sgct { class Window; }

namespace sgct::openxr {
    /**
     * Creates the OpenXR instance and finds the head-mounted display. As the OpenXR
     * runtime has to create the Vulkan instance and device, this has to be called before
     * the Vulkan context is initialized, which then has to use the
     * #vulkanCreationOverrides.
     *
     * \return `true` if the runtime and a head-mounted display are available
     */
    SGCT_EXPORT bool initializeInstance();

    /**
     * \return The functions with which the OpenXR runtime creates the Vulkan instance and
     *         device. Only valid after #initializeInstance succeeded
     */
    SGCT_EXPORT vulkan::Context::CreationOverrides vulkanCreationOverrides();

    /// Creates the OpenXR session for the Vulkan context, which has to be initialized
    /// with the #vulkanCreationOverrides
    SGCT_EXPORT void initialize(float nearClip, float farClip);

    /// Shutdown OpenXR and release runtime resources. Must be called before the Vulkan
    /// context is destroyed
    SGCT_EXPORT void shutdown();

    SGCT_EXPORT bool isHMDActive();

    SGCT_EXPORT ivec2 eyeResolution(FrustumMode eye);

    /**
     * Records the copy of the eyes of an SGCT window's render target into the swapchain
     * images of the OpenXR runtime. The commands are recorded into the command buffer of
     * the window's frame (Window::frameCommandBuffer), so this has to be called between
     * Window::renderComposite and Window::swapBuffers. The frame is handed to the
     * runtime by #submitFrame.
     */
    SGCT_EXPORT void copyWindowToHMD(Window* win);

    /**
     * Releases the swapchain images and submits the frame to the OpenXR runtime. Must be
     * called after the command buffer into which #copyWindowToHMD recorded the copies was
     * submitted (Window::swapBuffers).
     */
    SGCT_EXPORT void submitFrame();

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
