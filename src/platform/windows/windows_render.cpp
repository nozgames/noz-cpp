//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include "windows_vulkan.h"

enum UniformBufferType
{
    UNIFORM_BUFFER_CAMERA,
    UNIFORM_BUFFER_TRANSFORM ,
    UNIFORM_BUFFER_COLOR,
    UNIFORM_BUFFER_COUNT
};

constexpr int MAX_TEXTURES = 128;
constexpr int VK_SPACE_TEXTURE = UNIFORM_BUFFER_COUNT;

constexpr int MAX_UNIFORM_BUFFERS = 4096;
constexpr u32 UNIFORM_BUFFER_SIZE = sizeof(Mat4);
constexpr u32 DYNAMIC_UNIFORM_BUFFER_SIZE = UNIFORM_BUFFER_SIZE * MAX_UNIFORM_BUFFERS;

static VkFilter ToVK(TextureFilter filter)
{
    return filter == TEXTURE_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

static VkSamplerAddressMode ToVK(TextureClamp clamp)
{
    return clamp == TEXTURE_CLAMP_REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}

struct platform::Texture
{
    VkImage vk_image;
    VkImageView vk_image_view;
    VkDeviceMemory vk_memory;
    VkSampler vk_sampler;
    VkDescriptorSet vk_descriptor_set;
    SamplerOptions sampler_options;
    Vec2Int size;
    i32 channels;
};

struct platform::Shader
{
    VkShaderModule vertex_module;
    VkShaderModule geometry_module;
    VkShaderModule fragment_module;
    VkPipeline pipeline;
    PipelineLayout* platform_layout;
};

struct VulkanDynamicBuffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped_ptr;
    u32 offset;
    u32 alignment;
    u32 size;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;
};

struct VulkanRenderer
{
    RendererTraits traits;
    HWND hwnd;
    HMODULE library;
    VkInstance instance;
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
    VkSampleCountFlagBits msaa_samples;
    VkImage msaa_color_image;
    VkDeviceMemory msaa_color_image_memory;
    VkImageView msaa_color_image_view;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> swapchain_framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    VkDescriptorSetLayout texture_descriptor_set_layout;
    VkDescriptorSet texture_descriptor_set;
    VkDescriptorPool descriptor_pool;
    VulkanDynamicBuffer uniform_buffers[UNIFORM_BUFFER_COUNT];  // Now consecutive 0,1,2
    u32 min_uniform_buffer_offset_alignment;
    VkPipelineLayout pipeline_layout;
    u32 current_image_index;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

static VulkanRenderer g_vulkan = {};

static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
static void RecreateSwapchainObjects();
static VkSampleCountFlagBits GetMaxUsableSampleCount();
static void CreateColorResources();
static void CleanupColorResources();

static void SetVulkanObjectName(VkObjectType object_type, uint64_t object_handle, const char* name)
{
    if (!name || !vkSetDebugUtilsObjectNameEXT)
        return;

    assert(g_vulkan.device);

    VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = object_type,
        .objectHandle = object_handle,
        .pObjectName = name,
    };
    vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
}

u32 FindMemoryType(u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(g_vulkan.physical_device, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    for (u32 i = 0; i < 32; i++)
        if (type_filter & 1 << i)
            return i;

    return 0;
}

static bool CreateUniformBuffer(VulkanDynamicBuffer* buffer, const char* name)
{
    buffer->size = DYNAMIC_UNIFORM_BUFFER_SIZE;
    buffer->alignment = g_vulkan.min_uniform_buffer_offset_alignment;
    buffer->offset = 0;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer->size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (VK_SUCCESS != vkCreateBuffer(g_vulkan.device, &buffer_info, nullptr, &buffer->buffer))
        return false;

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, buffer->buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(
            mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (VK_SUCCESS != vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &buffer->memory))
    {
        vkDestroyBuffer(g_vulkan.device, buffer->buffer, nullptr);
        return false;
    }

    vkBindBufferMemory(g_vulkan.device, buffer->buffer, buffer->memory, 0);

    if (VK_SUCCESS != vkMapMemory(g_vulkan.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped_ptr))
    {
        vkFreeMemory(g_vulkan.device, buffer->memory, nullptr);
        vkDestroyBuffer(g_vulkan.device, buffer->buffer, nullptr);
        return false;
    }

    SetVulkanObjectName(VK_OBJECT_TYPE_BUFFER, (u64)buffer->buffer, name);

    return true;
}

static void* AcquireUniformBuffer(UniformBufferType type)
{
    assert(type >= 0 && type < UNIFORM_BUFFER_COUNT);

    u32 aligned_size = (UNIFORM_BUFFER_SIZE + g_vulkan.min_uniform_buffer_offset_alignment - 1) 
                       & ~(g_vulkan.min_uniform_buffer_offset_alignment - 1);
    
    VulkanDynamicBuffer* buffer = &g_vulkan.uniform_buffers[type];
    
    // Check if we have space
    if (buffer->offset + aligned_size > buffer->size)
        return nullptr;

    u32 current_offset = buffer->offset;
    void* ptr = (char*)buffer->mapped_ptr + current_offset;
    buffer->offset += aligned_size;
    
    // Uniform buffer enum values now match descriptor spaces directly
    u32 descriptor_space = (u32)type;
    
    vkCmdBindDescriptorSets(
        g_vulkan.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        g_vulkan.pipeline_layout,
        descriptor_space,
        1,
        &buffer->descriptor_set,
        1,
        &current_offset
    );
    
    return ptr;
}


static void ResetFrameBuffers()
{
    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        g_vulkan.uniform_buffers[i].offset = 0;
    }
}

