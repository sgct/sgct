/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_OPENXR

#include <sgct/openxr.h>

#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/window.h>

#define XR_USE_GRAPHICS_API_OPENGL
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
#define XR_USE_GRAPHICS_API_VULKAN
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>
#include <Unknwn.h>
#endif // WIN32
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
#include <vulkan/vulkan.h>
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#ifdef WIN32
#include <glad/glad_wgl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif // WIN32

namespace {
    using namespace sgct;

    constexpr uint32_t EyeCount = 2;

    enum class GraphicsBackend {
        OpenGL,
        Vulkan
    };

    struct Swapchain {
        XrSwapchain handle = XR_NULL_HANDLE;
        int32_t width = 0;
        int32_t height = 0;
        GLuint framebuffer = 0;
        std::vector<XrSwapchainImageOpenGLKHR> images;
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
        std::vector<XrSwapchainImageVulkanKHR> vulkanImages;
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
    };

    bool isOpenXRInitialized = false;
    bool isSessionRunning = false;
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace appSpace = XR_NULL_HANDLE;
    XrTime predictedDisplayTime = 0;
    bool isFrameBegun = false;
    bool shouldSubmitFrame = false;
    bool areViewsValid = false;
    GraphicsBackend graphicsBackend = GraphicsBackend::OpenGL;

    std::array<XrViewConfigurationView, EyeCount> configurationViews;
    std::array<XrView, EyeCount> views;
    std::array<XrCompositionLayerProjectionView, EyeCount> projectionViews;
    std::array<Swapchain, EyeCount> swapchains;
    std::array<glm::mat4, EyeCount> eyeProjectionMatrices = {
        glm::mat4(1.f),
        glm::mat4(1.f)
    };
    std::array<glm::mat4, EyeCount> eyeToHeadMatrices = {
        glm::mat4(1.f),
        glm::mat4(1.f)
    };

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    struct VulkanState {
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        VkDeviceSize stagingSize = 0;
    };

    VulkanState vulkan;
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    glm::mat4 poseHMDMat = glm::mat4(1.f);
    float hmdNearClip = 0.1f;
    float hmdFarClip = 100.f;

    bool succeeded(XrResult result, std::string_view action) {
        if (XR_SUCCEEDED(result)) {
            return true;
        }

        char buffer[XR_MAX_RESULT_STRING_SIZE] = {};
        if (instance != XR_NULL_HANDLE) {
            xrResultToString(instance, result, buffer);
        }
        Log::Error(std::format("OpenXR error: {}. {}", action, buffer));
        return false;
    }

    void resetFrameState() {
        predictedDisplayTime = 0;
        isFrameBegun = false;
        shouldSubmitFrame = false;
        areViewsValid = false;
    }

    void endFrame(const XrCompositionLayerBaseHeader** layers, uint32_t layerCount) {
        if (!isFrameBegun) {
            return;
        }

        const XrFrameEndInfo endInfo = {
            .type = XR_TYPE_FRAME_END_INFO,
            .displayTime = predictedDisplayTime,
            .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
            .layerCount = layerCount,
            .layers = layers
        };
        succeeded(xrEndFrame(session, &endInfo), "End frame");
        resetFrameState();
    }

    uint32_t eyeIndex(FrustumMode eye) {
        return eye == FrustumMode::StereoRight ? 1 : 0;
    }

    glm::mat4 projectionMatrix(const XrFovf& fov, float nearClip, float farClip) {
        const float tanLeft = std::tan(fov.angleLeft);
        const float tanRight = std::tan(fov.angleRight);
        const float tanDown = std::tan(fov.angleDown);
        const float tanUp = std::tan(fov.angleUp);

        const float width = tanRight - tanLeft;
        const float height = tanUp - tanDown;
        glm::mat4 result(0.f);
        result[0][0] = 2.f / width;
        result[1][1] = 2.f / height;
        result[2][0] = (tanRight + tanLeft) / width;
        result[2][1] = (tanUp + tanDown) / height;
        result[2][2] = -(farClip + nearClip) / (farClip - nearClip);
        result[2][3] = -1.f;
        result[3][2] = -(2.f * farClip * nearClip) / (farClip - nearClip);
        return result;
    }

    glm::mat4 transformMatrix(const XrPosef& pose) {
        const glm::quat orientation(
            pose.orientation.w,
            pose.orientation.x,
            pose.orientation.y,
            pose.orientation.z
        );
        const glm::vec3 position(
            pose.position.x,
            pose.position.y,
            pose.position.z
        );
        return glm::translate(glm::mat4(1.f), position) * glm::mat4_cast(orientation);
    }

    bool hasExtension(std::string_view extension) {
        uint32_t n = 0;

        XrResult res = xrEnumerateInstanceExtensionProperties(nullptr, 0, &n, nullptr);
        if (!succeeded(res, "Enumerate extensions")) {
            return false;
        }

        std::vector<XrExtensionProperties> exts(n, { XR_TYPE_EXTENSION_PROPERTIES });
        res = xrEnumerateInstanceExtensionProperties(nullptr, n, &n, exts.data());
        if (!succeeded(res, "Enumerate extensions")) {
            return false;
        }

        return std::any_of(
            exts.begin(),
            exts.end(),
            [extension](const XrExtensionProperties& e) {
                return extension == e.extensionName;
            }
        );
    }

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    bool succeeded(VkResult result, std::string_view action) {
        if (result == VK_SUCCESS) {
            return true;
        }
        Log::Error(std::format("Vulkan {} failed: {}", action, static_cast<int>(result)));
        return false;
    }

