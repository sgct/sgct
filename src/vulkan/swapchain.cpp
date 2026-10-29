/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/swapchain.h>

#include <sgct/error.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>

namespace sgct {
namespace vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4100,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }
} // namespace

Swapchain::Swapchain(Context& context, GLFWwindow* window)
    : _context(context)
    , _window(window)
{}

Swapchain::~Swapchain() {
    destroy();
}

void Swapchain::setStereo(bool stereo) {
    _wantsStereo = stereo;
}

void Swapchain::setPresentBarrier(bool presentBarrier) {
    _wantsPresentBarrier = presentBarrier;
}

void Swapchain::setPresentMode(VkPresentModeKHR presentMode) {
    _presentMode = presentMode;
}

void Swapchain::create() {
    _surface = _context.createSurface(_window);
    chooseSurfaceFormat();
    chooseSwapExtent();
    createSwapchain();
    createImageViews();
    createSyncObjects();
    _valid = true;
}

void Swapchain::destroy() {
    cleanup();
    _valid = false;
}

void Swapchain::chooseSurfaceFormat() {
    uint32_t formatCount = 0;
    check(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            _context.physicalDevice(), _surface, &formatCount, nullptr
        ),
        "vkGetPhysicalDeviceSurfaceFormatsKHR"
    );
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        _context.physicalDevice(), _surface, &formatCount, formats.data()
    );

    // Prefer an 8-bit RGBA format with an alpha channel; fall back to the first format
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM ||
            format.format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            _format = format.format;
            return;
        }
    }
    _format = formats.front().format;
}

void Swapchain::chooseSwapExtent() {
    VkSurfaceCapabilitiesKHR capabilities = {};
    check(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            _context.physicalDevice(), _surface, &capabilities
        ),
        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"
    );

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);

    if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // The surface size is not fixed; clamp the window size to the supported range
        _extent.width = std::clamp<uint32_t>(
            width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
        );
        _extent.height = std::clamp<uint32_t>(
            height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
        );
    }
    else {
        _extent = capabilities.currentExtent;
    }
}

void Swapchain::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        _context.physicalDevice(), _surface, &capabilities
    );

    uint32_t desiredImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && desiredImageCount > capabilities.maxImageCount) {
        desiredImageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = desiredImageCount;
    createInfo.imageFormat = _format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = _extent;
    // Stereoscopic presentation uses one layer per eye
    _layerCount = _wantsStereo && capabilities.maxImageArrayLayers >= 2 ? 2 : 1;
    createInfo.imageArrayLayers = _layerCount;
    // The images can be copied from for screenshots of the back buffer if the surface
    // supports it
    _supportsCapture =
        (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        (_supportsCapture ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        _context.physicalDevice(),
        _surface,
        &modeCount,
        nullptr
    );
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        _context.physicalDevice(),
        _surface,
        &modeCount,
        modes.data()
    );
    const bool hasMode = std::find(modes.begin(), modes.end(), _presentMode) != modes.end();
    createInfo.presentMode = hasMode ? _presentMode : VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    // The present barrier has to be supported by the surface as well
    _usesPresentBarrier = false;
    if (_wantsPresentBarrier && _context.supportsPresentBarrier()) {
        VkSurfaceCapabilitiesPresentBarrierNV barrierCapabilities = {};
        barrierCapabilities.sType =
            VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_PRESENT_BARRIER_NV;
        VkSurfaceCapabilities2KHR capabilities2 = {};
        capabilities2.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
        capabilities2.pNext = &barrierCapabilities;
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        surfaceInfo.surface = _surface;
        check(
            vkGetPhysicalDeviceSurfaceCapabilities2KHR(
                _context.physicalDevice(),
                &surfaceInfo,
                &capabilities2
            ),
            "vkGetPhysicalDeviceSurfaceCapabilities2KHR"
        );
        _usesPresentBarrier = barrierCapabilities.presentBarrierSupported == VK_TRUE;
    }
    VkSwapchainPresentBarrierCreateInfoNV barrierInfo = {};
    barrierInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_BARRIER_CREATE_INFO_NV;
    barrierInfo.presentBarrierEnable = VK_TRUE;
    if (_usesPresentBarrier) {
        createInfo.pNext = &barrierInfo;
    }

    check(vkCreateSwapchainKHR(_context.device(), &createInfo, nullptr, &_swapchain),
        "vkCreateSwapchainKHR");

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(_context.device(), _swapchain, &imageCount, nullptr);
    _images.resize(imageCount);
    vkGetSwapchainImagesKHR(_context.device(), _swapchain, &imageCount, _images.data());
}

