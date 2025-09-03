//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <noz/noz.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <algorithm>
#include <cstring>
#include <vector>

// Vulkan structures (declare first)
struct VulkanRenderer
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> swapchain_framebuffers;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    platform::Window* window;
    RendererTraits traits;
};

static VulkanRenderer g_vulkan = {};
static uint32_t g_current_image_index = 0;

// Dynamic Vulkan function pointers
static HMODULE vulkan_library = nullptr;

// Global function pointers
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
static PFN_vkCreateInstance vkCreateInstance = nullptr;
static PFN_vkDestroyInstance vkDestroyInstance = nullptr;
static PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = nullptr;
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
static PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
static PFN_vkCreateDevice vkCreateDevice = nullptr;
static PFN_vkDestroyDevice vkDestroyDevice = nullptr;
static PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
static PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
static PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
static PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = nullptr;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
static PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
static PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
static PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
static PFN_vkCreateImageView vkCreateImageView = nullptr;
static PFN_vkDestroyImageView vkDestroyImageView = nullptr;
static PFN_vkCreateRenderPass vkCreateRenderPass = nullptr;
static PFN_vkDestroyRenderPass vkDestroyRenderPass = nullptr;
static PFN_vkCreateFramebuffer vkCreateFramebuffer = nullptr;
static PFN_vkDestroyFramebuffer vkDestroyFramebuffer = nullptr;
static PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
static PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
static PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
static PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
static PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
static PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = nullptr;
static PFN_vkCmdEndRenderPass vkCmdEndRenderPass = nullptr;
static PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
static PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
static PFN_vkCreateFence vkCreateFence = nullptr;
static PFN_vkDestroyFence vkDestroyFence = nullptr;
static PFN_vkWaitForFences vkWaitForFences = nullptr;
static PFN_vkResetFences vkResetFences = nullptr;
static PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
static PFN_vkQueueSubmit vkQueueSubmit = nullptr;
static PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;

// Pipeline functions
static PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = nullptr;
static PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
static PFN_vkCreatePipelineLayout vkCreatePipelineLayout = nullptr;
static PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
static PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
static PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
static PFN_vkCmdBindPipeline vkCmdBindPipeline = nullptr;