static void CreateDescriptorSetLayout()
{
    // Create descriptor set layouts for each uniform buffer type
    VkDescriptorSetLayoutBinding uniform_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = 0,  // Will be set per buffer type
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uniform_binding
    };

    // Camera buffer layout (vertex stage)
    uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &layout_info, nullptr, &g_vulkan.uniform_buffers[UNIFORM_BUFFER_CAMERA].descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create camera descriptor set layout");

    // Transform buffer layout (vertex stage)
    uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &layout_info, nullptr, &g_vulkan.uniform_buffers[UNIFORM_BUFFER_TRANSFORM].descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create transform descriptor set layout");

    // Color buffer layout (fragment stage)
    uniform_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &layout_info, nullptr, &g_vulkan.uniform_buffers[UNIFORM_BUFFER_COLOR].descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create color descriptor set layout");

    // texture
    VkDescriptorSetLayoutBinding texture_bindings[] = {
        // Texture0
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo texture_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(texture_bindings) / sizeof(VkDescriptorSetLayoutBinding),
        .pBindings = texture_bindings
    };
    
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &texture_layout_info, nullptr, &g_vulkan.texture_descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create texture descriptor set layout");

}

static void CreateDescriptorPool()
{
    VkDescriptorPoolSize pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 3  // camera + transform + color
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_TEXTURES
        }
    };
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = 3 + MAX_TEXTURES;  // vertex + texture + fragment + textures
    
    if (vkCreateDescriptorPool(g_vulkan.device, &pool_info, nullptr, &g_vulkan.descriptor_pool) != VK_SUCCESS)
        Exit("Failed to create descriptor pool");
}

enum DescriptorSpace
{
    DESCRIPTOR_SPACE_FRAGMENT
};


static void CreateDescriptorSets()
{
    // Create individual descriptor sets for each uniform buffer
    const char* buffer_names[] = {"CameraBuffer", "TransformBuffer", "ColorBuffer"};
    
    // Allocate descriptor sets for uniform buffers
    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_vulkan.descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &g_vulkan.uniform_buffers[i].descriptor_set_layout,
        };

        if (vkAllocateDescriptorSets(g_vulkan.device, &alloc_info, &g_vulkan.uniform_buffers[i].descriptor_set) != VK_SUCCESS)
            Exit("Failed to allocate uniform buffer descriptor set");

        // Update the descriptor set with the buffer
        VkDescriptorBufferInfo buffer_info = {
            .buffer = g_vulkan.uniform_buffers[i].buffer,
            .offset = 0,
            .range = UNIFORM_BUFFER_SIZE
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vulkan.uniform_buffers[i].descriptor_set,
            .dstBinding = 0,  // All uniform buffers use binding 0 in their own descriptor set
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(g_vulkan.device, 1, &write, 0, nullptr);
        SetVulkanObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)g_vulkan.uniform_buffers[i].descriptor_set, buffer_names[i]);
    }

    // Create texture descriptor set separately
    VkDescriptorSetAllocateInfo texture_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vulkan.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.texture_descriptor_set_layout,
    };

    if (vkAllocateDescriptorSets(g_vulkan.device, &texture_alloc_info, &g_vulkan.texture_descriptor_set) != VK_SUCCESS)
        Exit("Failed to allocate texture descriptor set");

    SetVulkanObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)g_vulkan.texture_descriptor_set, "Textures");
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        printf("Vulkan validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static bool CheckValidationLayerSupport()
{
    u32 layerCount;
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
    auto extensions = GetRequiredExtensions();

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "NoZ Game Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "NoZ",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = static_cast<u32>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

#ifdef _DEBUG
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    if (CheckValidationLayerSupport())
    {
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &validationLayer;
    }
#endif

    VkResult result = vkCreateInstance(&create_info, nullptr, &g_vulkan.instance);
    if (result != VK_SUCCESS)
        Exit("Failed to create Vulkan instance");

    LoadInstanceFunctions(g_vulkan.instance);
}

static void SetupDebugMessenger()
{
#ifdef _DEBUG
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = DebugCallback
    };

    func(g_vulkan.instance, &createInfo, nullptr, &g_vulkan.debug_messenger);
#endif
}

