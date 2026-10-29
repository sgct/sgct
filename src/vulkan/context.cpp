/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/context.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#ifndef WIN32
// The Xlib surface extension uses the Xlib Display and Window types, so the Xlib
// header must be included before the Vulkan headers
#include <X11/Xlib.h>
#endif

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <format>
#include <mutex>
#include <stdexcept>
#include <string>

#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#else // ^^^^ WIN32 // !WIN32 vvvv
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif // WIN32

namespace sgct {
namespace vulkan {

namespace {
    /// The surface extension that matches the platform
    const char* surfaceExtension() {
#ifdef WIN32
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else // ^^^^ WIN32 // !WIN32 vvvv
        return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#endif // WIN32
    }

    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Engine,
                4000,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    bool hasExtension(
        const std::vector<VkExtensionProperties>& extensions, std::string_view name)
    {
        for (const auto& ext : extensions) {
            if (name == ext.extensionName) {
                return true;
            }
        }
        return false;
    }
} // namespace

Context* Context::_singleton = nullptr;

Context& Context::instance() {
    if (_singleton == nullptr) {
        throw std::logic_error("Using the Vulkan context before it was created");
    }
    return *_singleton;
}

Context& Context::create() {
    if (_singleton != nullptr) {
        throw std::logic_error("The Vulkan context has already been created");
    }
    _singleton = new Context();
    return *_singleton;
}

void Context::destroy() {
    if (_singleton) {
        _singleton->shutdown();
        delete _singleton;
        _singleton = nullptr;
    }
}

Context::~Context() {
    shutdown();
}

void Context::initialize(GLFWwindow* firstWindow, bool debug,
                         const CreationOverrides& overrides)
{
    if (_initialized) {
        throw std::logic_error("The Vulkan context has already been initialized");
    }
    _debug = debug;
    _overrides = overrides;

    createInstance(debug);
    createDebugMessenger();
    pickPhysicalDevice();
    findGraphicsQueue();
    createDevice();
    createCommandPool();
    _overrides = {};

    // Validate that the chosen device can present to the platform by creating the first
    // surface. The surface itself is owned by the window, so we create a throwaway one
    // here only to check support, then destroy it
    VkSurfaceKHR probe = createSurface(firstWindow);
    uint32_t support = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        _physicalDevice, _graphicsQueueFamily, probe, &support
    );
    vkDestroySurfaceKHR(_instance, probe, nullptr);
    if (support == VK_FALSE) {
        throw Error(
            Error::Component::Engine,
            4001,
            "The chosen Vulkan device cannot present to the platform surface"
        );
    }

    Log::Info(std::format(
        "Vulkan device: {} (API version {}.{}.{})",
        _properties.deviceName,
        VK_VERSION_MAJOR(_properties.apiVersion),
        VK_VERSION_MINOR(_properties.apiVersion),
        VK_VERSION_PATCH(_properties.apiVersion)
    ));