static bool LoadVulkanLibrary()
{
    vulkan_library = LoadLibraryA("vulkan-1.dll");
    if (!vulkan_library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_library, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        return false;

    // Load global functions
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
    vkEnumerateInstanceLayerProperties =
        (PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties");

    return vkCreateInstance && vkEnumerateInstanceLayerProperties;
}

static void LoadInstanceFunctions(VkInstance instance)
{
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkEnumeratePhysicalDevices =
        (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(
        instance, "vkEnumerateDeviceExtensionProperties");
    vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
}

static void LoadDeviceFunctions(VkDevice device)
{
    vkDestroyDevice = (PFN_vkDestroyDevice)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyDevice");
    vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetInstanceProcAddr(g_vulkan.instance, "vkGetDeviceQueue");
    vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkGetInstanceProcAddr(g_vulkan.instance, "vkDeviceWaitIdle");
    vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateSwapchainKHR");
    vkDestroySwapchainKHR =
        (PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroySwapchainKHR");
    vkGetSwapchainImagesKHR =
        (PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(g_vulkan.instance, "vkGetSwapchainImagesKHR");
    vkCreateImageView = (PFN_vkCreateImageView)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateImageView");
    vkDestroyImageView = (PFN_vkDestroyImageView)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyImageView");
    vkCreateRenderPass = (PFN_vkCreateRenderPass)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateRenderPass");
    vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyRenderPass");
    vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateFramebuffer");
    vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyFramebuffer");
    vkCreateCommandPool = (PFN_vkCreateCommandPool)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateCommandPool");
    vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyCommandPool");
    vkAllocateCommandBuffers =
        (PFN_vkAllocateCommandBuffers)vkGetInstanceProcAddr(g_vulkan.instance, "vkAllocateCommandBuffers");
    vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetInstanceProcAddr(g_vulkan.instance, "vkBeginCommandBuffer");
    vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetInstanceProcAddr(g_vulkan.instance, "vkEndCommandBuffer");
    vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetInstanceProcAddr(g_vulkan.instance, "vkCmdBeginRenderPass");
    vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetInstanceProcAddr(g_vulkan.instance, "vkCmdEndRenderPass");
    vkCreateSemaphore = (PFN_vkCreateSemaphore)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateSemaphore");
    vkDestroySemaphore = (PFN_vkDestroySemaphore)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroySemaphore");
    vkCreateFence = (PFN_vkCreateFence)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateFence");
    vkDestroyFence = (PFN_vkDestroyFence)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyFence");
    vkWaitForFences = (PFN_vkWaitForFences)vkGetInstanceProcAddr(g_vulkan.instance, "vkWaitForFences");
    vkResetFences = (PFN_vkResetFences)vkGetInstanceProcAddr(g_vulkan.instance, "vkResetFences");
    vkAcquireNextImageKHR =
        (PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(g_vulkan.instance, "vkAcquireNextImageKHR");
    vkQueueSubmit = (PFN_vkQueueSubmit)vkGetInstanceProcAddr(g_vulkan.instance, "vkQueueSubmit");
    vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(g_vulkan.instance, "vkQueuePresentKHR");

    // Pipeline functions
    vkCreateGraphicsPipelines =
        (PFN_vkCreateGraphicsPipelines)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateGraphicsPipelines");
    vkDestroyPipeline = (PFN_vkDestroyPipeline)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyPipeline");
    vkCreatePipelineLayout =
        (PFN_vkCreatePipelineLayout)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreatePipelineLayout");
    vkDestroyPipelineLayout =
        (PFN_vkDestroyPipelineLayout)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyPipelineLayout");
    vkCreateShaderModule = (PFN_vkCreateShaderModule)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateShaderModule");
    vkDestroyShaderModule =
        (PFN_vkDestroyShaderModule)vkGetInstanceProcAddr(g_vulkan.instance, "vkDestroyShaderModule");
    vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetInstanceProcAddr(g_vulkan.instance, "vkCmdBindPipeline");
}

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        // Print warning or error
        printf("Vulkan validation layer: %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

// Helper functions
static bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    for (const auto& layerProperties : availableLayers)
    {
        if (strcmp(validationLayer, layerProperties.layerName) == 0)
            return true;
    }
    return false;
}

static std::vector<const char*> GetRequiredExtensions()
{
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

#ifdef _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

static void CreateInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "NoZ Game Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoZ";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef _DEBUG
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    if (CheckValidationLayerSupport())
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }
#endif

    VkResult result = vkCreateInstance(&createInfo, nullptr, &g_vulkan.instance);
    if (result != VK_SUCCESS)
        Exit("Failed to create Vulkan instance");

    // Load instance-specific functions
    LoadInstanceFunctions(g_vulkan.instance);
}

static void SetupDebugMessenger()
{
#ifdef _DEBUG
    auto func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

    func(g_vulkan.instance, &createInfo, nullptr, &g_vulkan.debug_messenger);
#endif
}

static void CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)g_vulkan.window;
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult result = vkCreateWin32SurfaceKHR(g_vulkan.instance, &createInfo, nullptr, &g_vulkan.surface);
    if (result != VK_SUCCESS)
        Exit("Failed to create window surface");
}

struct QueueFamilyIndices
{
    uint32_t graphics_family = UINT32_MAX;
    uint32_t present_family = UINT32_MAX;

    bool IsComplete() const
    {
        return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
    }
};

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics_family = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_vulkan.surface, &presentSupport);
        if (presentSupport)
            indices.present_family = i;

        if (indices.IsComplete())
            break;
    }

    return indices;
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    const char* requiredExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    for (const auto& extension : availableExtensions)
    {
        if (strcmp(extension.extensionName, requiredExtension) == 0)
            return true;
    }
    return false;
}

// Forward declaration for swapchain support
struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
static void RecreateSwapchainObjects();

static bool IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapchainAdequate = false;
    if (extensionsSupported)
    {
        SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.present_modes.empty();
    }

    return indices.IsComplete() && extensionsSupported && swapchainAdequate;
}

static void PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        Exit("Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            g_vulkan.physical_device = device;
            break;
        }
    }

    if (g_vulkan.physical_device == VK_NULL_HANDLE)
        Exit("Failed to find a suitable GPU");
}

static void CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<uint32_t> uniqueQueueFamilies = {indices.graphics_family, indices.present_family};

    // Remove duplicates
    if (indices.graphics_family == indices.present_family)
        uniqueQueueFamilies.resize(1);

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    const char* deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = &deviceExtension;

    VkResult result = vkCreateDevice(g_vulkan.physical_device, &createInfo, nullptr, &g_vulkan.device);
    if (result != VK_SUCCESS)
        Exit("Failed to create logical device");

    // Load device-specific functions
    LoadDeviceFunctions(g_vulkan.device);

    vkGetDeviceQueue(g_vulkan.device, indices.graphics_family, 0, &g_vulkan.graphics_queue);
    vkGetDeviceQueue(g_vulkan.device, indices.present_family, 0, &g_vulkan.present_queue);
}