    template <typename T>
    bool loadOpenXRFunction(const char* name, T& function) {
        PFN_xrVoidFunction proc = nullptr;
        const XrResult res = xrGetInstanceProcAddr(instance, name, &proc);
        const bool success = succeeded(res, std::format("Get {} function", name));
        if (success) {
            function = reinterpret_cast<T>(proc);
        }
        return success;
    }

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memoryProps = {};
        vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memoryProps);
        for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
            if ((typeBits & (1 << i)) != 0 &&
                (memoryProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        return std::numeric_limits<uint32_t>::max();
    }

    void destroyVulkanStagingBuffer() {
        if (vulkan.stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vulkan.device, vulkan.stagingBuffer, nullptr);
            vulkan.stagingBuffer = VK_NULL_HANDLE;
        }
        if (vulkan.stagingMemory != VK_NULL_HANDLE) {
            vkFreeMemory(vulkan.device, vulkan.stagingMemory, nullptr);
            vulkan.stagingMemory = VK_NULL_HANDLE;
        }
        vulkan.stagingSize = 0;
    }

    void destroyVulkanBackend() {
        if (vulkan.device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(vulkan.device);
            destroyVulkanStagingBuffer();
            if (vulkan.commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(vulkan.device, vulkan.commandPool, nullptr);
                vulkan.commandPool = VK_NULL_HANDLE;
            }
            vkDestroyDevice(vulkan.device, nullptr);
            vulkan.device = VK_NULL_HANDLE;
        }
        if (vulkan.instance != VK_NULL_HANDLE) {
            vkDestroyInstance(vulkan.instance, nullptr);
            vulkan.instance = VK_NULL_HANDLE;
        }
        vulkan.physicalDevice = VK_NULL_HANDLE;
        vulkan.queue = VK_NULL_HANDLE;
        vulkan.queueFamilyIndex = 0;
    }

    void cleanupFailedVulkanStagingBuffer() {
        if (vulkan.stagingBuffer == VK_NULL_HANDLE ||
            vulkan.stagingMemory == VK_NULL_HANDLE)
        {
            destroyVulkanStagingBuffer();
        }
    }

    bool createVulkanStagingBuffer(VkDeviceSize size) {
        if (vulkan.stagingBuffer != VK_NULL_HANDLE && vulkan.stagingSize >= size) {
            return true;
        }

        destroyVulkanStagingBuffer();

        const VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };
        VkResult res = vkCreateBuffer(
            vulkan.device,
            &bufferInfo,
            nullptr,
            &vulkan.stagingBuffer
        );
        if (!succeeded(res, "Create staging buffer")) {
            return false;
        }

        VkMemoryRequirements requirements = {};
        vkGetBufferMemoryRequirements(vulkan.device, vulkan.stagingBuffer, &requirements);
        const uint32_t memoryTypeIndex = findMemoryType(
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        if (memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
            Log::Error("Vulkan could not find host-visible memory for OpenXR staging");
            cleanupFailedVulkanStagingBuffer();
            return false;
        }

        const VkMemoryAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };
        res = vkAllocateMemory(
            vulkan.device,
            &allocateInfo,
            nullptr,
            &vulkan.stagingMemory
        );
        if (!succeeded(res, "Allocate staging memory")) {
            cleanupFailedVulkanStagingBuffer();
            return false;
        }

        res = vkBindBufferMemory(
            vulkan.device,
            vulkan.stagingBuffer,
            vulkan.stagingMemory,
            0
        );
        if (!succeeded(res, "Bind staging memory")) {
            cleanupFailedVulkanStagingBuffer();
            return false;
        }

        vulkan.stagingSize = size;
        return true;
    }

    bool submitVulkanImageUpload(VkImage image, int32_t width, int32_t height) {
        const VkCommandBufferAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vulkan.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkResult res = vkAllocateCommandBuffers(
            vulkan.device,
            &allocateInfo,
            &commandBuffer
        );
        if (!succeeded(res, "Allocate upload command buffer")) {
            return false;
        }

        const VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (!succeeded(res, "Begin upload command buffer")) {
            vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, 1, &commandBuffer);
            return false;
        }

        const VkImageMemoryBarrier toTransfer = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange  {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toTransfer
        );

        const VkBufferImageCopy copyRegion = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
                1
            }
        };
        vkCmdCopyBufferToImage(
            commandBuffer,
            vulkan.stagingBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion
        );

        const VkImageMemoryBarrier toShaderRead = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = toTransfer.subresourceRange
        };
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toShaderRead
        );

        res = vkEndCommandBuffer(commandBuffer);
        if (!succeeded(res, "End upload command buffer")) {
            vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, 1, &commandBuffer);
            return false;
        }

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer
        };
        res = vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
        VkResult res2 = vkQueueWaitIdle(vulkan.queue);
        const bool result = succeeded(res, "Submit upload command buffer") &&
            succeeded(res2, "Wait for upload queue");
        vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, 1, &commandBuffer);
        return result;
    }

    bool copyOpenGLFramebufferToVulkanImage(const Window& window, uint32_t index,
                                            VkImage image, int32_t width, int32_t height)
    {
        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
        if (!createVulkanStagingBuffer(imageSize)) {
            return false;
        }

        GLuint readFramebuffer = 0;
        GLuint resolveFramebuffer = 0;
        GLuint resolveTexture = 0;
        glGenFramebuffers(1, &readFramebuffer);
        glGenFramebuffers(1, &resolveFramebuffer);
        glGenTextures(1, &resolveTexture);

        glBindTexture(GL_TEXTURE_2D, resolveTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            resolveTexture,
            0
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
        const ivec2 dim = window.framebufferResolution();
        const auto attachSourceTexture = [](GLuint texture) {
            glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                texture,
                0
            );
        };
        const auto blitSourceRect = [width, height](int x, int y, int srcWidth,
                                                    int srcHeight)
        {
            glBlitFramebuffer(
                x,
                y,
                x + srcWidth,
                y + srcHeight,
                0,
                height,
                width,
                0,
                GL_COLOR_BUFFER_BIT,
                GL_LINEAR
            );
        };

        const FrustumMode eye = index == 0 ?
            FrustumMode::StereoLeft :
            FrustumMode::StereoRight;
        bool copiedViewport = false;
        attachSourceTexture(window.frameBufferTextureEye(Eye::MonoOrLeft));
        for (const std::unique_ptr<Viewport>& viewport : window.viewports()) {
            if (!viewport->isEnabled() || viewport->eye() != eye) {
                continue;
            }

            const vec2& position = viewport->position();
            const vec2& size = viewport->size();
            blitSourceRect(
                static_cast<int>(position.x * dim.x),
                static_cast<int>(position.y * dim.y),
                static_cast<int>(size.x * dim.x),
                static_cast<int>(size.y * dim.y)
            );
            copiedViewport = true;
        }

        if (!copiedViewport &&
            (window.stereoMode() == Window::StereoMode::SideBySide ||
             window.stereoMode() == Window::StereoMode::SideBySideInverted))
        {
            for (const std::unique_ptr<Viewport>& viewport : window.viewports()) {
                if (!viewport->isEnabled() || viewport->eye() != FrustumMode::Mono)
                {
                    continue;
                }

                const vec2& position = viewport->position();
                const vec2& size = viewport->size();
                const int srcX = static_cast<int>(position.x * dim.x);
                const int srcY = static_cast<int>(position.y * dim.y);
                const int srcWidth = static_cast<int>(size.x * dim.x);
                const int srcHeight = static_cast<int>(size.y * dim.y);
                const int eyeWidth = srcWidth / 2;
                const bool useLeftHalf =
                    (index == 0) ==
                    (window.stereoMode() == Window::StereoMode::SideBySide);
                const int s = srcX + (useLeftHalf ? 0 : eyeWidth);
                blitSourceRect(s, srcY, eyeWidth, srcHeight);
                copiedViewport = true;
            }
        }

        if (!copiedViewport) {
            attachSourceTexture(window.frameBufferTextureEye(
                index == 0 ? Eye::MonoOrLeft : Eye::Right
            ));
            blitSourceRect(0, 0, dim.x, dim.y);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFramebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        void* data = nullptr;
        bool copied = false;
        VkResult res = vkMapMemory(
            vulkan.device,
            vulkan.stagingMemory,
            0,
            imageSize,
            0,
            &data
        );
        if (succeeded(res, "Map staging memory")) {
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
            vkUnmapMemory(vulkan.device, vulkan.stagingMemory);
            copied = submitVulkanImageUpload(image, width, height);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDeleteTextures(1, &resolveTexture);
        glDeleteFramebuffers(1, &resolveFramebuffer);
        glDeleteFramebuffers(1, &readFramebuffer);
        return copied;
    }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    void destroySwapchains() {
        for (Swapchain& swapchain : swapchains) {
            if (swapchain.framebuffer != 0) {
                glDeleteFramebuffers(1, &swapchain.framebuffer);
                swapchain.framebuffer = 0;
            }
            if (swapchain.handle != XR_NULL_HANDLE) {
                xrDestroySwapchain(swapchain.handle);
                swapchain.handle = XR_NULL_HANDLE;
            }
            swapchain.images.clear();
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
            swapchain.vulkanImages.clear();
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
        }
    }

    bool createInstance(const char* extensionName) {
        if (!hasExtension(extensionName)) {
            Log::Error(std::format("OpenXR runtime does not support {}", extensionName));
            return false;
        }

        const char* extensions[] = { extensionName };
        XrInstanceCreateInfo createInfo = {
            .type = XR_TYPE_INSTANCE_CREATE_INFO,
            .applicationInfo = {
                .applicationVersion = 1,
                .engineVersion = 1,
                .apiVersion = XR_MAKE_VERSION(1, 0, 0)
            },
            .enabledExtensionCount = static_cast<uint32_t>(std::size(extensions)),
            .enabledExtensionNames = extensions
        };
        std::strncpy(
            createInfo.applicationInfo.applicationName,
            "SGCT",
            XR_MAX_APPLICATION_NAME_SIZE - 1
        );
        std::strncpy(
            createInfo.applicationInfo.engineName,
            "SGCT",
            XR_MAX_ENGINE_NAME_SIZE - 1
        );

        const XrResult res = xrCreateInstance(&createInfo, &instance);
        return succeeded(res, "Create instance");
    }

    void destroyInstanceOnly() {
        if (instance != XR_NULL_HANDLE) {
            xrDestroyInstance(instance);
            instance = XR_NULL_HANDLE;
        }
        systemId = XR_NULL_SYSTEM_ID;
    }

    bool createSystem() {
        XrSystemGetInfo systemInfo = {
            .type = XR_TYPE_SYSTEM_GET_INFO,
            .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
        };
        const XrResult res = xrGetSystem(instance, &systemInfo, &systemId);
        if (!succeeded(res, "Get HMD system")) {
            return false;
        }

        XrSystemProperties properties = {
            .type = XR_TYPE_SYSTEM_PROPERTIES
        };
        XrResult res2 = xrGetSystemProperties(instance, systemId, &properties);
        if (succeeded(res2, "Get system properties")) {
            Log::Info(std::format("OpenXR system: {}", properties.systemName));
        }
        return true;
    }

    bool createOpenGLSession() {
#ifdef WIN32
        XrGraphicsRequirementsOpenGLKHR requirements = {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR
        };
        PFN_xrGetOpenGLGraphicsRequirementsKHR getOpenGLGraphicsRequirements = nullptr;
        XrResult res = xrGetInstanceProcAddr(
            instance,
            "xrGetOpenGLGraphicsRequirementsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&getOpenGLGraphicsRequirements)
        );
        if (!succeeded(res, "Get OpenGL requirements function")) {
            return false;
        }

        res = getOpenGLGraphicsRequirements(instance, systemId, &requirements);
        if (!succeeded(res, "Get OpenGL requirements")) {
            return false;
        }

        GLFWwindow* glfwWindow = glfwGetCurrentContext();
        if (!glfwWindow) {
            Log::Error("OpenXR requires a current GLFW OpenGL context");
            return false;
        }

        const XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {
            .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
            .hDC = GetDC(glfwGetWin32Window(glfwWindow)),
            .hGLRC = glfwGetWGLContext(glfwWindow)
        };

        const XrSessionCreateInfo createInfo = {
            .type = XR_TYPE_SESSION_CREATE_INFO,
            .next = &graphicsBinding,
            .systemId = systemId
        };
        res = xrCreateSession(instance, &createInfo, &session);
        return succeeded(res, "Create session");
#else // ^^^^ WIN32 // !WIN32 vvvv
        Log::Error("OpenXR OpenGL support is currently implemented for Windows only");
        return false;
#endif // WIN32
    }

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    bool createVulkanBackend() {
        PFN_xrGetVulkanGraphicsRequirements2KHR getRequirements = nullptr;
        PFN_xrCreateVulkanInstanceKHR createInstanceKHR = nullptr;
        PFN_xrGetVulkanGraphicsDevice2KHR getGraphicsDevice = nullptr;
        PFN_xrCreateVulkanDeviceKHR createDeviceKHR = nullptr;
        if (!loadOpenXRFunction("xrGetVulkanGraphicsRequirements2KHR", getRequirements) ||
            !loadOpenXRFunction("xrCreateVulkanInstanceKHR", createInstanceKHR) ||
            !loadOpenXRFunction("xrGetVulkanGraphicsDevice2KHR", getGraphicsDevice) ||
            !loadOpenXRFunction("xrCreateVulkanDeviceKHR", createDeviceKHR))
        {
            return false;
        }

        XrGraphicsRequirementsVulkan2KHR requirements = {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR
        };
        XrResult res = getRequirements(instance, systemId, &requirements);
        if (!succeeded(res, "Get Vulkan requirements")) {
            return false;
        }

        VkApplicationInfo applicationInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "SGCT",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "SGCT",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
        };

        VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &applicationInfo
        };

        XrVulkanInstanceCreateInfoKHR xrInstanceInfo = {
            .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
            .systemId = systemId,
            .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vulkanCreateInfo = &instanceInfo
        };

        VkResult vkResult = VK_SUCCESS;
        createInstanceKHR(instance, &xrInstanceInfo, &vulkan.instance, &vkResult);
        if (!succeeded(vkResult, "Create Vulkan instance through OpenXR") ||
            !succeeded(vkResult, "Create Vulkan instance"))
        {
            return false;
        }

        XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo = {
            .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
            .systemId = systemId,
            .vulkanInstance = vulkan.instance
        };
        res = getGraphicsDevice(instance, &deviceGetInfo, &vulkan.physicalDevice);
        if (!succeeded(res, "Get Vulkan graphics device")) {
            return false;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            vulkan.physicalDevice,
            &queueFamilyCount,
            nullptr
        );
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            vulkan.physicalDevice,
            &queueFamilyCount,
            queueFamilies.data()
        );
        const auto queueIt = std::find_if(
            queueFamilies.begin(),
            queueFamilies.end(),
            [](const VkQueueFamilyProperties& family) {
                return (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            }
        );
        if (queueIt == queueFamilies.end()) {
            Log::Error("Vulkan OpenXR fallback could not find a graphics queue family");
            return false;
        }
        vulkan.queueFamilyIndex = static_cast<uint32_t>(
            std::distance(queueFamilies.begin(), queueIt)
        );

        constexpr float QueuePriority = 1.f;
        const VkDeviceQueueCreateInfo queueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vulkan.queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &QueuePriority
        };

        const VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueInfo
        };

        const XrVulkanDeviceCreateInfoKHR xrDeviceInfo = {
            .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
            .systemId = systemId,
            .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vulkanPhysicalDevice = vulkan.physicalDevice,
            .vulkanCreateInfo = &deviceInfo
        };

        XrResult r = createDeviceKHR(instance, &xrDeviceInfo, &vulkan.device, &vkResult);
        if (!succeeded(r, "Create Vulkan device through OpenXR") ||
            !succeeded(vkResult, "Create Vulkan device"))
        {
            return false;
        }
        vkGetDeviceQueue(vulkan.device, vulkan.queueFamilyIndex, 0, &vulkan.queue);

        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vulkan.queueFamilyIndex
        };
        vkResult = vkCreateCommandPool(
            vulkan.device,
            &poolInfo,
            nullptr,
            &vulkan.commandPool
        );
        return succeeded(res,"Create command pool");
    }

    bool createVulkanSession() {
        if (!createVulkanBackend()) {
            return false;
        }

        const XrGraphicsBindingVulkan2KHR graphicsBinding = {
            .type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR,
            .instance = vulkan.instance,
            .physicalDevice = vulkan.physicalDevice,
            .device = vulkan.device,
            .queueFamilyIndex = vulkan.queueFamilyIndex,
            .queueIndex = 0
        };

        const XrSessionCreateInfo createInfo = {
            .type = XR_TYPE_SESSION_CREATE_INFO,
            .next = &graphicsBinding,
            .systemId = systemId
        };
        XrResult res = xrCreateSession(instance, &createInfo, &session);
        return succeeded(res, "Create Vulkan session");
    }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    bool initializeBackend(GraphicsBackend backend) {
        graphicsBackend = backend;
        const char* extensionName = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
        if (backend == GraphicsBackend::Vulkan) {
            extensionName = XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME;
        }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

        if (!createInstance(extensionName) || !createSystem()) {
            destroyInstanceOnly();
            return false;
        }

        bool sessionCreated = false;
        if (backend == GraphicsBackend::OpenGL) {
            sessionCreated = createOpenGLSession();
        }
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
        else {
            sessionCreated = createVulkanSession();
        }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

        if (!sessionCreated) {
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
            if (backend == GraphicsBackend::Vulkan) {
                destroyVulkanBackend();
            }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
            if (session != XR_NULL_HANDLE) {
                xrDestroySession(session);
                session = XR_NULL_HANDLE;
            }
            destroyInstanceOnly();
            return false;
        }

        return true;
    }

    bool createSpace() {
        const XrReferenceSpaceCreateInfo createInfo = {
            .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
            .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
            .poseInReferenceSpace = {
                .orientation = {
                    .w = 1.f
                }
            }
        };
        XrResult res = xrCreateReferenceSpace(session, &createInfo, &appSpace);
        return succeeded(res, "Create local space");
    }

    bool waitForRunningSession() {
        XrEventDataBuffer event = {
            .type = XR_TYPE_EVENT_DATA_BUFFER
        };
        while (xrPollEvent(instance, &event) == XR_SUCCESS) {
            if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                event = {
                    .type = XR_TYPE_EVENT_DATA_BUFFER
                };
                continue;
            }

            const XrEventDataSessionStateChanged& state =
                *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
            if (state.state == XR_SESSION_STATE_READY) {
                const XrSessionBeginInfo beginInfo = {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .primaryViewConfigurationType =
                        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
                };
                XrResult res = xrBeginSession(session, &beginInfo);
                if (!succeeded(res, "Begin session")) {
                    return false;
                }
                isSessionRunning = true;
            }
            else if (state.state == XR_SESSION_STATE_STOPPING) {
                xrEndSession(session);
                isSessionRunning = false;
            }
            event = {
                .type = XR_TYPE_EVENT_DATA_BUFFER
            };
        }
        return true;
    }

    bool enumerateViewConfiguration() {
        uint32_t viewCount = 0;
        XrResult res = xrEnumerateViewConfigurationViews(
            instance,
            systemId,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            0,
            &viewCount,
            nullptr
        );
        if (!succeeded(res, "Enumerate view configuration count")) {
            return false;
        }
        if (viewCount != EyeCount) {
            Log::Error(std::format(
                "OpenXR expected {} stereo views, runtime reported {}",
                EyeCount, viewCount
            ));
            return false;
        }

        configurationViews = {
            XrViewConfigurationView{ .type = XR_TYPE_VIEW_CONFIGURATION_VIEW },
            XrViewConfigurationView{ .type = XR_TYPE_VIEW_CONFIGURATION_VIEW }
        };
        res = xrEnumerateViewConfigurationViews(
            instance,
            systemId,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            EyeCount,
            &viewCount,
            configurationViews.data()
        );
        return succeeded(res, "Enumerate view configuration");
    }

    int64_t chooseSwapchainFormat() {
        uint32_t formatCount = 0;
        XrResult res = xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
        if (!succeeded(res, "Enumerate swapchain format count")) {
            return 0;
        }

        std::vector<int64_t> formats(formatCount);
        res = xrEnumerateSwapchainFormats(
            session,
            formatCount,
            &formatCount,
            formats.data()
        );
        if (!succeeded(res, "Enumerate swapchain formats")) {
            return 0;
        }

        if (graphicsBackend == GraphicsBackend::OpenGL) {
            constexpr std::array<int64_t, 4> PreferredFormats = {
                GL_SRGB8_ALPHA8,
                GL_RGBA8,
                GL_RGBA16F,
                GL_RGBA16
            };
            for (int64_t preferredFormat : PreferredFormats) {
                auto it = std::find(formats.begin(), formats.end(), preferredFormat);
                if (it != formats.end()) {
                    return preferredFormat;
                }
            }
        }
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
        else {
            constexpr std::array<int64_t, 4> PreferredFormats = {
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_FORMAT_B8G8R8A8_UNORM
            };
            for (int64_t preferredFormat : PreferredFormats) {
                auto it = std::find(formats.begin(), formats.end(), preferredFormat);
                if (it != formats.end()) {
                    return preferredFormat;
                }
            }
        }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
        return formats.empty() ? 0 : formats.front();
    }

    bool createSwapchains() {
        if (swapchains[0].handle != XR_NULL_HANDLE) {
            return true;
        }

        const int64_t format = chooseSwapchainFormat();
        if (format == 0) {
            Log::Error("OpenXR runtime did not report a usable swapchain format");
            return false;
        }

        for (uint32_t i = 0; i < EyeCount; ++i) {
            const XrViewConfigurationView& view = configurationViews[i];
            Swapchain& swapchain = swapchains[i];
            swapchain.width = static_cast<int32_t>(view.recommendedImageRectWidth);
            swapchain.height = static_cast<int32_t>(view.recommendedImageRectHeight);

            const XrSwapchainCreateInfo createInfo = {
                .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                    XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT,
                .format = format,
                .sampleCount = view.recommendedSwapchainSampleCount,
                .width = view.recommendedImageRectWidth,
                .height = view.recommendedImageRectHeight,
                .faceCount = 1,
                .arraySize = 1,
                .mipCount = 1
            };
            XrResult res = xrCreateSwapchain(session, &createInfo, &swapchain.handle);
            if (!succeeded(res, "Create swapchain")) {
                return false;
            }

            uint32_t imageCount = 0;
            res = xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);
            if (!succeeded(res, "Enumerate swapchain image count")) {
                return false;
            }
            if (graphicsBackend == GraphicsBackend::OpenGL) {
                swapchain.images.assign(
                    imageCount,
                    { .type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR }
                );
                const XrResult r = xrEnumerateSwapchainImages(
                    swapchain.handle,
                    imageCount,
                    &imageCount,
                    reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())
                );
                if (!succeeded(r, "Enumerate OpenGL swapchain images")) {
                    return false;
                }

                glGenFramebuffers(1, &swapchain.framebuffer);
            }
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
            else {
                swapchain.vulkanImages.assign(
                    imageCount,
                    { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR }
                );
                XrResult r = xrEnumerateSwapchainImages(
                    swapchain.handle,
                    imageCount,
                    &imageCount,
                    reinterpret_cast<XrSwapchainImageBaseHeader*>(
                        swapchain.vulkanImages.data()
                    )
                );
                if (!succeeded(r, "Enumerate Vulkan swapchain images")) {
                    return false;
                }
            }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
        }

        Log::Info(std::format(
            "OpenXR render dimensions per eye: {} x {} ({})",
            swapchains[0].width, swapchains[0].height,
            graphicsBackend == GraphicsBackend::OpenGL ? "OpenGL" : "Vulkan"
        ));
        return true;
    }

    bool blitEye(const Window& window, uint32_t index) {
        Swapchain& swapchain = swapchains[index];
        const FrustumMode eye = index == 0 ?
            FrustumMode::StereoLeft :
            FrustumMode::StereoRight;

        XrSwapchainImageAcquireInfo acquireInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO
        };
        uint32_t imageIndex = 0;
        XrResult res = xrAcquireSwapchainImage(
            swapchain.handle,
            &acquireInfo,
            &imageIndex
        );
        if (!succeeded(res, "Acquire swapchain image")) {
            return false;
        }

        const XrSwapchainImageWaitInfo waitInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .timeout = XR_INFINITE_DURATION
        };
        res = xrWaitSwapchainImage(swapchain.handle, &waitInfo);
        if (!succeeded(res, "Wait swapchain image")) {
            return false;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, swapchain.framebuffer);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            swapchain.images[imageIndex].image,
            0
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        GLuint readFramebuffer = 0;
        glGenFramebuffers(1, &readFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);

        const ivec2 dim = window.framebufferResolution();
        const auto attachSourceTexture = [](GLuint texture) {
            glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                texture,
                0
            );
        };

        bool copiedViewport = false;
        attachSourceTexture(window.frameBufferTextureEye(Eye::MonoOrLeft));
        const auto blitSourceRect = [&swapchain](int x, int y, int width, int height) {
            glBlitFramebuffer(
                x,
                y,
                x + width,
                y + height,
                0,
                0,
                swapchain.width,
                swapchain.height,
                GL_COLOR_BUFFER_BIT,
                GL_LINEAR
            );
        };

        for (const std::unique_ptr<Viewport>& viewport : window.viewports()) {
            if (!viewport->isEnabled() || viewport->eye() != eye) {
                continue;
            }

            const vec2& position = viewport->position();
            const vec2& size = viewport->size();
            const int srcX = static_cast<int>(position.x * dim.x);
            const int srcY = static_cast<int>(position.y * dim.y);
            const int srcWidth = static_cast<int>(size.x * dim.x);
            const int srcHeight = static_cast<int>(size.y * dim.y);

            blitSourceRect(srcX, srcY, srcWidth, srcHeight);
            copiedViewport = true;
        }

        if (!copiedViewport &&
            (window.stereoMode() == Window::StereoMode::SideBySide ||
             window.stereoMode() == Window::StereoMode::SideBySideInverted))
        {
            for (const std::unique_ptr<Viewport>& vp : window.viewports()) {
                if (!vp->isEnabled() || vp->eye() != FrustumMode::Mono) {
                    continue;
                }

                const vec2& position = vp->position();
                const vec2& size = vp->size();
                const int srcX = static_cast<int>(position.x * dim.x);
                const int srcY = static_cast<int>(position.y * dim.y);
                const int srcWidth = static_cast<int>(size.x * dim.x);
                const int srcHeight = static_cast<int>(size.y * dim.y);
                const int eyeWidth = srcWidth / 2;
                const bool useLeftHalf =
                    (index == 0) ==
                    (window.stereoMode() == Window::StereoMode::SideBySide);

                const int s = srcX + (useLeftHalf ? 0 : eyeWidth);
                blitSourceRect(s, srcY, eyeWidth, srcHeight);
                copiedViewport = true;
            }
        }

        if (!copiedViewport) {
            attachSourceTexture(
                window.frameBufferTextureEye(index == 0 ? Eye::MonoOrLeft : Eye::Right)
            );
            glBlitFramebuffer(
                0,
                0,
                dim.x,
                dim.y,
                0,
                0,
                swapchain.width,
                swapchain.height,
                GL_COLOR_BUFFER_BIT,
                GL_LINEAR
            );
        }

        glDeleteFramebuffers(1, &readFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        XrSwapchainImageReleaseInfo releaseInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
        };
        const XrResult res2 = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
        return succeeded(res2, "Release swapchain image");
    }

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    bool copyEyeVulkan(const Window& window, uint32_t index) {
        Swapchain& swapchain = swapchains[index];

        const XrSwapchainImageAcquireInfo acquireInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO
        };
        uint32_t imageIndex = 0;
        XrResult res = xrAcquireSwapchainImage(
            swapchain.handle,
            &acquireInfo,
            &imageIndex
        );
        if (!succeeded(res, "Acquire Vulkan swapchain image")) {
            return false;
        }

        XrSwapchainImageWaitInfo waitInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .timeout = XR_INFINITE_DURATION
        };
        res = xrWaitSwapchainImage(swapchain.handle, &waitInfo);
        if (!succeeded(res, "Wait Vulkan swapchain image")) {
            XrSwapchainImageReleaseInfo releaseInfo = {
                .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
            };
            XrResult r = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
            succeeded(r, "Release unwaited Vulkan swapchain image");
            return false;
        }

        const bool copied = copyOpenGLFramebufferToVulkanImage(
            window,
            index,
            swapchain.vulkanImages[imageIndex].image,
            swapchain.width,
            swapchain.height
        );

        XrSwapchainImageReleaseInfo releaseInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
        };
        res = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
        const bool released = succeeded(res, "Release Vulkan swapchain image");
        return copied && released;
    }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    bool copyEye(const Window& window, uint32_t index) {
        if (graphicsBackend == GraphicsBackend::OpenGL) {
            return blitEye(window, index);
        }
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
        return copyEyeVulkan(window, index);
#else // ^^^^ SGCT_HAS_OPENXR_VULKAN_FALLBACK // fallback unavailable vvvv
        return false;
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK
    }
} // namespace