    _initialized = true;
}

void Context::shutdown() {
    if (!_initialized && _instance == VK_NULL_HANDLE) {
        return;
    }
    if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        if (_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(_device, _commandPool, nullptr);
            _commandPool = VK_NULL_HANDLE;
        }
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }
    if (_instance != VK_NULL_HANDLE) {
        if (_debugMessenger != VK_NULL_HANDLE && _destroyDebugUtilsMessengerEXT != nullptr) {
            _destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
            _debugMessenger = VK_NULL_HANDLE;
        }
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
    _physicalDevice = VK_NULL_HANDLE;
    _graphicsQueue = VK_NULL_HANDLE;
    _commandPool = VK_NULL_HANDLE;
    _initialized = false;
}

void Context::createInstance(bool debug) {
    uint32_t instanceExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(instanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(
        nullptr, &instanceExtensionCount, extensions.data()
    );

    std::vector<const char*> enabledExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        surfaceExtension()
    };
    if (!hasExtension(extensions, surfaceExtension())) {
        throw Error(
            Error::Component::Engine,
            4002,
            std::format("The surface extension '{}' is not supported", surfaceExtension())
        );
    }

    // Needed to query whether a surface supports the present barrier
    if (hasExtension(extensions, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
        enabledExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        _hasSurfaceCapabilities2 = true;
    }

    std::vector<const char*> enabledLayers;
    if (debug) {
        if (hasExtension(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else {
            Log::Warning("The Vulkan debug utils extension is not available");
        }

        // Enable the validation layer if it is available
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        for (const auto& layer : layers) {
            if (std::string_view(layer.layerName) == "VK_LAYER_KHRONOS_validation") {
                enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
                Log::Info("The Vulkan validation layer is enabled");
                break;
            }
        }
        if (enabledLayers.empty()) {
            Log::Warning("The Vulkan validation layer is not available");
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SGCT";
    appInfo.applicationVersion = VK_MAKE_VERSION(4, 0, 0);
    appInfo.pEngineName = "SGCT";
    appInfo.engineVersion = VK_MAKE_VERSION(4, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    if (_overrides.createInstance) {
        check(_overrides.createInstance(createInfo, _instance), "Create instance");
    }
    else {
        check(vkCreateInstance(&createInfo, nullptr, &_instance), "vkCreateInstance");
    }
}

void Context::createDebugMessenger() {
    if (!_debug || _instance == VK_NULL_HANDLE) {
        return;
    }
    // The VK_EXT_debug_utils entry points are not exported by the Vulkan loader, so they
    // have to be resolved through vkGetInstanceProcAddr
    _createDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT")
    );
    _destroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (_createDebugUtilsMessengerEXT == nullptr) {
        Log::Warning("The Vulkan debug utils extension is not available");
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = &Context::debugCallback;
    createInfo.pUserData = this;

    check(
        _createDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger),
        "vkCreateDebugUtilsMessengerEXT"
    );
}

VKAPI_ATTR VkBool32 VKAPI_CALL Context::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData
) {
    if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
    {
        Log::Warning(std::format("Vulkan: {}", data->pMessage));
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if (auto* context = static_cast<Context*>(userData)) {
            context->_validationErrors.fetch_add(1);
        }
    }
    return VK_FALSE;
}

void Context::pickPhysicalDevice() {
    if (_overrides.physicalDevice) {
        _physicalDevice = _overrides.physicalDevice(_instance);
        if (_physicalDevice == VK_NULL_HANDLE) {
            throw Error(Error::Component::Engine, 4007, "No Vulkan device was provided");
        }
        vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);
        _limits = _properties.limits;
        return;
    }

    uint32_t deviceCount = 0;
    check(vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr),
        "vkEnumeratePhysicalDevices");
    if (deviceCount == 0) {
        throw Error(Error::Component::Engine, 4003, "No Vulkan devices found");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    // Pick the first device that is not a software renderer; fall back to the first one
    for (const VkPhysicalDevice& device : devices) {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
            _physicalDevice = device;
            _properties = props;
            _limits = props.limits;
            return;
        }
    }
    _physicalDevice = devices.front();
    vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);
    _limits = _properties.limits;
}

void Context::findGraphicsQueue() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        _physicalDevice, &queueFamilyCount, nullptr
    );
    std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        _physicalDevice, &queueFamilyCount, families.data()
    );

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _graphicsQueueFamily = i;
            return;
        }
    }
    throw Error(
        Error::Component::Engine, 4004, "No graphics queue family found on the device"
    );
}