static void CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = g_vulkan.hwnd,
    };

    VkResult result = vkCreateWin32SurfaceKHR(g_vulkan.instance, &createInfo, nullptr, &g_vulkan.surface);
    if (result != VK_SUCCESS)
        Exit("Failed to create window surface");
}

struct QueueFamilyIndices
{
    u32 graphics_family = UINT32_MAX;
    u32 present_family = UINT32_MAX;

    bool IsComplete() const
    {
        return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
    }
};

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (u32 i = 0; i < queueFamilies.size(); i++)
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
    u32 extensionCount;
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
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        Exit("Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, devices.data());

    for (const auto& device : devices)
        if (IsDeviceSuitable(device))
        {
            g_vulkan.physical_device = device;
            break;
        }

    if (g_vulkan.physical_device == VK_NULL_HANDLE)
        Exit("Failed to find a suitable GPU");

    g_vulkan.msaa_samples = g_vulkan.traits.msaa ? GetMaxUsableSampleCount() : VK_SAMPLE_COUNT_1_BIT;
}

static void CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector uniqueQueueFamilies = {indices.graphics_family, indices.present_family};

    // Remove duplicates
    if (indices.graphics_family == indices.present_family)
        uniqueQueueFamilies.resize(1);

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    const char* deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &deviceExtension,
        .pEnabledFeatures = &deviceFeatures,
    };

    VkResult result = vkCreateDevice(g_vulkan.physical_device, &createInfo, nullptr, &g_vulkan.device);
    if (result != VK_SUCCESS)
        Exit("Failed to create logical device");

    LoadDeviceFunctions(g_vulkan.instance);

    vkGetDeviceQueue(g_vulkan.device, indices.graphics_family, 0, &g_vulkan.graphics_queue);
    vkGetDeviceQueue(g_vulkan.device, indices.present_family, 0, &g_vulkan.present_queue);
}

static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_vulkan.surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount, details.present_modes.data());
    }

    return details;
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // Force VSync by always returning FIFO mode (guaranteed to be available)
    return g_vulkan.traits.vsync == 0 ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;

    Vec2Int screen_size = platform::GetScreenSize();
    VkExtent2D actualExtent = {static_cast<u32>(screen_size.x), static_cast<u32>(screen_size.y)};

    actualExtent.width = std::max(capabilities.minImageExtent.width,
                                  std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height,
                                   std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

static void CreateSwapchain()
{
    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(g_vulkan.physical_device);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapchainSupport.capabilities);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        imageCount = swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_vulkan.surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);
    u32 queueFamilyIndices[] = {indices.graphics_family, indices.present_family};

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
        VkImageViewCreateInfo vk_image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = g_vulkan.swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = g_vulkan.swapchain_image_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        if (VK_SUCCESS != vkCreateImageView(g_vulkan.device, &vk_image_create_info, nullptr, &g_vulkan.swapchain_image_views[i]))
            Exit("Failed to create image views");
        
        // Set debug names
        char image_name[32];
        snprintf(image_name, sizeof(image_name), "Swapchain%zu", i);
        SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)g_vulkan.swapchain_images[i], image_name);
        SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)g_vulkan.swapchain_image_views[i], image_name);
    }
}

static void CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = g_vulkan.swapchain_image_format;
    colorAttachment.samples = g_vulkan.msaa_samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = g_vulkan.swapchain_image_format;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 1;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
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
        VkImageView vk_attachments[] = {g_vulkan.msaa_color_image_view, g_vulkan.swapchain_image_views[i]};

        VkFramebufferCreateInfo vk_frame_buffer_info = {};
        vk_frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vk_frame_buffer_info.renderPass = g_vulkan.render_pass;
        vk_frame_buffer_info.attachmentCount = 2;
        vk_frame_buffer_info.pAttachments = vk_attachments;
        vk_frame_buffer_info.width = g_vulkan.swapchain_extent.width;
        vk_frame_buffer_info.height = g_vulkan.swapchain_extent.height;
        vk_frame_buffer_info.layers = 1;

        VkResult result =
            vkCreateFramebuffer(g_vulkan.device, &vk_frame_buffer_info, nullptr, &g_vulkan.swapchain_framebuffers[i]);
        if (result != VK_SUCCESS)
            Exit("Failed to create framebuffer");
    }
}

static void CreateCommandPool()
{
    QueueFamilyIndices vk_family_indices = FindQueueFamilies(g_vulkan.physical_device);

    VkCommandPoolCreateInfo vf_pool_info = {};
    vf_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vf_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vf_pool_info.queueFamilyIndex = vk_family_indices.graphics_family;

    VkResult result = vkCreateCommandPool(g_vulkan.device, &vf_pool_info, nullptr, &g_vulkan.command_pool);
    if (result != VK_SUCCESS)
        Exit("Failed to create command pool");
}

static void CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo vf_alloc_info = {};
    vf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vf_alloc_info.commandPool = g_vulkan.command_pool;
    vf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vf_alloc_info.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(g_vulkan.device, &vf_alloc_info, &g_vulkan.command_buffer);
    if (result != VK_SUCCESS)
        Exit("Failed to allocate command buffers");
}