namespace sgct::openxr {

void initialize(float nearClip, float farClip) {
    if (isOpenXRInitialized) {
        Log::Info("OpenXR has already been initialized");
        return;
    }

    hmdNearClip = nearClip;
    hmdFarClip = farClip;

    bool backendInitialized = initializeBackend(GraphicsBackend::OpenGL);
#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    if (!backendInitialized) {
        Log::Info("OpenXR OpenGL initialization failed; trying Vulkan fallback");
        backendInitialized = initializeBackend(GraphicsBackend::Vulkan);
    }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    if (!backendInitialized || !createSpace() || !enumerateViewConfiguration()) {
        shutdown();
        return;
    }

    updateHMDMatrices(nearClip, farClip);
    isOpenXRInitialized = true;
    Log::Info("OpenXR initialized");
}

void shutdown() {
    endFrame(nullptr, 0);

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    if (vulkan.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(vulkan.device);
    }
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    destroySwapchains();

    if (session != XR_NULL_HANDLE && isSessionRunning) {
        xrEndSession(session);
        isSessionRunning = false;
    }
    if (appSpace != XR_NULL_HANDLE) {
        xrDestroySpace(appSpace);
        appSpace = XR_NULL_HANDLE;
    }
    if (session != XR_NULL_HANDLE) {
        xrDestroySession(session);
        session = XR_NULL_HANDLE;
    }
    if (instance != XR_NULL_HANDLE) {
        xrDestroyInstance(instance);
        instance = XR_NULL_HANDLE;
    }

#ifdef SGCT_HAS_OPENXR_VULKAN_FALLBACK
    destroyVulkanBackend();
#endif // SGCT_HAS_OPENXR_VULKAN_FALLBACK

    systemId = XR_NULL_SYSTEM_ID;
    resetFrameState();
    isOpenXRInitialized = false;
}

