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
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/offscreenbuffer.h>
#include <sgct/vulkan/projectionpass.h>

#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN
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
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {
    using namespace sgct;
    // On X11 the global `Window` typedef (from Xlib.h, pulled in by the Vulkan Xlib
    // surface and the OpenXR headers) would otherwise be ambiguous with sgct::Window
    using sgct::Window;

    constexpr uint32_t EyeCount = 2;

    struct Swapchain {
        XrSwapchain handle = XR_NULL_HANDLE;
        int32_t width = 0;
        int32_t height = 0;
        std::vector<XrSwapchainImageVulkan2KHR> images;
        std::vector<VkImageView> views;
        std::vector<VkFramebuffer> framebuffers;
        /// Whether an image was acquired that has to be released in submitFrame
        bool isAcquired = false;
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
    /// Whether the copies into the swapchain images were recorded into the window's
    /// command buffer, so that the frame has to be submitted in submitFrame
    bool isFrameRecorded = false;

    PFN_xrGetVulkanGraphicsRequirements2KHR getVulkanGraphicsRequirements = nullptr;
    PFN_xrCreateVulkanInstanceKHR createVulkanInstance = nullptr;
    PFN_xrGetVulkanGraphicsDevice2KHR getVulkanGraphicsDevice = nullptr;
    PFN_xrCreateVulkanDeviceKHR createVulkanDevice = nullptr;

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

    // The objects that copy the window's render target into the swapchain images
    struct {
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        std::unique_ptr<vulkan::ProjectionPass> pass;
    } copy;

    glm::mat4 poseHMDMat = glm::mat4(1.f);
    float hmdNearClip = 0.1f;
    float hmdFarClip = 100.f;

    // Samples the window's render target; the texture coordinates of the quad select the
    // area of the eye
    constexpr const char* CopyFrag = R"(
  #version 460

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_color;

  layout (set = 0, binding = 0) uniform sampler2D tex;

  void main() {
    out_color = texture(tex, in_texCoords);
  }
)";

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

    bool succeeded(VkResult result, std::string_view action) {
        if (result == VK_SUCCESS) {
            return true;
        }
        Log::Error(std::format("Vulkan {} failed: {}", action, static_cast<int>(result)));
        return false;
    }

    template <typename T>
    bool loadFunction(const char* name, T& function) {
        PFN_xrVoidFunction proc = nullptr;
        const XrResult res = xrGetInstanceProcAddr(instance, name, &proc);
        const bool success = succeeded(res, std::format("Get {} function", name));
        if (success) {
            function = reinterpret_cast<T>(proc);
        }
        return success;
    }

    void resetFrameState() {
        predictedDisplayTime = 0;
        isFrameBegun = false;
        shouldSubmitFrame = false;
        areViewsValid = false;
        isFrameRecorded = false;
    }

    void releaseImages() {
        for (Swapchain& swapchain : swapchains) {
            if (!swapchain.isAcquired) {
                continue;
            }
            const XrSwapchainImageReleaseInfo releaseInfo = {
                .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
            };
            const XrResult res = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
            succeeded(res, "Release swapchain image");
            swapchain.isAcquired = false;
        }
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

    void destroySwapchains() {
        if (swapchains[0].handle == XR_NULL_HANDLE && copy.renderPass == VK_NULL_HANDLE) {
            // Nothing was created, and the Vulkan context might not exist
            return;
        }

        vulkan::Context& ctx = vulkan::Context::instance();
        if (ctx.device() != VK_NULL_HANDLE) {
            // The last frames might still be copying into the swapchain images
            vkDeviceWaitIdle(ctx.device());
        }

        for (Swapchain& swapchain : swapchains) {
            for (VkFramebuffer framebuffer : swapchain.framebuffers) {
                vkDestroyFramebuffer(ctx.device(), framebuffer, nullptr);
            }
            swapchain.framebuffers.clear();
            for (VkImageView view : swapchain.views) {
                vkDestroyImageView(ctx.device(), view, nullptr);
            }
            swapchain.views.clear();
            swapchain.images.clear();
            if (swapchain.handle != XR_NULL_HANDLE) {
                xrDestroySwapchain(swapchain.handle);
                swapchain.handle = XR_NULL_HANDLE;
            }
            swapchain.isAcquired = false;
        }

        copy.pass = nullptr;
        if (copy.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(ctx.device(), copy.sampler, nullptr);
            copy.sampler = VK_NULL_HANDLE;
        }
        if (copy.renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(ctx.device(), copy.renderPass, nullptr);
            copy.renderPass = VK_NULL_HANDLE;
        }
    }

    void destroyInstance() {
        if (instance != XR_NULL_HANDLE) {
            xrDestroyInstance(instance);
            instance = XR_NULL_HANDLE;
        }
        systemId = XR_NULL_SYSTEM_ID;
        getVulkanGraphicsRequirements = nullptr;
        createVulkanInstance = nullptr;
        getVulkanGraphicsDevice = nullptr;
        createVulkanDevice = nullptr;
    }

    bool createInstance() {
        if (!hasExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME)) {
            Log::Error(std::format(
                "OpenXR runtime does not support {}", XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME
            ));
            return false;
        }

        const char* extensions[] = { XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME };
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

    bool checkVulkanRequirements() {
        const bool hasFunctions =
            loadFunction(
                "xrGetVulkanGraphicsRequirements2KHR",
                getVulkanGraphicsRequirements
            ) &&
            loadFunction("xrCreateVulkanInstanceKHR", createVulkanInstance) &&
            loadFunction("xrGetVulkanGraphicsDevice2KHR", getVulkanGraphicsDevice) &&
            loadFunction("xrCreateVulkanDeviceKHR", createVulkanDevice);
        if (!hasFunctions) {
            return false;
        }

        // The requirements have to be queried before the session is created
        XrGraphicsRequirementsVulkan2KHR requirements = {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR
        };
        const XrResult res =
            getVulkanGraphicsRequirements(instance, systemId, &requirements);
        if (!succeeded(res, "Get Vulkan requirements")) {
            return false;
        }

        constexpr XrVersion VulkanVersion = XR_MAKE_VERSION(1, 4, 0);
        auto versionString = [](XrVersion v) {
            return std::format("{}.{}", XR_VERSION_MAJOR(v), XR_VERSION_MINOR(v));
        };
        if (VulkanVersion < requirements.minApiVersionSupported) {
            Log::Error(std::format(
                "OpenXR runtime requires Vulkan {}",
                versionString(requirements.minApiVersionSupported)
            ));
            return false;
        }
        if (VulkanVersion > requirements.maxApiVersionSupported) {
            // Newer versions might work if they are compatible
            Log::Warning(std::format(
                "OpenXR runtime was tested up to Vulkan {}, but Vulkan 1.4 is used",
                versionString(requirements.maxApiVersionSupported)
            ));
        }
        return true;
    }

    bool createSession() {
        vulkan::Context& ctx = vulkan::Context::instance();
        const XrGraphicsBindingVulkan2KHR graphicsBinding = {
            .type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR,
            .instance = ctx.vkInstance(),
            .physicalDevice = ctx.physicalDevice(),
            .device = ctx.device(),
            .queueFamilyIndex = ctx.graphicsQueueFamily(),
            .queueIndex = 0
        };

        const XrSessionCreateInfo createInfo = {
            .type = XR_TYPE_SESSION_CREATE_INFO,
            .next = &graphicsBinding,
            .systemId = systemId
        };
        const XrResult res = xrCreateSession(instance, &createInfo, &session);
        return succeeded(res, "Create session");
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

    bool isSrgb(VkFormat format) {
        return format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_B8G8R8A8_SRGB;
    }

    struct FormatChoice {
        /// The format of the swapchain images
        VkFormat swapchain = VK_FORMAT_UNDEFINED;
        /// The format of the image views that the window is copied into
        VkFormat view = VK_FORMAT_UNDEFINED;
    };

    FormatChoice chooseSwapchainFormat(VkFormat sourceFormat) {
        uint32_t formatCount = 0;
        XrResult res = xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
        if (!succeeded(res, "Enumerate swapchain format count")) {
            return {};
        }

        std::vector<int64_t> formats(formatCount);
        res = xrEnumerateSwapchainFormats(
            session,
            formatCount,
            &formatCount,
            formats.data()
        );
        if (!succeeded(res, "Enumerate swapchain formats")) {
            return {};
        }

        constexpr std::array<VkFormat, 4> PreferredFormats = {
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_FORMAT_B8G8R8A8_SRGB,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8A8_UNORM
        };
        for (VkFormat preferredFormat : PreferredFormats) {
            auto it = std::find(formats.begin(), formats.end(), preferredFormat);
            if (it == formats.end()) {
                continue;
            }

            // The window's render target contains the colors that a monitor interprets
            // as sRGB-encoded. The runtime decodes sRGB swapchain images, so the values
            // are copied unchanged by writing them through a UNORM view if the render
            // target is not an sRGB image itself (the OpenGL renderer blitted them
            // without an sRGB conversion as well)
            FormatChoice choice = { preferredFormat, preferredFormat };
            if (isSrgb(preferredFormat) && !isSrgb(sourceFormat)) {
                choice.view = preferredFormat == VK_FORMAT_R8G8B8A8_SRGB ?
                    VK_FORMAT_R8G8B8A8_UNORM :
                    VK_FORMAT_B8G8R8A8_UNORM;
            }
            return choice;
        }

        if (formats.empty()) {
            return {};
        }
        Log::Warning("OpenXR runtime does not support any of the preferred formats");
        const VkFormat format = static_cast<VkFormat>(formats.front());
        return { format, format };
    }

    bool createCopyPass(VkFormat format) {
        vulkan::Context& ctx = vulkan::Context::instance();

        // The whole image is overwritten, and the runtime expects the image to be in the
        // color attachment layout when it is released
        VkAttachmentDescription attachment = {};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference reference = {};
        reference.attachment = 0;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &reference;

        // The previous copy into the image has to finish before the layout transition
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        VkResult res = vkCreateRenderPass(
            ctx.device(),
            &renderPassInfo,
            nullptr,
            &copy.renderPass
        );
        if (!succeeded(res, "Create copy render pass")) {
            return false;
        }

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxLod = 0.f;
        res = vkCreateSampler(ctx.device(), &samplerInfo, nullptr, &copy.sampler);
        if (!succeeded(res, "Create copy sampler")) {
            return false;
        }

        vulkan::ProjectionPass::Settings settings;
        settings.fragmentShader = CopyFrag;
        settings.pushConstantSize = sizeof(std::array<vec4, 4>);
        copy.pass = std::make_unique<vulkan::ProjectionPass>(
            ctx,
            copy.renderPass,
            settings
        );
        return true;
    }

    bool createSwapchains(const Window& window) {
        if (swapchains[0].handle != XR_NULL_HANDLE) {
            return true;
        }

        const vulkan::OffscreenBuffer* target = window.offscreenTarget(Eye::MonoOrLeft);
        if (!target) {
            return false;
        }

        const FormatChoice format = chooseSwapchainFormat(target->colorFormat());
        if (format.swapchain == VK_FORMAT_UNDEFINED) {
            Log::Error("OpenXR runtime did not report a usable swapchain format");
            return false;
        }

        if (!createCopyPass(format.view)) {
            return false;
        }

        vulkan::Context& ctx = vulkan::Context::instance();
        for (uint32_t i = 0; i < EyeCount; ++i) {
            const XrViewConfigurationView& view = configurationViews[i];
            Swapchain& swapchain = swapchains[i];
            swapchain.width = static_cast<int32_t>(view.recommendedImageRectWidth);
            swapchain.height = static_cast<int32_t>(view.recommendedImageRectHeight);

            XrSwapchainUsageFlags usage = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            if (format.view != format.swapchain) {
                usage |= XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT;
            }
            const XrSwapchainCreateInfo createInfo = {
                .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                .usageFlags = usage,
                .format = format.swapchain,
                .sampleCount = 1,
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
            swapchain.images.assign(
                imageCount,
                { .type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR }
            );
            res = xrEnumerateSwapchainImages(
                swapchain.handle,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())
            );
            if (!succeeded(res, "Enumerate swapchain images")) {
                return false;
            }

            for (const XrSwapchainImageVulkan2KHR& image : swapchain.images) {
                VkImageViewCreateInfo viewInfo = {};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = image.image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = format.view;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.layerCount = 1;
                VkImageView imageView = VK_NULL_HANDLE;
                VkResult r =
                    vkCreateImageView(ctx.device(), &viewInfo, nullptr, &imageView);
                if (!succeeded(r, "Create swapchain image view")) {
                    return false;
                }
                swapchain.views.push_back(imageView);

                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = copy.renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &imageView;
                framebufferInfo.width = view.recommendedImageRectWidth;
                framebufferInfo.height = view.recommendedImageRectHeight;
                framebufferInfo.layers = 1;
                VkFramebuffer framebuffer = VK_NULL_HANDLE;
                r = vkCreateFramebuffer(
                    ctx.device(),
                    &framebufferInfo,
                    nullptr,
                    &framebuffer
                );
                if (!succeeded(r, "Create swapchain framebuffer")) {
                    return false;
                }
                swapchain.framebuffers.push_back(framebuffer);
            }
        }

        Log::Info(std::format(
            "OpenXR render dimensions per eye: {} x {}",
            swapchains[0].width, swapchains[0].height
        ));
        return true;
    }

    struct EyeSource {
        const vulkan::OffscreenBuffer* target = nullptr;
        /// The area of the eye in pixels with OpenGL's bottom-left origin
        ivec4 rect;
    };

    EyeSource eyeSource(const Window& window, uint32_t index) {
        const ivec2 dim = window.framebufferResolution();
        const FrustumMode eye = index == 0 ?
            FrustumMode::StereoLeft :
            FrustumMode::StereoRight;

        auto pixelRect = [dim](const Viewport& vp) {
            return ivec4{
                static_cast<int>(vp.position().x * dim.x),
                static_cast<int>(vp.position().y * dim.y),
                static_cast<int>(vp.size().x * dim.x),
                static_cast<int>(vp.size().y * dim.y)
            };
        };

        // A viewport that is dedicated to the eye; the last one wins
        std::optional<ivec4> rect;
        for (const std::unique_ptr<Viewport>& viewport : window.viewports()) {
            if (viewport->isEnabled() && viewport->eye() == eye) {
                rect = pixelRect(*viewport);
            }
        }
        if (rect) {
            return { window.offscreenTarget(Eye::MonoOrLeft), *rect };
        }

        // The half of a mono viewport that contains the eye in side-by-side stereo
        const Window::StereoMode sm = window.stereoMode();
        if (sm == Window::StereoMode::SideBySide ||
            sm == Window::StereoMode::SideBySideInverted)
        {
            for (const std::unique_ptr<Viewport>& viewport : window.viewports()) {
                if (!viewport->isEnabled() || viewport->eye() != FrustumMode::Mono) {
                    continue;
                }

                ivec4 r = pixelRect(*viewport);
                const int eyeWidth = r.z / 2;
                const bool useLeftHalf =
                    (index == 0) == (sm == Window::StereoMode::SideBySide);
                r.x += useLeftHalf ? 0 : eyeWidth;
                r.z = eyeWidth;
                rect = r;
            }
        }
        if (rect) {
            return { window.offscreenTarget(Eye::MonoOrLeft), *rect };
        }

        // The whole render target of the eye
        return {
            window.offscreenTarget(index == 0 ? Eye::MonoOrLeft : Eye::Right),
            ivec4{ 0, 0, dim.x, dim.y }
        };
    }

    bool recordEyeCopy(VkCommandBuffer cmd, const Window& window, uint32_t index) {
        Swapchain& swapchain = swapchains[index];

        const XrSwapchainImageAcquireInfo acquireInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO
        };
        uint32_t imageIndex = 0;
        XrResult res =
            xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex);
        if (!succeeded(res, "Acquire swapchain image")) {
            return false;
        }
        swapchain.isAcquired = true;

        const XrSwapchainImageWaitInfo waitInfo = {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .timeout = XR_INFINITE_DURATION
        };
        res = xrWaitSwapchainImage(swapchain.handle, &waitInfo);
        if (!succeeded(res, "Wait swapchain image")) {
            return false;
        }

        const EyeSource source = eyeSource(window, index);
        if (!source.target) {
            return false;
        }

        // The render target's first row is the top of the image (the scene is rendered
        // with a y-flipped viewport) like the first row of the swapchain image, whereas
        // the rectangle of the eye has a bottom-left origin
        const float w = static_cast<float>(source.target->extent().width);
        const float h = static_cast<float>(source.target->extent().height);
        const float u0 = source.rect.x / w;
        const float u1 = (source.rect.x + source.rect.z) / w;
        const float vTop = 1.f - (source.rect.y + source.rect.w) / h;
        const float vBottom = 1.f - source.rect.y / h;
        // The viewport is not flipped, so y = -1 is the top of the swapchain image
        const std::array<vec4, 4> corners = {
            vec4{ -1.f, -1.f, u0, vTop },
            vec4{ -1.f,  1.f, u0, vBottom },
            vec4{  1.f, -1.f, u1, vTop },
            vec4{  1.f,  1.f, u1, vBottom }
        };

        const VkExtent2D extent = {
            static_cast<uint32_t>(swapchain.width),
            static_cast<uint32_t>(swapchain.height)
        };
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = copy.renderPass;
        renderPassInfo.framebuffer = swapchain.framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.f;
        VkRect2D scissor = {};
        scissor.extent = extent;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        VkDescriptorImageInfo image = {};
        image.sampler = copy.sampler;
        image.imageView = source.target->colorImageView();
        image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        copy.pass->bind(cmd, corners.data());
        copy.pass->setTextures(cmd, std::span(&image, 1));
        copy.pass->drawQuad(cmd);

        vkCmdEndRenderPass(cmd);
        return true;
    }

    void abortFrame() {
        releaseImages();
        endFrame(nullptr, 0);
    }
} // namespace