static void CreateSyncObjects()
{
    VkFenceCreateInfo vk_fence_info = {};
    vk_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vk_fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo vk_semaphore_info = {};
    vk_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkResult result = vkCreateSemaphore(g_vulkan.device, &vk_semaphore_info, nullptr, &g_vulkan.image_available_semaphore);
    if (result != VK_SUCCESS)
        Exit("Failed to create image available semaphore");

    result = vkCreateSemaphore(g_vulkan.device, &vk_semaphore_info, nullptr, &g_vulkan.render_finished_semaphore);
    if (result != VK_SUCCESS)
        Exit("Failed to create render finished semaphore");

    result = vkCreateFence(g_vulkan.device, &vk_fence_info, nullptr, &g_vulkan.in_flight_fence);
    if (result != VK_SUCCESS)
        Exit("Failed to create in flight fence");
}

static void CreatePipelineLayout()
{
    VkDescriptorSetLayout layouts[] = {
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_CAMERA].descriptor_set_layout,     // Set 0: Camera
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_TRANSFORM].descriptor_set_layout,  // Set 1: Transform  
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_COLOR].descriptor_set_layout,      // Set 2: Color
        g_vulkan.texture_descriptor_set_layout                                      // Set 3: Texture
    };

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 4;
    layout_info.pSetLayouts = layouts;
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(g_vulkan.device, &layout_info, nullptr, &g_vulkan.pipeline_layout);
}

void RecreateSwapchainObjects()
{
    assert(g_vulkan.device);
    assert(g_vulkan.swapchain);

    for (auto framebuffer : g_vulkan.swapchain_framebuffers)
        vkDestroyFramebuffer(g_vulkan.device, framebuffer, nullptr);

    CleanupColorResources();

    for (auto image_view : g_vulkan.swapchain_image_views)
        vkDestroyImageView(g_vulkan.device, image_view, nullptr);

    vkDestroySwapchainKHR(g_vulkan.device, g_vulkan.swapchain, nullptr);

    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(g_vulkan.physical_device);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapchainSupport.capabilities);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        imageCount = swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_vulkan.surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapchainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);
    u32 queueFamilyIndices[] = {indices.graphics_family, indices.present_family};

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

    vkCreateSwapchainKHR(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain);

    // Update stored extent and format
    g_vulkan.swapchain_extent = extent;
    g_vulkan.swapchain_image_format = surfaceFormat.format;

    // Get new swapchain images
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, nullptr);
    g_vulkan.swapchain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, g_vulkan.swapchain_images.data());

    // image views
    g_vulkan.swapchain_image_views.resize(g_vulkan.swapchain_images.size());
    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++)
    {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = g_vulkan.swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = g_vulkan.swapchain_image_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        vkCreateImageView(g_vulkan.device, &create_info, nullptr, &g_vulkan.swapchain_image_views[i]);
        
        // Set debug names
        char image_name[32];
        snprintf(image_name, sizeof(image_name), "Swapchain%zu", i);
        SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)g_vulkan.swapchain_images[i], image_name);
        SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)g_vulkan.swapchain_image_views[i], image_name);
    }

    // Recreate color resources for MSAA
    CreateColorResources();

    // frame buffers
    g_vulkan.swapchain_framebuffers.resize(g_vulkan.swapchain_image_views.size());
    for (size_t i = 0; i < g_vulkan.swapchain_image_views.size(); i++)
    {
        VkImageView attachments[] = {g_vulkan.msaa_color_image_view, g_vulkan.swapchain_image_views[i]};
        VkFramebufferCreateInfo frame_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = g_vulkan.render_pass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .layers = 1
        };

        vkCreateFramebuffer(g_vulkan.device, &frame_buffer_info, nullptr, &g_vulkan.swapchain_framebuffers[i]);
    }
}

void ResizeVulkan(const Vec2Int& size)
{
    assert(g_vulkan.device);
    assert(g_vulkan.hwnd);
    assert(g_vulkan.swapchain);
    assert(size.x > 0);
    assert(size.y > 0);

    vkDeviceWaitIdle(g_vulkan.device);

    RecreateSwapchainObjects();
}

void platform::BeginRenderFrame()
{
    ResetFrameBuffers();

    vkWaitForFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence);

    VkResult result = vkAcquireNextImageKHR(
        g_vulkan.device,
        g_vulkan.swapchain,
        UINT64_MAX,
        g_vulkan.image_available_semaphore,
        VK_NULL_HANDLE,
        &g_vulkan.current_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        RecreateSwapchainObjects();
        result = vkAcquireNextImageKHR(
            g_vulkan.device,
            g_vulkan.swapchain,
            UINT64_MAX,
            g_vulkan.image_available_semaphore,
            VK_NULL_HANDLE,
            &g_vulkan.current_image_index);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            Exit("Failed to acquire swapchain image after recreation");
    }
    else if (result != VK_SUCCESS)
    {
        Exit("Failed to acquire swapchain image");
    }

    VkCommandBufferBeginInfo vg_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(g_vulkan.command_buffer, &vg_begin_info);

}