static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_vulkan.surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount,
                                                  details.present_modes.data());
    }

    return details;
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }
    return availableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        Vec2Int screen_size = platform::GetWindowSize(g_vulkan.window);
        VkExtent2D actualExtent = {static_cast<uint32_t>(screen_size.x), static_cast<uint32_t>(screen_size.y)};

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

static void CreateSwapchain()
{
    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(g_vulkan.physical_device);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = g_vulkan.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);
    uint32_t queueFamilyIndices[] = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family != indices.present_family)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain);
    if (result != VK_SUCCESS)
        Exit("Failed to create swap chain");

    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, nullptr);
    g_vulkan.swapchain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, g_vulkan.swapchain_images.data());

    g_vulkan.swapchain_image_format = surfaceFormat.format;
    g_vulkan.swapchain_extent = extent;
}

static void CreateImageViews()
{
    g_vulkan.swapchain_image_views.resize(g_vulkan.swapchain_images.size());

    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = g_vulkan.swapchain_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = g_vulkan.swapchain_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain_image_views[i]);
        if (result != VK_SUCCESS)
            Exit("Failed to create image views");
    }
}

static void CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = g_vulkan.swapchain_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

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
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(g_vulkan.device, &renderPassInfo, nullptr, &g_vulkan.render_pass);
    if (result != VK_SUCCESS)
        Exit("Failed to create render pass");
}

static void CreateFramebuffers()
{
    g_vulkan.swapchain_framebuffers.resize(g_vulkan.swapchain_image_views.size());

    for (size_t i = 0; i < g_vulkan.swapchain_image_views.size(); i++)
    {
        VkImageView attachments[] = {g_vulkan.swapchain_image_views[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = g_vulkan.render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = g_vulkan.swapchain_extent.width;
        framebufferInfo.height = g_vulkan.swapchain_extent.height;
        framebufferInfo.layers = 1;

        VkResult result =
            vkCreateFramebuffer(g_vulkan.device, &framebufferInfo, nullptr, &g_vulkan.swapchain_framebuffers[i]);
        if (result != VK_SUCCESS)
            Exit("Failed to create framebuffer");
    }
}

static void CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(g_vulkan.physical_device);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics_family;

    VkResult result = vkCreateCommandPool(g_vulkan.device, &poolInfo, nullptr, &g_vulkan.command_pool);
    if (result != VK_SUCCESS)
        Exit("Failed to create command pool");
}

static void CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = g_vulkan.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(g_vulkan.device, &allocInfo, &g_vulkan.command_buffer);
    if (result != VK_SUCCESS)
        Exit("Failed to allocate command buffers");
}

static void CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateSemaphore(g_vulkan.device, &semaphoreInfo, nullptr, &g_vulkan.image_available_semaphore);
    if (result != VK_SUCCESS)
        Exit("Failed to create image available semaphore");

    result = vkCreateSemaphore(g_vulkan.device, &semaphoreInfo, nullptr, &g_vulkan.render_finished_semaphore);
    if (result != VK_SUCCESS)
        Exit("Failed to create render finished semaphore");

    result = vkCreateFence(g_vulkan.device, &fenceInfo, nullptr, &g_vulkan.in_flight_fence);
    if (result != VK_SUCCESS)
        Exit("Failed to create in flight fence");
}

void InitVulkan(const RendererTraits* traits, platform::Window* window)
{
    g_vulkan.traits = *traits;
    g_vulkan.window = window;

    // Load Vulkan library dynamically
    if (!LoadVulkanLibrary())
    {
        Exit("Failed to load Vulkan library. Make sure Vulkan drivers are installed.");
        return;
    }

    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();

    printf("Vulkan renderer initialized successfully\n");
}