namespace sgct::openxr {

bool initializeInstance() {
    if (instance != XR_NULL_HANDLE) {
        return true;
    }

    if (!createInstance() || !createSystem() || !checkVulkanRequirements()) {
        destroyInstance();
        return false;
    }
    return true;
}

vulkan::Context::CreationOverrides vulkanCreationOverrides() {
    vulkan::Context::CreationOverrides overrides;
    overrides.createInstance = [](const VkInstanceCreateInfo& info, VkInstance& inst) {
        const XrVulkanInstanceCreateInfoKHR createInfo = {
            .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
            .systemId = systemId,
            .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vulkanCreateInfo = &info
        };
        VkResult vkResult = VK_SUCCESS;
        const XrResult res =
            createVulkanInstance(instance, &createInfo, &inst, &vkResult);
        if (!succeeded(res, "Create Vulkan instance")) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return vkResult;
    };
    overrides.physicalDevice = [](VkInstance inst) {
        const XrVulkanGraphicsDeviceGetInfoKHR getInfo = {
            .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
            .systemId = systemId,
            .vulkanInstance = inst
        };
        VkPhysicalDevice device = VK_NULL_HANDLE;
        const XrResult res = getVulkanGraphicsDevice(instance, &getInfo, &device);
        succeeded(res, "Get Vulkan graphics device");
        return device;
    };
    overrides.createDevice = [](VkPhysicalDevice physicalDevice,
                                const VkDeviceCreateInfo& info, VkDevice& device)
    {
        const XrVulkanDeviceCreateInfoKHR createInfo = {
            .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
            .systemId = systemId,
            .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vulkanPhysicalDevice = physicalDevice,
            .vulkanCreateInfo = &info
        };
        VkResult vkResult = VK_SUCCESS;
        const XrResult res =
            createVulkanDevice(instance, &createInfo, &device, &vkResult);
        if (!succeeded(res, "Create Vulkan device")) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return vkResult;
    };
    return overrides;
}

void initialize(float nearClip, float farClip) {
    if (isOpenXRInitialized) {
        Log::Info("OpenXR has already been initialized");
        return;
    }
    if (instance == XR_NULL_HANDLE) {
        // The Vulkan context was not created through the OpenXR runtime
        Log::Error("OpenXR is not available");
        return;
    }

    hmdNearClip = nearClip;
    hmdFarClip = farClip;

    if (!createSession() || !createSpace() || !enumerateViewConfiguration()) {
        shutdown();
        return;
    }

    isOpenXRInitialized = true;
    updateHMDMatrices(nearClip, farClip);
    Log::Info("OpenXR initialized");
}

void shutdown() {
    abortFrame();
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
    destroyInstance();

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

    // The window's frame might have been skipped, for example while it is hidden
    VkCommandBuffer cmd = win->frameCommandBuffer();
    if (!areViewsValid || cmd == VK_NULL_HANDLE || !createSwapchains(*win)) {
        abortFrame();
        return;
    }

    if (!recordEyeCopy(cmd, *win, 0) || !recordEyeCopy(cmd, *win, 1)) {
        // The images that were acquired cannot be released before the window's command
        // buffer was submitted, as the copies might have been recorded into it already
        isFrameRecorded = true;
        shouldSubmitFrame = false;
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
    isFrameRecorded = true;
}

void submitFrame() {
    if (!isFrameRecorded) {
        return;
    }

    // The copies into the swapchain images have been submitted with the window's command
    // buffer, so the runtime can take them over
    releaseImages();
    if (!shouldSubmitFrame) {
        endFrame(nullptr, 0);
        return;
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
        abortFrame();
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