void platform::EndRenderFrame()
{
    // End the command buffer
    vkEndCommandBuffer(g_vulkan.command_buffer);

    // Submit queue
    VkSemaphore vk_wait_semaphores[] = {g_vulkan.image_available_semaphore};
    VkPipelineStageFlags vk_wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore vk_signal_semaphores[] = {g_vulkan.render_finished_semaphore};
    VkSubmitInfo vk_submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk_wait_semaphores,
        .pWaitDstStageMask = vk_wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &g_vulkan.command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = vk_signal_semaphores
    };

    vkQueueSubmit(g_vulkan.graphics_queue, 1, &vk_submit_info, g_vulkan.in_flight_fence);

    // Present
    VkSwapchainKHR vk_swap_chains[] = {g_vulkan.swapchain};
    VkPresentInfoKHR vk_parent_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = vk_signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = vk_swap_chains,
        .pImageIndices = &g_vulkan.current_image_index
    };

    vkQueuePresentKHR(g_vulkan.present_queue, &vk_parent_info);
}

static void CopyMat3ToGPU(void* dst, const Mat3& src)
{
    float* f = (float*)dst;
    f[0] = src.m[0]; f[1] = src.m[1]; f[2] = src.m[2]; f[3] = 0;
    f[4] = src.m[3]; f[5] = src.m[4]; f[6] = src.m[5]; f[7] = 0;
    f[8] = src.m[6]; f[9] = src.m[7]; f[10] = src.m[8]; f[11] = 0;
}

void platform::BindTransform(const Mat3& transform)
{
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_TRANSFORM);
    if (!buffer_ptr)
        return;

    CopyMat3ToGPU(buffer_ptr, transform);
}

void platform::BindCamera(const Mat3& view_matrix)
{
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_CAMERA);
    if (!buffer_ptr)
        return;

    CopyMat3ToGPU(buffer_ptr, view_matrix);
}

void platform::BindColor(const Color& color)
{
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_COLOR);
    if (!buffer_ptr)
        return;

    memcpy(buffer_ptr, &color, sizeof(Color));
}

platform::Buffer* platform::CreateVertexBuffer(const MeshVertex* vertices, size_t vertex_count, const char* name)
{
    assert(vertices);
    assert(vertex_count > 0);
    assert(g_vulkan.device != VK_NULL_HANDLE);

    VkBufferCreateInfo vk_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertex_count * sizeof(MeshVertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer vf_vertex_buffer = VK_NULL_HANDLE;
    if (VK_SUCCESS != vkCreateBuffer(g_vulkan.device, &vk_buffer_info, nullptr, &vf_vertex_buffer))
        return nullptr;
    
    VkMemoryRequirements vk_mem_reqs = {};
    vkGetBufferMemoryRequirements(g_vulkan.device, vf_vertex_buffer, &vk_mem_reqs);
    
    VkMemoryAllocateInfo vk_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = vk_mem_reqs.size,
        .memoryTypeIndex = FindMemoryType(
            vk_mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    VkDeviceMemory vk_vertex_memory = VK_NULL_HANDLE;
    if (VK_SUCCESS != vkAllocateMemory(g_vulkan.device, &vk_alloc_info, nullptr, &vk_vertex_memory))
    {
        vkDestroyBuffer(g_vulkan.device, vf_vertex_buffer, nullptr);
        return nullptr;
    }
    
    vkBindBufferMemory(g_vulkan.device, vf_vertex_buffer, vk_vertex_memory, 0);
    
    // Copy vertex data
    void* mapped_data;
    if (VK_SUCCESS == vkMapMemory(g_vulkan.device, vk_vertex_memory, 0, vk_buffer_info.size, 0, &mapped_data))
    {
        memcpy(mapped_data, vertices, vk_buffer_info.size);
        vkUnmapMemory(g_vulkan.device, vk_vertex_memory);
    }
    
    // Set debug name for RenderDoc/debugging tools
    SetVulkanObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)vf_vertex_buffer, name);
    
    return (Buffer*)vf_vertex_buffer;
}