bool isHMDActive() {
    return isOpenXRInitialized && session != XR_NULL_HANDLE;
}

ivec2 eyeResolution(FrustumMode eye) {
    if (!isHMDActive()) {
        return ivec2{ 0, 0 };
    }

    const XrViewConfigurationView& view = configurationViews[eyeIndex(eye)];
    return ivec2 {
        static_cast<int>(view.recommendedImageRectWidth),
        static_cast<int>(view.recommendedImageRectHeight)
    };
}

void copyWindowToHMD(Window* win) {    
    if (!win || !isHMDActive() || !isFrameBegun || predictedDisplayTime == 0 ||
        !isSessionRunning || !shouldSubmitFrame)
    {
        return;
    }
    if (!areViewsValid) {
        endFrame(nullptr, 0);
        return;
    }
    if (!createSwapchains()) {
        endFrame(nullptr, 0);
        return;
    }

    if (!copyEye(*win, 0)) {
        endFrame(nullptr, 0);
        return;
    }
    if (!copyEye(*win, 1)) {
        endFrame(nullptr, 0);
        return;
    }

    for (uint32_t i = 0; i < EyeCount; ++i) {
        projectionViews[i] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .pose = views[i].pose,
            .fov = views[i].fov,
            .subImage = {
                .swapchain = swapchains[i].handle,
                .imageRect = {
                    .offset = { 0, 0 },
                    .extent = { swapchains[i].width, swapchains[i].height }
                }
            }
        };
    }

    const XrCompositionLayerProjection projectionLayer = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .space = appSpace,
        .viewCount = EyeCount,
        .views = projectionViews.data()
    };
    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };
    endFrame(layers, 1);
}