void Context::createDevice() {
    const float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = _graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // The present barrier synchronizes the presentation of swapchains across a cluster
    // with NVIDIA's frame lock hardware (the Vulkan equivalent of WGL_NV_swap_group)
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(
        _physicalDevice,
        nullptr,
        &extensionCount,
        nullptr
    );
    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        _physicalDevice,
        nullptr,
        &extensionCount,
        deviceExtensions.data()
    );
    VkPhysicalDevicePresentBarrierFeaturesNV presentBarrier = {};
    presentBarrier.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_BARRIER_FEATURES_NV;
    if (_hasSurfaceCapabilities2 &&
        hasExtension(deviceExtensions, VK_NV_PRESENT_BARRIER_EXTENSION_NAME))
    {
        VkPhysicalDeviceFeatures2 query = {};
        query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        query.pNext = &presentBarrier;
        vkGetPhysicalDeviceFeatures2(_physicalDevice, &query);
        _supportsPresentBarrier = presentBarrier.presentBarrier == VK_TRUE;
        if (_supportsPresentBarrier) {
            extensions.push_back(VK_NV_PRESENT_BARRIER_EXTENSION_NAME);
        }
    }
    VkPhysicalDeviceFeatures features = {};
    features.shaderFloat64 = VK_TRUE;

    // Push descriptors (used for the per-viewport mask and overlay textures) are core in
    // Vulkan 1.4 and must be supported, but still have to be enabled
    VkPhysicalDeviceVulkan14Features supported14 = {};
    supported14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    VkPhysicalDeviceFeatures2 supported = {};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supported.pNext = &supported14;
    vkGetPhysicalDeviceFeatures2(_physicalDevice, &supported);
    if (!supported14.pushDescriptor) {
        throw Error(
            Error::Component::Engine,
            4006,
            "The Vulkan device does not support push descriptors"
        );
    }
    VkPhysicalDeviceVulkan14Features features14 = {};
    features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    features14.pushDescriptor = VK_TRUE;
    if (_supportsPresentBarrier) {
        presentBarrier.pNext = nullptr;
        presentBarrier.presentBarrier = VK_TRUE;
        features14.pNext = &presentBarrier;
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features14;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &features;

    if (_overrides.createDevice) {
        check(_overrides.createDevice(_physicalDevice, createInfo, _device), "Create device");
    }
    else {
        check(vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device),
            "vkCreateDevice");
    }

    // Retrieve the actual queue handle; vkCreateDevice only requests it, it does not
    // hand it back
    vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
}

void Context::createCommandPool() {
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = _graphicsQueueFamily;

    check(vkCreateCommandPool(_device, &createInfo, nullptr, &_commandPool),
        "vkCreateCommandPool");
}

VkSurfaceKHR Context::createSurface(GLFWwindow* window) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
#ifdef WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = GetModuleHandle(nullptr);
    createInfo.hwnd = glfwGetWin32Window(window);
    check(vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &surface),
        "vkCreateWin32SurfaceKHR");
#else // ^^^^ WIN32 // !WIN32 vvvv
    // The Xlib surface entry point is not always exported by the loader, so resolve it
    // dynamically
    auto createXlibSurface = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
        vkGetInstanceProcAddr(_instance, "vkCreateXlibSurfaceKHR")
    );
    if (createXlibSurface == nullptr) {
        throw Error(Error::Component::Engine, 4005, "vkCreateXlibSurfaceKHR is not available");
    }
    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = glfwGetX11Display();
    createInfo.window = glfwGetX11Window(window);
    check(createXlibSurface(_instance, &createInfo, nullptr, &surface),
        "vkCreateXlibSurfaceKHR");
#endif // WIN32
    return surface;
}

std::vector<uint32_t> Context::compileShader(const char* source, Stage stage) const {
    static std::once_flag glslangInit;
    std::call_once(glslangInit, [] { glslang::InitializeProcess(); });

    const EShLanguage lang = [stage]() {
        switch (stage) {
            case Stage::Vertex:
                return EShLangVertex;
            case Stage::Fragment:
                return EShLangFragment;
            case Stage::Compute:
                return EShLangCompute;
        }
        return EShLangVertex;
    }();

    glslang::TShader shader(lang);
    shader.setStrings(&source, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    // Use glslang's built-in resource limits; a default-constructed TBuiltInResource has
    // uninitialized members (e.g. maxDrawBuffers), which makes the parser reject valid
    // fragment output locations
    const EShMessages messages =
        static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 460, false, messages)) {
        throw Error(
            Error::Component::Shader,
            4600,
            std::format("GLSL compilation failed:\n{}", shader.getInfoLog())
        );
    }

    // Linking resolves the implicitly sized built-in arrays (gl_ClipDistance and
    // gl_CullDistance in gl_PerVertex); without it they are emitted as runtime arrays,
    // which is invalid SPIR-V for Vulkan
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        throw Error(
            Error::Component::Shader,
            4601,
            std::format("GLSL linking failed:\n{}", program.getInfoLog())
        );
    }

    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(lang), spirv);
    return spirv;
}

VkShaderModule Context::createShaderModule(const std::vector<uint32_t>& spirv) const {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule module = VK_NULL_HANDLE;
    check(vkCreateShaderModule(_device, &createInfo, nullptr, &module),
        "vkCreateShaderModule");
    return module;
}

} // namespace vulkan
} // namespace sgct