platform::Buffer* platform::CreateIndexBuffer(const u16* indices, size_t index_count, const char* name)
{
    assert(indices);
    assert(index_count > 0);

    VkBufferCreateInfo vk_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = index_count * sizeof(u16),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VkBuffer index_buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(g_vulkan.device, &vk_buffer_info, nullptr, &index_buffer) != VK_SUCCESS)
        return nullptr;
    
    VkMemoryRequirements vk_mem_reqs = {};
    vkGetBufferMemoryRequirements(g_vulkan.device, index_buffer, &vk_mem_reqs);
    
    VkMemoryAllocateInfo vk_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = vk_mem_reqs.size,
        .memoryTypeIndex = FindMemoryType(
            vk_mem_reqs.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };
    
    VkDeviceMemory index_memory = VK_NULL_HANDLE;
    if (vkAllocateMemory(g_vulkan.device, &vk_alloc_info, nullptr, &index_memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(g_vulkan.device, index_buffer, nullptr);
        return nullptr;
    }
    
    vkBindBufferMemory(g_vulkan.device, index_buffer, index_memory, 0);
    
    void* data;
    if (vkMapMemory(g_vulkan.device, index_memory, 0, vk_buffer_info.size, 0, &data) == VK_SUCCESS)
    {
        memcpy(data, indices, vk_buffer_info.size);
        vkUnmapMemory(g_vulkan.device, index_memory);
    }
    
    SetVulkanObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)index_buffer, name);
    
    return (Buffer*)index_buffer;
}

void platform::BindVertexBuffer(Buffer* buffer)
{
    assert(buffer);
    assert(g_vulkan.command_buffer);
    VkBuffer vertex_buffers[] = {(VkBuffer)buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(g_vulkan.command_buffer, 0, 1, vertex_buffers, offsets);
}

void platform::BindIndexBuffer(Buffer* buffer)
{
    assert(buffer);
    assert(g_vulkan.command_buffer);
    vkCmdBindIndexBuffer(g_vulkan.command_buffer, (VkBuffer)buffer, 0, VK_INDEX_TYPE_UINT16);
}

void platform::DrawIndexed(size_t index_count)
{
    assert(g_vulkan.command_buffer);
    assert(index_count > 0);
    vkCmdDrawIndexed(g_vulkan.command_buffer, index_count, 1, 0, 0, 0);
}

static bool CreateTextureInternal(platform::Texture* texture, void* data, const SamplerOptions& sampler_options, const char* name)
{
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = (u32)(texture->size.x * texture->size.y * texture->channels),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (VK_SUCCESS != vkCreateBuffer(g_vulkan.device, &buffer_info, nullptr, &staging_buffer))
        return false;

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, staging_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(
            mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (VK_SUCCESS != vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &staging_buffer_memory))
        return false;

    vkBindBufferMemory(g_vulkan.device, staging_buffer, staging_buffer_memory, 0);

    if (data)
    {
        void* mapped_data;
        vkMapMemory(g_vulkan.device, staging_buffer_memory, 0, buffer_info.size, 0, &mapped_data);
        memcpy(mapped_data, data, buffer_info.size);
        vkUnmapMemory(g_vulkan.device, staging_buffer_memory);
    }

    VkImageCreateInfo vk_image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (texture->channels == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width = (u32)texture->size.x,
            .height = (u32)texture->size.y,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (VK_SUCCESS != vkCreateImage(g_vulkan.device, &vk_image_create_info, nullptr, &texture->vk_image))
        return false;

    // Allocate memory for image
    VkMemoryRequirements image_mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, texture->vk_image, &image_mem_requirements);

    VkMemoryAllocateInfo image_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = image_mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    if (VK_SUCCESS != vkAllocateMemory(g_vulkan.device, &image_alloc_info, nullptr, &texture->vk_memory))
        return false;

    vkBindImageMemory(g_vulkan.device, texture->vk_image, texture->vk_memory, 0);

    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = g_vulkan.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(g_vulkan.device, &cmd_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture->vk_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {(u32)texture->size.x, (u32)texture->size.y, 1},
    };

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to shader read optimal layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    };

    vkQueueSubmit(g_vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_vulkan.graphics_queue);
    vkFreeMemory(g_vulkan.device, staging_buffer_memory, nullptr);
    vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = texture->vk_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = (texture->channels == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    if (VK_SUCCESS != vkCreateImageView(g_vulkan.device, &view_info, nullptr, &texture->vk_image_view))
        return false;

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = ToVK(sampler_options.filter),
        .minFilter = ToVK(sampler_options.filter),
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = ToVK(sampler_options.clamp),
        .addressModeV = ToVK(sampler_options.clamp),
        .addressModeW = ToVK(sampler_options.clamp),
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (VK_SUCCESS != vkCreateSampler(g_vulkan.device, &sampler_info, nullptr, &texture->vk_sampler))
        return false;

    // Descriptor set
    VkDescriptorSetAllocateInfo desc_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vulkan.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.texture_descriptor_set_layout
    };

    if (vkAllocateDescriptorSets(g_vulkan.device, &desc_alloc_info, &texture->vk_descriptor_set) != VK_SUCCESS)
        return false;

    VkDescriptorImageInfo vk_descriptor_image_info = {
        .sampler = texture->vk_sampler,
        .imageView = texture->vk_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet vk_write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = texture->vk_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &vk_descriptor_image_info
    };
    vkUpdateDescriptorSets(g_vulkan.device, 1, &vk_write_descriptor_set, 0, nullptr);

    SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)texture->vk_image, name);
    SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)texture->vk_image_view, name);
    SetVulkanObjectName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)texture->vk_sampler, name);
    SetVulkanObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)texture->vk_descriptor_set, name);

    return true;
}

