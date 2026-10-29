/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_SWAPCHAIN__H__
#define __SGCT__VULKAN_SWAPCHAIN__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct GLFWwindow;

namespace sgct {
namespace vulkan {

/**
 * Thrown by Swapchain::acquireImage when the swapchain is out of date or suboptimal
 * and must be recreated by the window.
 */
class SGCT_EXPORT SwapchainOutOfDate final : public std::runtime_error {
public:
    SwapchainOutOfDate()
        : std::runtime_error("The swapchain is out of date and needs to be recreated")
    {}
};

/**
 * Owns the per-window VkSurfaceKHR, VkSwapchainKHR, the swapchain image views, and the
 * synchronization objects (image-available / render-finished semaphores and in-flight
 * fences) for a fixed number of frames. It is recreated when the window is resized.
 */
class SGCT_EXPORT Swapchain {
public:
    /// The number of frames that may be in flight at the same time
    static constexpr uint32_t MaxFramesInFlight = 2;

    /**
     * \param context The shared Vulkan context
     * \param window The GLFW window this swapchain presents to
     */
    Swapchain(Context& context, GLFWwindow* window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    /**
     * Requests swapchain images with two layers for stereoscopic presentation, where the
     * first layer is the left eye and the second the right eye. It is only used if the
     * surface supports it; see #layerCount. Must be called before #create.
     */
    void setStereo(bool stereo);

    /**
     * Requests that the swapchain joins the present barrier (VK_NV_present_barrier),
     * which synchronizes the presentation with other swapchains and nodes. It is only
     * used if the device and surface support it; see #usesPresentBarrier. Must be called
     * before #create.
     */
    void setPresentBarrier(bool presentBarrier);

    /**
     * Sets the present mode that the swapchain uses if the surface supports it, and
     * VK_PRESENT_MODE_FIFO_KHR (which every surface supports) otherwise. Must be called
     * before #create.
     */
    void setPresentMode(VkPresentModeKHR presentMode);

    /**
     * Creates the surface, swapchain, image views, and synchronization objects.
     *
     * \throw std::runtime_error If any part of the creation fails
     */
    void create();

    /**
     * Destroys the swapchain and all of its resources.
     */
    void destroy();

    /**
     * Waits for and acquires the next swapchain image.
     *
     * \param frameIndex The in-flight frame index (in [0, MaxFramesInFlight))
     *
     * \return The index of the acquired image
     * \throw std::runtime_error If acquiring the image fails
     */
    uint32_t acquireImage(uint32_t frameIndex) const;

    /**
     * Presents the provided swapchain image to the window. The presentation waits on the
     * image's render-finished semaphore.
     *
     * \param imageIndex The index of the image to present
     *
     * \throw std::runtime_error If presenting fails
     */
    void present(uint32_t imageIndex) const;

    VkSurfaceKHR surface() const { return _surface; }
    VkSwapchainKHR swapchain() const { return _swapchain; }
    VkImage image(uint32_t index) const { return _images.at(index); }
    /// A view of one layer of the swapchain image \p index
    VkImageView imageView(uint32_t index, uint32_t layer = 0) const {
        return _imageViews.at(index * _layerCount + layer);
    }
    /// All image views, ordered by image and then by layer
    const std::vector<VkImageView>& imageViews() const { return _imageViews; }
    /// The number of layers of the images, which is 2 for stereoscopic presentation
    uint32_t layerCount() const { return _layerCount; }
    /// Whether the swapchain uses the present barrier
    bool usesPresentBarrier() const { return _usesPresentBarrier; }
    VkFormat format() const { return _format; }
    VkExtent2D extent() const { return _extent; }
    uint32_t imageCount() const { return static_cast<uint32_t>(_images.size()); }
    bool isValid() const { return _valid; }
    /// Whether the images can be the source of copies, which is needed for screenshots
    bool supportsCapture() const { return _supportsCapture; }

    VkSemaphore imageAvailableSemaphore(uint32_t frameIndex) const {
        return _imageAvailableSemaphores.at(frameIndex);
    }
    /// The semaphore signaled when rendering into the swapchain image \p imageIndex has
    /// finished; there is one per swapchain image (not per in-flight frame)
    VkSemaphore renderFinishedSemaphore(uint32_t imageIndex) const {
        return _renderFinishedSemaphores.at(imageIndex);
    }
    VkFence inFlightFence(uint32_t frameIndex) const {
        return _inFlightFences.at(frameIndex);
    }

private:
    void chooseSurfaceFormat();
    void chooseSwapExtent();
    void createSwapchain();
    void createImageViews();
    void createSyncObjects();
    void cleanup();

    Context& _context;
    GLFWwindow* _window;

    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    VkExtent2D _extent = { 0, 0 };
    bool _valid = false;
    bool _supportsCapture = false;
    bool _wantsStereo = false;
    bool _wantsPresentBarrier = false;
    VkPresentModeKHR _presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t _layerCount = 1;
    bool _usesPresentBarrier = false;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_SWAPCHAIN__H__