void ShutdownVulkan()
{
    if (g_vulkan.device)
    {
        vkDeviceWaitIdle(g_vulkan.device);

        // Cleanup sync objects
        vkDestroySemaphore(g_vulkan.device, g_vulkan.render_finished_semaphore, nullptr);
        vkDestroySemaphore(g_vulkan.device, g_vulkan.image_available_semaphore, nullptr);
        vkDestroyFence(g_vulkan.device, g_vulkan.in_flight_fence, nullptr);

        // Cleanup command pool
        vkDestroyCommandPool(g_vulkan.device, g_vulkan.command_pool, nullptr);

        // Cleanup framebuffers
        for (auto framebuffer : g_vulkan.swapchain_framebuffers)
        {
            vkDestroyFramebuffer(g_vulkan.device, framebuffer, nullptr);
        }

        // Cleanup render pass
        vkDestroyRenderPass(g_vulkan.device, g_vulkan.render_pass, nullptr);

        // Cleanup swapchain
        for (auto imageView : g_vulkan.swapchain_image_views)
        {
            vkDestroyImageView(g_vulkan.device, imageView, nullptr);
        }

        if (g_vulkan.swapchain)
            vkDestroySwapchainKHR(g_vulkan.device, g_vulkan.swapchain, nullptr);

        vkDestroyDevice(g_vulkan.device, nullptr);
    }

    // Cleanup resources
    if (g_vulkan.surface)
        vkDestroySurfaceKHR(g_vulkan.instance, g_vulkan.surface, nullptr);

#ifdef _DEBUG
    if (g_vulkan.debug_messenger)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vulkan.instance,
                                                                               "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(g_vulkan.instance, g_vulkan.debug_messenger, nullptr);
    }
#endif

    if (g_vulkan.instance)
        vkDestroyInstance(g_vulkan.instance, nullptr);

    // Free Vulkan library
    if (vulkan_library)
    {
        FreeLibrary(vulkan_library);
        vulkan_library = nullptr;
    }
}