platform::Texture* platform::CreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name)
{
    Texture* texture = new Texture{};
    texture->size = {(int)width, (int)height};
    texture->channels = channels;
    if (!CreateTextureInternal(texture, data, sampler_options, name))
    {
        DestroyTexture(texture);
        return nullptr;
    }

    return texture;
}

void platform::DestroyTexture(Texture* texture)
{
    if (texture->vk_sampler != VK_NULL_HANDLE)
        vkDestroySampler(g_vulkan.device, texture->vk_sampler, nullptr);
    if (texture->vk_image_view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, texture->vk_image_view, nullptr);
    if (texture->vk_memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, texture->vk_memory, nullptr);
    if (texture->vk_image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, texture->vk_image, nullptr);

    delete texture;
}

void platform::BindTexture(Texture* texture, int slot)
{
    VkDescriptorSet texture_descriptor_set = texture->vk_descriptor_set;
    vkCmdBindDescriptorSets(
        g_vulkan.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        g_vulkan.pipeline_layout,
        VK_SPACE_TEXTURE,
        1,
        &texture_descriptor_set,
        0,
        nullptr
    );
}

void platform::BeginRenderPass(Color clear_color)
{
    assert(g_vulkan.command_buffer);

    VkClearValue vk_clear_color = {
        .color = { clear_color.r, clear_color.g, clear_color.b, clear_color.a},
    };

    VkRenderPassBeginInfo vk_render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.render_pass,
        .framebuffer = g_vulkan.swapchain_framebuffers[g_vulkan.current_image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = (u32)(clear_color.a > 0 ? 1 : 0),
        .pClearValues = &vk_clear_color,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &vk_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)g_vulkan.swapchain_extent.width,
        .height = (float)g_vulkan.swapchain_extent.height,
        .minDepth = -1.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);
    
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

void platform::EndRenderPass()
{
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

static VkShaderModule CreateShaderModule(const void* code, u32 code_size, const char* name)
{
    assert(code);
    assert(code_size > 0);
    assert(name);

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = (const u32*)code
    };

    VkShaderModule module;
    if (vkCreateShaderModule(g_vulkan.device, &create_info, nullptr, &module) != VK_SUCCESS)
        return nullptr;

    SetVulkanObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)module, name);

    return module;
}

static bool CreateShaderInternal(
    platform::Shader* shader,
    const void* vertex_code,
    u32 vertex_code_size,
    const void* geometry_code,
    u32 geometry_code_size,
    const void* fragment_code,
    u32 fragment_code_size,
    ShaderFlags flags,
    const char* name)
{
    assert(vertex_code);
    assert(vertex_code_size > 0);
    assert(fragment_code);
    assert(fragment_code_size > 0);
    assert(name);
    // geometry_code can be nullptr for non-geometry shaders


    VkVertexInputAttributeDescription attr_descs[] = {
        // Position
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0
        },
        // UV
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = sizeof(float) * 2
        },
        // Normal
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = sizeof(float) * 4
        },
        // Bone index
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32_SFLOAT,
            .offset = sizeof(float) * 7
        }
    };

    VkVertexInputBindingDescription binding_desc = {
        .binding = 0,
        .stride = sizeof(float) * 8,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = sizeof(attr_descs) / sizeof(VkVertexInputAttributeDescription),
        .pVertexAttributeDescriptions = attr_descs,
    };

    shader->vertex_module = CreateShaderModule(vertex_code, vertex_code_size, name);
    shader->geometry_module = geometry_code ? CreateShaderModule(geometry_code, geometry_code_size, name) : VK_NULL_HANDLE;
    shader->fragment_module = CreateShaderModule(fragment_code, fragment_code_size, name);

    // Build shader stages array dynamically based on what shaders are present
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    
    // Vertex shader (always present)
    shader_stages.push_back({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader->vertex_module,
        .pName = "main",
    });
    
    // Geometry shader (optional)
    if (shader->geometry_module != VK_NULL_HANDLE)
    {
        shader_stages.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_GEOMETRY_BIT,
            .module = shader->geometry_module,
            .pName = "main",
        });
    }
    
    // Fragment shader (always present)
    shader_stages.push_back({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader->fragment_module,
        .pName = "main"
    });

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = g_vulkan.msaa_samples,
        .sampleShadingEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    if (flags & SHADER_FLAGS_BLEND)
    {
        // Standard alpha blending: (src_alpha * src_color) + ((1 - src_alpha) * dst_color)
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    else
    {
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
        .pDynamicStates = dynamic_states
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = g_vulkan.pipeline_layout,
        .renderPass = g_vulkan.render_pass,
        .subpass = 0
    };

    return vkCreateGraphicsPipelines(g_vulkan.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &shader->pipeline) == VK_SUCCESS;
}

static VkSampleCountFlagBits GetMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(g_vulkan.physical_device, &physical_device_properties);

    VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & 
                                physical_device_properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