void Swapchain::createImageViews() {
    _imageViews.resize(_images.size() * _layerCount);
    for (size_t i = 0; i < _images.size(); i++) {
        for (uint32_t layer = 0; layer < _layerCount; layer++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = _format;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = layer;
            createInfo.subresourceRange.layerCount = 1;

            check(
                vkCreateImageView(
                    _context.device(),
                    &createInfo,
                    nullptr,
                    &_imageViews[i * _layerCount + layer]
                ),
                "vkCreateImageView"
            );
        }
    }
}

void Swapchain::createSyncObjects() {
    _imageAvailableSemaphores.resize(MaxFramesInFlight);
    _inFlightFences.resize(MaxFramesInFlight);
    // The presentation engine holds on to the render-finished semaphore until the image
    // it was presented with is acquired again, which is not tied to the in-flight frame
    // slots. So there is one render-finished semaphore per swapchain image instead
    _renderFinishedSemaphores.resize(_images.size());

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        check(
            vkCreateSemaphore(
                _context.device(), &semaphoreInfo, nullptr,
                &_imageAvailableSemaphores[i]
            ),
            "vkCreateSemaphore (image available)"
        );
        check(
            vkCreateFence(_context.device(), &fenceInfo, nullptr, &_inFlightFences[i]),
            "vkCreateFence"
        );
    }
    for (size_t i = 0; i < _renderFinishedSemaphores.size(); i++) {
        check(
            vkCreateSemaphore(
                _context.device(), &semaphoreInfo, nullptr,
                &_renderFinishedSemaphores[i]
            ),
            "vkCreateSemaphore (render finished)"
        );
    }
}

uint32_t Swapchain::acquireImage(uint32_t frameIndex) const {
    // Wait for the previous frame that used this in-flight slot to finish
    check(
        vkWaitForFences(
            _context.device(), 1, &_inFlightFences[frameIndex], VK_TRUE,
            std::numeric_limits<uint64_t>::max()
        ),
        "vkWaitForFences"
    );

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        _context.device(), _swapchain, std::numeric_limits<uint64_t>::max(),
        _imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        throw SwapchainOutOfDate();
    }
    check(result, "vkAcquireNextImageKHR");
    return imageIndex;
}

void Swapchain::present(uint32_t imageIndex) const {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(_context.queue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        throw SwapchainOutOfDate();
    }
    check(result, "vkQueuePresentKHR");
}

void Swapchain::cleanup() {
    if (_context.device() == VK_NULL_HANDLE) {
        return;
    }
    vkDeviceWaitIdle(_context.device());
    for (auto fence : _inFlightFences) {
        vkDestroyFence(_context.device(), fence, nullptr);
    }
    for (auto semaphore : _imageAvailableSemaphores) {
        vkDestroySemaphore(_context.device(), semaphore, nullptr);
    }
    for (auto semaphore : _renderFinishedSemaphores) {
        vkDestroySemaphore(_context.device(), semaphore, nullptr);
    }
    for (auto view : _imageViews) {
        vkDestroyImageView(_context.device(), view, nullptr);
    }
    if (_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_context.device(), _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
    }
    if (_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(_context.vkInstance(), _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
    _images.clear();
    _imageViews.clear();
    _inFlightFences.clear();
    _imageAvailableSemaphores.clear();
    _renderFinishedSemaphores.clear();
}

} // namespace vulkan
} // namespace sgct