glm::mat4 currentViewProjectionMatrix(FrustumMode eye) {
    return eyeProjectionMatrix(eye, hmdNearClip, hmdFarClip) *
        eyeToHeadMatrices[eyeIndex(eye)] * poseHMDMat;
}

void updatePoses() {
    if (!isHMDActive() || !waitForRunningSession()) {
        return;
    }
    if (isFrameBegun) {
        endFrame(nullptr, 0);
    }

    XrFrameWaitInfo waitInfo = { .type = XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState frameState = { .type = XR_TYPE_FRAME_STATE };
    XrResult res = xrWaitFrame(session, &waitInfo, &frameState);
    if (!succeeded(res, "Wait frame")) {
        resetFrameState();
        return;
    }
    predictedDisplayTime = frameState.predictedDisplayTime;

    XrFrameBeginInfo beginInfo = {
        .type = XR_TYPE_FRAME_BEGIN_INFO
    };
    res = xrBeginFrame(session, &beginInfo);
    if (!succeeded(res, "Begin frame")) {
        resetFrameState();
        return;
    }
    isFrameBegun = true;
    shouldSubmitFrame = static_cast<bool>(frameState.shouldRender);
    areViewsValid = false;

    if (!isSessionRunning || !frameState.shouldRender) {
        endFrame(nullptr, 0);
        return;
    }

    views = {
        XrView{ .type = XR_TYPE_VIEW },
        XrView{ .type = XR_TYPE_VIEW }
    };
    XrViewState viewState = { .type = XR_TYPE_VIEW_STATE };
    XrViewLocateInfo locateInfo = {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        .displayTime = predictedDisplayTime,
        .space = appSpace
    };

    uint32_t viewCount = 0;
    res = xrLocateViews(
        session,
        &locateInfo,
        &viewState,
        EyeCount,
        &viewCount,
        views.data()
    );
    if (!succeeded(res, "Locate views")) {
        endFrame(nullptr, 0);
        return;
    }

    areViewsValid = viewCount == EyeCount &&
        (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0 &&
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0;
    if (areViewsValid) {
        eyeToHeadMatrices[0] = glm::inverse(transformMatrix(views[0].pose));
        eyeToHeadMatrices[1] = glm::inverse(transformMatrix(views[1].pose));
        poseHMDMat = glm::mat4(1.f);
    }
    else {
        endFrame(nullptr, 0);
    }
}

void updateHMDMatrices(float nearClip, float farClip) {
    hmdNearClip = nearClip;
    hmdFarClip = farClip;

    for (uint32_t i = 0; i < EyeCount; ++i) {
        const XrViewConfigurationView& view = configurationViews[i];
        views[i] = {
            .type = XR_TYPE_VIEW,
            .fov = {
                .angleLeft = -std::atan(0.5f),
                .angleRight = std::atan(0.5f),
                .angleUp = std::atan(0.5f),
                .angleDown = -std::atan(0.5f)
            }
        };

        if (view.recommendedImageRectWidth > 0 && view.recommendedImageRectHeight > 0) {
            const float aspect = static_cast<float>(view.recommendedImageRectWidth) /
                static_cast<float>(view.recommendedImageRectHeight);
            views[i].fov.angleLeft = -std::atan(aspect * 0.5f);
            views[i].fov.angleRight = std::atan(aspect * 0.5f);
        }
    }

    eyeProjectionMatrices[0] = eyeProjectionMatrix(
        FrustumMode::StereoLeft,
        nearClip,
        farClip
    );
    eyeProjectionMatrices[1] = eyeProjectionMatrix(
        FrustumMode::StereoRight,
        nearClip,
        farClip
    );
}

glm::mat4 eyeProjectionMatrix(FrustumMode eye, float nearClip, float farClip) {
    if (!isHMDActive()) {
        return glm::mat4(1.f);
    }
    return projectionMatrix(views[eyeIndex(eye)].fov, nearClip, farClip);
}

glm::mat4 eyeToHeadTransform(FrustumMode eye) {
    return eyeToHeadMatrices[eyeIndex(eye)];
}

glm::mat4 poseMatrix() {
    return poseHMDMat;
}

glm::quat inverseRotation(glm::mat4 matPose) {
    glm::quat q;
    q.w = std::sqrt(
        std::max(0.f, 1.f + matPose[0][0] + matPose[1][1] + matPose[2][2])
    ) / 2.f;
    q.x = std::sqrt(
        std::max(0.f, 1.f + matPose[0][0] - matPose[1][1] - matPose[2][2])
    ) / 2.f;
    q.y = std::sqrt(
        std::max(0.f, 1.f - matPose[0][0] + matPose[1][1] - matPose[2][2])
    ) / 2.f;
    q.z = std::sqrt(
        std::max(0.f, 1.f - matPose[0][0] - matPose[1][1] + matPose[2][2])
    ) / 2.f;
    q.x = std::copysign(q.x, matPose[2][1] - matPose[1][2]);
    q.y = std::copysign(q.y, matPose[0][2] - matPose[2][0]);
    q.z = std::copysign(q.z, matPose[1][0] - matPose[0][1]);
    return q;
}

} // namespace sgct::openxr

#endif // SGCT_HAS_OPENXR