static void CreateColorResources()
{
    VkFormat color_format = g_vulkan.swapchain_image_format;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = g_vulkan.swapchain_extent.width;
    image_info.extent.height = g_vulkan.swapchain_extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = color_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.samples = g_vulkan.msaa_samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(g_vulkan.device, &image_info, nullptr, &g_vulkan.msaa_color_image) != VK_SUCCESS)
        Exit("Failed to create MSAA color image");

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.msaa_color_image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    if (vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &g_vulkan.msaa_color_image_memory) != VK_SUCCESS)
        Exit("Failed to allocate MSAA color image memory");

    vkBindImageMemory(g_vulkan.device, g_vulkan.msaa_color_image, g_vulkan.msaa_color_image_memory, 0);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vulkan.msaa_color_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = color_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if (vkCreateImageView(g_vulkan.device, &view_info, nullptr, &g_vulkan.msaa_color_image_view) != VK_SUCCESS)
        Exit("Failed to create MSAA color image view");

    SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)g_vulkan.msaa_color_image, "MSAA Color");
    SetVulkanObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)g_vulkan.msaa_color_image_view, "MSAA Color");
}

static void CleanupColorResources()
{
    if (g_vulkan.msaa_color_image_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(g_vulkan.device, g_vulkan.msaa_color_image_view, nullptr);
        g_vulkan.msaa_color_image_view = VK_NULL_HANDLE;
    }
    if (g_vulkan.msaa_color_image != VK_NULL_HANDLE)
    {
        vkDestroyImage(g_vulkan.device, g_vulkan.msaa_color_image, nullptr);
        g_vulkan.msaa_color_image = VK_NULL_HANDLE;
    }
    if (g_vulkan.msaa_color_image_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(g_vulkan.device, g_vulkan.msaa_color_image_memory, nullptr);
        g_vulkan.msaa_color_image_memory = VK_NULL_HANDLE;
    }
}

platform::Shader* platform::CreateShader(
    const void* vertex_code,
    u32 vertex_code_size,
    const void* geometry_code,
    u32 geometry_code_size,
    const void* fragment_code,
    u32 fragment_code_size,
    ShaderFlags flags,
    const char* name)
{
    Shader* shader = (Shader*)Alloc(ALLOCATOR_DEFAULT, sizeof(Shader));
    if (!CreateShaderInternal(shader, vertex_code, vertex_code_size, geometry_code, geometry_code_size, fragment_code, fragment_code_size, flags, name))
    {
        DestroyShader(shader);
        return nullptr;
    }

    return shader;
}

void platform::BindShader(Shader* shader)
{
    vkCmdBindPipeline(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
}

void platform::DestroyShader(Shader* shader)
{
    assert(shader);
    assert(g_vulkan.device);
    if (shader->fragment_module != VK_NULL_HANDLE)
        vkDestroyShaderModule(g_vulkan.device, (VkShaderModule)shader->fragment_module, nullptr);
    if (shader->geometry_module != VK_NULL_HANDLE)
        vkDestroyShaderModule(g_vulkan.device, (VkShaderModule)shader->geometry_module, nullptr);
    if (shader->vertex_module != VK_NULL_HANDLE)
        vkDestroyShaderModule(g_vulkan.device, (VkShaderModule)shader->vertex_module, nullptr);
    if (shader->pipeline)
        vkDestroyPipeline(g_vulkan.device, (VkPipeline)shader->pipeline, nullptr);
}

void InitVulkan(const RendererTraits* traits, HWND hwnd)
{
    g_vulkan = {};
    g_vulkan.traits = *traits;
    g_vulkan.hwnd = hwnd;

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
    CreateColorResources();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();

    // Get device properties for uniform buffer alignment
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(g_vulkan.physical_device, &device_properties);
    g_vulkan.min_uniform_buffer_offset_alignment = device_properties.limits.minUniformBufferOffsetAlignment;

    // Create descriptor set layouts first
    CreateDescriptorSetLayout();

    // Create dynamic uniform buffers with layouts
    const char* buffer_names[] = {"CameraBuffer", "TransformBuffer", "ColorBuffer"};
    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        if (!CreateUniformBuffer(&g_vulkan.uniform_buffers[i], buffer_names[i]))
            Exit("Failed to create uniform buffer");
    }

    CreateDescriptorPool();
    CreateDescriptorSets();
    CreatePipelineLayout();
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

        // Cleanup MSAA resources
        CleanupColorResources();

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

    if (g_vulkan.surface)
        vkDestroySurfaceKHR(g_vulkan.instance, g_vulkan.surface, nullptr);

#ifdef _DEBUG
    if (g_vulkan.debug_messenger)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            g_vulkan.instance,
            "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(g_vulkan.instance, g_vulkan.debug_messenger, nullptr);
    }
#endif

    if (g_vulkan.instance)
        vkDestroyInstance(g_vulkan.instance, nullptr);

    if (g_vulkan.library)
        FreeLibrary(g_vulkan.library);

    g_vulkan = {};
}
