/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_CONTEXT__H__
#define __SGCT__VULKAN_CONTEXT__H__

#include <sgct/sgctexports.h>

#include <vulkan/vulkan.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct GLFWwindow;

namespace sgct {
namespace vulkan {

/// The shader stages that the context can compile GLSL sources for
enum class Stage : uint32_t {
    Vertex,
    Fragment,
    Compute
};

/**
 * The central Vulkan context for SGCT. It owns the VkInstance, the shared
 * VkPhysicalDevice and VkDevice, the graphics queue, the command pool, and the glslang
 * compiler used to turn GLSL sources into SPIR-V. It is created once by the Engine after
 * all windows exist (surfaces need the native window handles) and destroyed when the
 * Engine is torn down. All windows on a node share the same device.
 */
class SGCT_EXPORT Context {
public:
    static Context& instance();

    /**
     * Creates the global Vulkan context singleton. This function can only be called while
     * no context exists, i.e. it must be the first call or #destroy was called in between.
     *
     * \return A reference to the created context
     *
     * \throw std::logic_error If a context already exists
     */
    static Context& create();
    static void destroy();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /**
     * Functions that replace the creation of the Vulkan instance, the choice of the
     * physical device, and the creation of the logical device. They are used when an
     * OpenXR runtime has to create these objects (XR_KHR_vulkan_enable2), which adds the
     * extensions that the runtime requires. Each function that is empty is replaced by
     * the default behavior.
     */
    struct CreationOverrides {
        std::function<VkResult(const VkInstanceCreateInfo&, VkInstance&)> createInstance;
        std::function<VkPhysicalDevice(VkInstance)> physicalDevice;
        std::function<
            VkResult(VkPhysicalDevice, const VkDeviceCreateInfo&, VkDevice&)
        > createDevice;
    };

    /**
     * Creates the Vulkan instance, picks a physical device and graphics queue, and creates
     * the logical device and command pool. Must be called once, after all GLFW windows have
     * been created.
     *
     * \param firstWindow Any of the created GLFW windows; it is used to create the first
     *        surface, which is required to validate that the chosen device can present to
     *        the platform
     * \param debug Whether to enable the validation layer and debug messenger
     * \param overrides Replacements for the creation of the instance and device
     *
     * \throw std::runtime_error If any part of the initialization fails
     */
    void initialize(GLFWwindow* firstWindow, bool debug = false,
        const CreationOverrides& overrides = {});

    /**
     * Destroys all Vulkan resources held by the context.
     */
    void shutdown();

    VkInstance vkInstance() const { return _instance; }
    VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
    VkDevice device() const { return _device; }
    VkQueue queue() const { return _graphicsQueue; }
    uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }
    VkCommandPool commandPool() const { return _commandPool; }
    const VkPhysicalDeviceLimits& limits() const { return _limits; }
    const VkPhysicalDeviceProperties& properties() const { return _properties; }
    bool isInitialized() const { return _initialized; }

    /// Whether the device supports VK_NV_present_barrier, which is enabled if it does
    bool supportsPresentBarrier() const { return _supportsPresentBarrier; }

    /// Whether the instance has VK_KHR_get_surface_capabilities2 enabled
    bool hasSurfaceCapabilities2() const { return _hasSurfaceCapabilities2; }

    /**
     * Whether the validation layer reported at least one error through the debug
     * messenger since the context was initialized. Only meaningful when the context was
     * created with validation enabled.
     */
    bool hasValidationErrors() const { return _validationErrors.load() != 0; }

    /**
     * Creates a VkSurfaceKHR for the provided GLFW window. The caller is responsible for
     * destroying the returned surface with vkDestroySurfaceKHR.
     *
     * \param window The GLFW window to create the surface for
     *
     * \throw std::runtime_error If creating the surface fails
     */
    VkSurfaceKHR createSurface(GLFWwindow* window) const;

    /**
     * Compiles a GLSL source string into SPIR-V targeting the Vulkan client. The source
     * must contain a `#version` directive.
     *
     * \param source The GLSL source code
     * \param stage The shader stage the source belongs to
     *
     * \return The compiled SPIR-V bytecode
     * \throw std::runtime_error If the shader fails to compile; the message contains the
     *        glslang info log
     */
    std::vector<uint32_t> compileShader(const char* source, Stage stage) const;

    /**
     * Creates a VkShaderModule from precompiled SPIR-V. The caller owns the returned
     * module.
     *
     * \param spirv The SPIR-V bytecode
     *
     * \return The created shader module
     * \throw std::runtime_error If creating the module fails
     */
    VkShaderModule createShaderModule(const std::vector<uint32_t>& spirv) const;

private:
    Context() = default;
    ~Context();

    /// The global singleton instance, created by the Engine and removed through #destroy
    static Context* _singleton;

    void createInstance(bool debug);
    void createDebugMessenger();
    void pickPhysicalDevice();
    void findGraphicsQueue();
    void createDevice();
    void createCommandPool();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData
    );

    bool _initialized = false;
    bool _debug = false;
    /// The number of validation layer errors reported through the debug messenger
    std::atomic<uint32_t> _validationErrors{0};
    /// The overrides are only used during the initialization
    CreationOverrides _overrides;
    bool _supportsPresentBarrier = false;
    bool _hasSurfaceCapabilities2 = false;

    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    uint32_t _graphicsQueueFamily = 0;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties _properties = {};
    VkPhysicalDeviceLimits _limits = {};

    // The VK_EXT_debug_utils entry points are not exported by the Vulkan loader, so they
    // are resolved through vkGetInstanceProcAddr in createDebugMessenger
    PFN_vkCreateDebugUtilsMessengerEXT _createDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT _destroyDebugUtilsMessengerEXT = nullptr;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_CONTEXT__H__