// Render frame functions
void VulkanBeginRenderFrame(Color clear_color)
{
    vkWaitForFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence);

    VkResult result = vkAcquireNextImageKHR(g_vulkan.device, g_vulkan.swapchain, UINT64_MAX,
                                            g_vulkan.image_available_semaphore, VK_NULL_HANDLE, &g_current_image_index);

    // Handle swapchain out of date (window was resized while rendering)
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapchainObjects();
        result = vkAcquireNextImageKHR(g_vulkan.device, g_vulkan.swapchain, UINT64_MAX,
                                       g_vulkan.image_available_semaphore, VK_NULL_HANDLE, &g_current_image_index);
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(g_vulkan.command_buffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = g_vulkan.render_pass;
    renderPassInfo.framebuffer = g_vulkan.swapchain_framebuffers[g_current_image_index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = g_vulkan.swapchain_extent;

    VkClearValue clearColor = {{{clear_color.r, clear_color.g, clear_color.b, clear_color.a}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanEndRenderFrame()
{
    vkCmdEndRenderPass(g_vulkan.command_buffer);
    vkEndCommandBuffer(g_vulkan.command_buffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {g_vulkan.image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_vulkan.command_buffer;

    VkSemaphore signalSemaphores[] = {g_vulkan.render_finished_semaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(g_vulkan.graphics_queue, 1, &submitInfo, g_vulkan.in_flight_fence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {g_vulkan.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &g_current_image_index;

    VkResult result = vkQueuePresentKHR(g_vulkan.present_queue, &presentInfo);

    // Handle swapchain out of date or suboptimal
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Swapchain needs to be recreated, but we'll handle this on the next frame
        // to avoid doing it during present
    }
}

void RecreateSwapchainObjects()
{
    // Safety check - ensure we have valid Vulkan objects
    if (!g_vulkan.device || !g_vulkan.swapchain)
        return;

    // Clean up existing swapchain-dependent objects
    for (auto framebuffer : g_vulkan.swapchain_framebuffers)
    {
        vkDestroyFramebuffer(g_vulkan.device, framebuffer, nullptr);
    }
    for (auto image_view : g_vulkan.swapchain_image_views)
    {
        vkDestroyImageView(g_vulkan.device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(g_vulkan.device, g_vulkan.swapchain, nullptr);

    // Get new window size
    Vec2Int screen_size = platform::GetWindowSize(g_vulkan.window);

    // Recreate swapchain with new dimensions
    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(g_vulkan.physical_device);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = g_vulkan.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);
    uint32_t queueFamilyIndices[] = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family != indices.present_family)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain);

    // Update stored extent and format
    g_vulkan.swapchain_extent = extent;
    g_vulkan.swapchain_image_format = surfaceFormat.format;

    // Get new swapchain images
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, nullptr);
    g_vulkan.swapchain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, g_vulkan.swapchain_images.data());

    // Recreate image views
    g_vulkan.swapchain_image_views.resize(g_vulkan.swapchain_images.size());
    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = g_vulkan.swapchain_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = g_vulkan.swapchain_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain_image_views[i]);
    }

    // Recreate framebuffers
    g_vulkan.swapchain_framebuffers.resize(g_vulkan.swapchain_image_views.size());
    for (size_t i = 0; i < g_vulkan.swapchain_image_views.size(); i++)
    {
        VkImageView attachments[] = {g_vulkan.swapchain_image_views[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = g_vulkan.render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = g_vulkan.swapchain_extent.width;
        framebufferInfo.height = g_vulkan.swapchain_extent.height;
        framebufferInfo.layers = 1;

        vkCreateFramebuffer(g_vulkan.device, &framebufferInfo, nullptr, &g_vulkan.swapchain_framebuffers[i]);
    }
}

// Function to handle window resize
void ResizeVulkan()
{
    // Safety checks - don't recreate swapchain if Vulkan is not initialized or window is closing
    if (!g_vulkan.device || !g_vulkan.window || !g_vulkan.swapchain)
        return;

    // Check if window still exists and has valid size
    Vec2Int window_size = platform::GetWindowSize(g_vulkan.window);
    if (window_size.x <= 0 || window_size.y <= 0)
        return;

    // Wait for device to be idle before recreating swapchain
    vkDeviceWaitIdle(g_vulkan.device);

    // Recreate all swapchain-dependent objects with new dimensions
    RecreateSwapchainObjects();
}

// Pipeline factory support functions
VkDevice GetVulkanDevice()
{
    return g_vulkan.device;
}

VkRenderPass GetVulkanRenderPass()
{
    return g_vulkan.render_pass;
}

platform::Pipeline* platform::CreatePipeline(Shader* shader, bool msaa, bool shadow)
{
    if (!g_vulkan.device)
        return nullptr;

    // TODO: Get actual shader modules from shader object
    // For now, this is a placeholder that will need shader integration

    // Create basic pipeline layout (no uniforms for now)
    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 0;
    layout_info.pSetLayouts = nullptr;
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout(g_vulkan.device, &layout_info, nullptr, &layout) != VK_SUCCESS)
        return nullptr;

    // Vertex input (basic for now - position, uv, normal, bone_index)
    VkVertexInputBindingDescription binding_desc = {};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(float) * 7; // 2 pos + 2 uv + 2 normal + 1 bone
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attr_descs[4] = {};
    // Position
    attr_descs[0].binding = 0;
    attr_descs[0].location = 0;
    attr_descs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attr_descs[0].offset = 0;
    // UV
    attr_descs[1].binding = 0;
    attr_descs[1].location = 1;
    attr_descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attr_descs[1].offset = sizeof(float) * 2;
    // Normal
    attr_descs[2].binding = 0;
    attr_descs[2].location = 2;
    attr_descs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attr_descs[2].offset = sizeof(float) * 4;
    // Bone index
    attr_descs[3].binding = 0;
    attr_descs[3].location = 3;
    attr_descs[3].format = VK_FORMAT_R32_SFLOAT;
    attr_descs[3].offset = sizeof(float) * 6;

    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding_desc;
    vertex_input.vertexAttributeDescriptionCount = 4;
    vertex_input.pVertexAttributeDescriptions = attr_descs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // TODO: Get from shader
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaa ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;  // TODO: Get from shader
    depth_stencil.depthWriteEnable = VK_TRUE; // TODO: Get from shader
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE; // TODO: Get from shader

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = shadow ? 0 : 1;
    color_blending.pAttachments = shadow ? nullptr : &color_blend_attachment;

    // Dynamic state
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    // TODO: Shader stages will be added when shader system is updated
    VkPipelineShaderStageCreateInfo shader_stages[2] = {};
    // Placeholder - these will be populated from actual shader data

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 0; // TODO: Set to 2 when shaders are ready
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = layout;
    pipeline_info.renderPass = g_vulkan.render_pass;
    pipeline_info.subpass = 0;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(g_vulkan.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
    {
        vkDestroyPipelineLayout(g_vulkan.device, layout, nullptr);
        return nullptr;
    }

    // Return the VkPipeline cast as PlatformPipeline (opaque handle)
    return (Pipeline*)pipeline;
}

void platform::DestroyPipeline(Pipeline* pipeline)
{
    if (pipeline && g_vulkan.device)
    {
        // Cast back to VkPipeline and destroy
        VkPipeline vk_pipeline = (VkPipeline)pipeline;
        vkDestroyPipeline(g_vulkan.device, vk_pipeline, nullptr);
        // TODO: Also destroy associated layout when we store it
    }
}

void platform::BindPipeline(Pipeline* pipeline)
{
    if (pipeline && g_vulkan.command_buffer)
    {
        // Cast back to VkPipeline and bind
        VkPipeline vk_pipeline = (VkPipeline)pipeline;
        vkCmdBindPipeline(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
    }
}
