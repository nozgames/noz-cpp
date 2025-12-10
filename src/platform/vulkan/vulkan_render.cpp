//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include "../vulkan/vulkan_render.h"

#define VK_CHECK(x) if((x) != VK_SUCCESS) Exit("vulkan init failed")
#define VK_NAME(id, handle, name) SetVulkanObjectName(id, reinterpret_cast<uint64_t>(handle), name)

enum UniformBufferType {
    UNIFORM_BUFFER_CAMERA,
    UNIFORM_BUFFER_OBJECT,
    UNIFORM_BUFFER_SKELETON,
    UNIFORM_BUFFER_VERTEX_USER,
    UNIFORM_BUFFER_COLOR,
    UNIFORM_BUFFER_FRAGMENT_USER,
    UNIFORM_BUFFER_COUNT
};

constexpr int VK_MAX_TEXTURES = 64;
constexpr int VK_SPACE_TEXTURE = UNIFORM_BUFFER_COUNT;
constexpr int VK_MAX_UNIFORM_BUFFERS = 8192;
constexpr u32 VK_DYNAMIC_UNIFORM_BUFFER_SIZE = MAX_UNIFORM_BUFFER_SIZE * VK_MAX_UNIFORM_BUFFERS;

const char* VK_UNIFORM_BUFFER_NAMES[] = {
    "CameraBuffer",
    "TransformBuffer",
    "SkeletonBuffer",
    "VertexUserBuffer",
    "ColorBuffer",
    "FragmentUserBuffer"
};

static_assert(sizeof(VK_UNIFORM_BUFFER_NAMES) / sizeof(const char*) == UNIFORM_BUFFER_COUNT);

struct PlatformTexture {
    VkImage vk_image;
    VkImageView vk_image_view;
    VkDeviceMemory vk_memory;
    VkSampler vk_sampler;
    VkDescriptorSet vk_descriptor_set;
    SamplerOptions sampler_options;
    Vec2Int size;
    i32 channels;
};

struct PlatformShader {
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkPipeline pipeline;
};

struct ObjectBuffer {
    float transform[12];
    float depth;
    float depth_scale;
    float depth_min;
    float depth_max;
};

struct ColorBuffer {
    Color color;
    Color emission;
    Vec2 uv_offset;
    Vec2 padding;
};

struct DynamicBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped_ptr;
    u32 offset;
    u32 alignment;
    u32 size;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;
};

struct OffscreenTarget {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet descriptor_set;
    VkFramebuffer framebuffer;
};

struct Swapchain {
    VkImage image;
    VkImageView view;
    VkFramebuffer framebuffer;
};

struct VulkanRenderer {
    RendererTraits traits;
    HWND hwnd;
    HMODULE library;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkFence in_flight_fence;
    VkPipelineLayout pipeline_layout;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkSampleCountFlagBits msaa_samples;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout sampler_descriptor_set_layout;
    VkDescriptorSet sampler_descriptor_set;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    std::vector<Swapchain> swapchain_framebuffers;
    std::vector<VkFramebuffer> postprocess_framebuffers;
    std::vector<VkFramebuffer> composite_framebuffers;

    DynamicBuffer uniform_buffers[UNIFORM_BUFFER_COUNT];

    VkRenderPass render_pass;
    VkRenderPass scene_render_pass;
    VkRenderPass post_proc_render_pass;
    VkRenderPass ui_render_pass;
    VkRenderPass composite_render_pass;

    OffscreenTarget scene_target;
    OffscreenTarget ui_target;
    OffscreenTarget msaa_target;
    OffscreenTarget depth_target;

    u32 min_uniform_buffer_offset_alignment;
    u32 current_image_index;
    float depth_conversion_factor;

    VkFramebuffer ui_framebuffer;

    VkImage ui_msaa_color_image;
    VkDeviceMemory ui_msaa_color_image_memory;
    VkImageView ui_msaa_color_image_view;

    VkImage ui_depth_image;
    VkDeviceMemory ui_depth_image_memory;
    VkImageView ui_depth_image_view;

    bool postprocess_enabled;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

static VulkanRenderer g_vulkan = {};

extern void InitInstanceFunctions(VkInstance instance);
extern void InitDeviceFunctions(VkInstance instance);

static void SetVulkanObjectName(VkObjectType object_type, uint64_t object_handle, const char* name) {
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

static VkSampleCountFlagBits GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(g_vulkan.physical_device, &physical_device_properties);

    VkSampleCountFlags counts =
        physical_device_properties.limits.framebufferColorSampleCounts &
        physical_device_properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

static u32 FindMemoryType(u32 type_filter, VkMemoryPropertyFlags properties) {
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

static void InitUniformBuffer(DynamicBuffer* buffer, const char* name) {
    buffer->size = VK_DYNAMIC_UNIFORM_BUFFER_SIZE;
    buffer->alignment = g_vulkan.min_uniform_buffer_offset_alignment;
    buffer->offset = 0;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer->size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VK_CHECK(vkCreateBuffer(g_vulkan.device, &buffer_info, nullptr, &buffer->buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, buffer->buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(
            mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &buffer->memory));
    VK_CHECK(vkBindBufferMemory(g_vulkan.device, buffer->buffer, buffer->memory, 0));
    VK_CHECK(vkMapMemory(g_vulkan.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped_ptr));
    VK_NAME(VK_OBJECT_TYPE_BUFFER, buffer->buffer, name);
}

static void InitUniformBuffers() {
    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++)
        InitUniformBuffer(&g_vulkan.uniform_buffers[i], VK_UNIFORM_BUFFER_NAMES[i]);
}

static void* AcquireUniformBuffer(UniformBufferType type) {
    assert(type >= 0 && type < UNIFORM_BUFFER_COUNT);

    u32 aligned_size = (MAX_UNIFORM_BUFFER_SIZE + g_vulkan.min_uniform_buffer_offset_alignment - 1)
                       & ~(g_vulkan.min_uniform_buffer_offset_alignment - 1);

    DynamicBuffer* buffer = &g_vulkan.uniform_buffers[type];

    if (buffer->offset + aligned_size > buffer->size)
        return nullptr;

    u32 current_offset = buffer->offset;
    void* ptr = static_cast<char *>(buffer->mapped_ptr) + current_offset;
    buffer->offset += aligned_size;

    u32 descriptor_space = static_cast<u32>(type);
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

static void CreateDescriptorSetLayout(VkShaderStageFlags stage_flags, UniformBufferType buffer_type) {
    VkDescriptorSetLayoutBinding uniform_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = stage_flags,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo uniform_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uniform_binding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(
        g_vulkan.device,
        &uniform_layout,
        nullptr,
        &g_vulkan.uniform_buffers[buffer_type].descriptor_set_layout));
}

static void InitDescriptorSetLayout() {
    // uniform buffers
    CreateDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_CAMERA);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_OBJECT);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_SKELETON);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_VERTEX_USER);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_COLOR);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_FRAGMENT_USER);
    CreateDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_FRAGMENT_USER);

    // samplers
    VkDescriptorSetLayoutBinding sampler_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo sampler_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &sampler_binding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(g_vulkan.device, &sampler_layout, nullptr, &g_vulkan.sampler_descriptor_set_layout));
}

static void InitDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = UNIFORM_BUFFER_COUNT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = VK_MAX_TEXTURES
        }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = UNIFORM_BUFFER_COUNT + VK_MAX_TEXTURES;
    VK_CHECK(vkCreateDescriptorPool(g_vulkan.device, &pool_info, nullptr, &g_vulkan.descriptor_pool));
}

static void InitDescriptorSets() {
    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_vulkan.descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &g_vulkan.uniform_buffers[i].descriptor_set_layout,
        };

        VK_CHECK(vkAllocateDescriptorSets(g_vulkan.device, &alloc_info, &g_vulkan.uniform_buffers[i].descriptor_set));

        VkDescriptorBufferInfo buffer_info = {
            .buffer = g_vulkan.uniform_buffers[i].buffer,
            .offset = 0,
            .range = MAX_UNIFORM_BUFFER_SIZE
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vulkan.uniform_buffers[i].descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(g_vulkan.device, 1, &write, 0, nullptr);
        VK_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, g_vulkan.uniform_buffers[i].descriptor_set, VK_UNIFORM_BUFFER_NAMES[i]);
    }

    VkDescriptorSetAllocateInfo texture_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vulkan.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.sampler_descriptor_set_layout,
    };

    VK_CHECK(vkAllocateDescriptorSets(g_vulkan.device, &texture_alloc_info, &g_vulkan.sampler_descriptor_set));
    VK_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, g_vulkan.sampler_descriptor_set, "Textures");
}

#if defined(_DEBUG)
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    (void)messageType;
    (void)pUserData;

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        printf("Vulkan validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static bool CheckValidationLayerSupport() {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    for (const auto& layerProperties : availableLayers) {
        if (strcmp(validationLayer, layerProperties.layerName) == 0)
            return true;
    }
    return false;
}
#endif

static std::vector<const char*> GetRequiredExtensions() {
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

#ifdef _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

static void InitDebugMessenger() {
#ifdef _DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(g_vulkan.instance, "vkCreateDebugUtilsMessengerEXT"));

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

static void InitSurface() {
    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = g_vulkan.hwnd,
    };
    VK_CHECK(vkCreateWin32SurfaceKHR(g_vulkan.instance, &createInfo, nullptr, &g_vulkan.surface));
}

struct QueueFamilyIndices {
    u32 graphics_family = UINT32_MAX;
    u32 present_family = UINT32_MAX;

    bool IsComplete() const {
        return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
    }
};

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (u32 i = 0; i < queueFamilies.size(); i++) {
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

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    const char* requiredExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    for (const auto& extension : availableExtensions)
        if (strcmp(extension.extensionName, requiredExtension) == 0)
            return true;

    return false;
}

static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_vulkan.surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vulkan.surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vulkan.surface, &presentModeCount, details.present_modes.data());
    }

    return details;
}

static bool IsDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.present_modes.empty();
    }

    return indices.IsComplete() && extensionsSupported && swapchainAdequate;
}

static void InitPhysicsDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        Exit("Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(g_vulkan.instance, &deviceCount, devices.data());

    for (const auto& device : devices)
        if (IsDeviceSuitable(device)) {
            g_vulkan.physical_device = device;
            break;
        }

    if (g_vulkan.physical_device == VK_NULL_HANDLE)
        Exit("Failed to find a suitable GPU");

    g_vulkan.msaa_samples = g_vulkan.traits.msaa
        ? GetMaxUsableSampleCount()
        : VK_SAMPLE_COUNT_1_BIT;

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(g_vulkan.physical_device, &device_properties);
    g_vulkan.min_uniform_buffer_offset_alignment = static_cast<u32>(device_properties.limits.minUniformBufferOffsetAlignment);
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    (void)availablePresentModes;
    return g_vulkan.traits.vsync == 0
        ? VK_PRESENT_MODE_IMMEDIATE_KHR
        : VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;

    Vec2Int screen_size = GetScreenSize();
    VkExtent2D actual_extent = {
        static_cast<u32>(screen_size.x),
        static_cast<u32>(screen_size.y)
    };

    actual_extent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actual_extent.height));

    return actual_extent;
}

static void InitSwapchains() {
    SwapchainSupportDetails swapchain_support = QuerySwapchainSupport(g_vulkan.physical_device);
    VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swapchain_support.formats);
    VkPresentModeKHR present_mode = ChooseSwapPresentMode(swapchain_support.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapchain_support.capabilities);

    u32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
        image_count = swapchain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_vulkan.surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);
    u32 queue_family_indices[] = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family != indices.present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queue_family_indices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchain_support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain));

    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &image_count, nullptr);
    g_vulkan.swapchain_framebuffers.resize(image_count);

    std::vector<VkImage> swapchain_images(image_count);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &image_count, swapchain_images.data());

    g_vulkan.swapchain_image_format = surface_format.format;
    g_vulkan.swapchain_extent = extent;

    for (size_t i = 0; i < g_vulkan.swapchain_framebuffers.size(); i++) {
        Swapchain& swapchain = g_vulkan.swapchain_framebuffers[i];
        swapchain.image = swapchain_images[i];

        VkImageViewCreateInfo vk_image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain.image,
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

        VK_CHECK(vkCreateImageView(g_vulkan.device, &vk_image_create_info, nullptr, &swapchain.view));

        Text image_name = {};
        Format(image_name, "Swapchain%zu", i);
        VK_NAME(VK_OBJECT_TYPE_IMAGE,swapchain.image, image_name.value);
        VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW,swapchain.view, image_name.value);
    }
}

static void InitSwapchainFrameBuffers() {
    for (size_t i = 0; i < g_vulkan.swapchain_framebuffers.size(); i++) {
        Swapchain& swapchain = g_vulkan.swapchain_framebuffers[i];
        VkImageView vk_attachments[] = {
            g_vulkan.msaa_target.view,
            swapchain.view,
            g_vulkan.depth_target.view
        };

        VkFramebufferCreateInfo vk_frame_buffer_info = {};
        vk_frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vk_frame_buffer_info.renderPass = g_vulkan.render_pass;
        vk_frame_buffer_info.attachmentCount = sizeof(vk_attachments) / sizeof(VkImageView);
        vk_frame_buffer_info.pAttachments = vk_attachments;
        vk_frame_buffer_info.width = g_vulkan.swapchain_extent.width;
        vk_frame_buffer_info.height = g_vulkan.swapchain_extent.height;
        vk_frame_buffer_info.layers = 1;
        VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &vk_frame_buffer_info, nullptr, &swapchain.framebuffer));

        Text name = {};
        Format(name, "Swapchain%zu", i);
        VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, swapchain.framebuffer, name.value);
    }
}

static void InitRenderPass() {
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

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = g_vulkan.msaa_samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, colorAttachmentResolve, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 3;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(g_vulkan.device, &renderPassInfo, nullptr, &g_vulkan.render_pass));
}

static void InitCommandPool() {
    VkCommandPoolCreateInfo vf_pool_info = {};
    vf_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vf_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vf_pool_info.queueFamilyIndex = FindQueueFamilies(g_vulkan.physical_device).graphics_family;
    VK_CHECK(vkCreateCommandPool(g_vulkan.device, &vf_pool_info, nullptr, &g_vulkan.command_pool));
}

static void InitCommandBuffer() {
    VkCommandBufferAllocateInfo vf_alloc_info = {};
    vf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vf_alloc_info.commandPool = g_vulkan.command_pool;
    vf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vf_alloc_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(g_vulkan.device, &vf_alloc_info, &g_vulkan.command_buffer));
}

static void InitSyncObjects() {
    VkFenceCreateInfo vk_fence_info = {};
    vk_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vk_fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo vk_semaphore_info = {};
    vk_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(g_vulkan.device, &vk_semaphore_info, nullptr, &g_vulkan.image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(g_vulkan.device, &vk_semaphore_info, nullptr, &g_vulkan.render_finished_semaphore));
    VK_CHECK(vkCreateFence(g_vulkan.device, &vk_fence_info, nullptr, &g_vulkan.in_flight_fence));
}

static void InitPipeline() {
    VkDescriptorSetLayout layouts[] = {
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_CAMERA].descriptor_set_layout,
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_OBJECT].descriptor_set_layout,
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_SKELETON].descriptor_set_layout,
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_VERTEX_USER].descriptor_set_layout,
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_COLOR].descriptor_set_layout,
        g_vulkan.uniform_buffers[UNIFORM_BUFFER_FRAGMENT_USER].descriptor_set_layout,
        g_vulkan.sampler_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = sizeof(layouts) / sizeof(VkDescriptorSetLayout);
    layout_info.pSetLayouts = layouts;
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;
    vkCreatePipelineLayout(g_vulkan.device, &layout_info, nullptr, &g_vulkan.pipeline_layout);
}

static void DestroyOffscreenTarget(OffscreenTarget& target) {
    if (target.sampler != VK_NULL_HANDLE)
        vkDestroySampler(g_vulkan.device, target.sampler, nullptr);
    if (target.view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, target.view, nullptr);
    if (target.image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, target.image, nullptr);
    if (target.memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, target.memory, nullptr);
    if (target.framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(g_vulkan.device, target.framebuffer, nullptr);

    target.sampler = VK_NULL_HANDLE;
    target.view = VK_NULL_HANDLE;
    target.image = VK_NULL_HANDLE;
    target.memory = VK_NULL_HANDLE;
    target.framebuffer = VK_NULL_HANDLE;
}

static void DestroyFrameBuffers() {
    for (auto framebuffer : g_vulkan.swapchain_framebuffers) {
        if (framebuffer.framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(g_vulkan.device, framebuffer.framebuffer, nullptr);
        if (framebuffer.view != VK_NULL_HANDLE)
            vkDestroyImageView(g_vulkan.device, framebuffer.view, nullptr);
        if (framebuffer.image != VK_NULL_HANDLE)
            vkDestroyImage(g_vulkan.device, framebuffer.image, nullptr);
    }

    if (g_vulkan.scene_target.sampler != VK_NULL_HANDLE)
        vkDestroySampler(g_vulkan.device, g_vulkan.scene_target.sampler, nullptr);
    if (g_vulkan.scene_target.view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, g_vulkan.scene_target.view, nullptr);
    if (g_vulkan.scene_target.image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, g_vulkan.scene_target.image, nullptr);
    if (g_vulkan.scene_target.memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, g_vulkan.scene_target.memory, nullptr);
    if (g_vulkan.scene_target.framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(g_vulkan.device, g_vulkan.scene_target.framebuffer, nullptr);

    for (auto framebuffer : g_vulkan.postprocess_framebuffers)
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(g_vulkan.device, framebuffer, nullptr);

    for (auto framebuffer : g_vulkan.composite_framebuffers)
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(g_vulkan.device, framebuffer, nullptr);

    if (g_vulkan.ui_framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(g_vulkan.device, g_vulkan.ui_framebuffer, nullptr);
    if (g_vulkan.ui_msaa_color_image_view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, g_vulkan.ui_msaa_color_image_view, nullptr);
    if (g_vulkan.ui_msaa_color_image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, g_vulkan.ui_msaa_color_image, nullptr);
    if (g_vulkan.ui_msaa_color_image_memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, g_vulkan.ui_msaa_color_image_memory, nullptr);
    if (g_vulkan.ui_depth_image_view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, g_vulkan.ui_depth_image_view, nullptr);
    if (g_vulkan.ui_depth_image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, g_vulkan.ui_depth_image, nullptr);
    if (g_vulkan.ui_depth_image_memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, g_vulkan.ui_depth_image_memory, nullptr);
    if (g_vulkan.ui_target.sampler != VK_NULL_HANDLE)
        vkDestroySampler(g_vulkan.device, g_vulkan.ui_target.sampler, nullptr);
    if (g_vulkan.ui_target.view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, g_vulkan.ui_target.view, nullptr);
    if (g_vulkan.ui_target.image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, g_vulkan.ui_target.image, nullptr);
    if (g_vulkan.ui_target.memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, g_vulkan.ui_target.memory, nullptr);

    if (g_vulkan.swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(g_vulkan.device, g_vulkan.swapchain, nullptr);

    DestroyOffscreenTarget(g_vulkan.scene_target);
    DestroyOffscreenTarget(g_vulkan.ui_target);
    DestroyOffscreenTarget(g_vulkan.msaa_target);
    DestroyOffscreenTarget(g_vulkan.depth_target);

    g_vulkan.scene_target.sampler = VK_NULL_HANDLE;
    g_vulkan.scene_target.view = VK_NULL_HANDLE;
    g_vulkan.scene_target.image = VK_NULL_HANDLE;
    g_vulkan.scene_target.memory = VK_NULL_HANDLE;
    g_vulkan.scene_target.framebuffer = VK_NULL_HANDLE;
    g_vulkan.ui_framebuffer = VK_NULL_HANDLE;
    g_vulkan.ui_msaa_color_image_view = VK_NULL_HANDLE;
    g_vulkan.ui_msaa_color_image = VK_NULL_HANDLE;
    g_vulkan.ui_msaa_color_image_memory = VK_NULL_HANDLE;
    g_vulkan.ui_depth_image_view = VK_NULL_HANDLE;
    g_vulkan.ui_depth_image = VK_NULL_HANDLE;
    g_vulkan.ui_depth_image_memory = VK_NULL_HANDLE;
    g_vulkan.ui_target.sampler = VK_NULL_HANDLE;
    g_vulkan.ui_target.view = VK_NULL_HANDLE;
    g_vulkan.ui_target.image = VK_NULL_HANDLE;
    g_vulkan.ui_target.memory = VK_NULL_HANDLE;
    g_vulkan.swapchain = VK_NULL_HANDLE;
    g_vulkan.swapchain_framebuffers.clear();
    g_vulkan.postprocess_framebuffers.clear();
    g_vulkan.composite_framebuffers.clear();
}

static void WaitRenderDriver() {
    vkWaitForFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence);
}

static void CopyMat3ToGPU(void* dst, const Mat3& src) {
    float *f = static_cast<float *>(dst);
    f[0] = src.m[0]; f[1] = src.m[1]; f[2] = src.m[2]; f[3] = 0.0f;
    f[4] = src.m[3]; f[5] = src.m[4]; f[6] = src.m[5]; f[7] = 0.0f;
    f[8] = src.m[6]; f[9] = src.m[7]; f[10]= src.m[8]; f[11]= 0.0f;
}

void PlatformBindSkeleton(const Mat3* bone_transforms, u8 bone_count) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_SKELETON);
    if (!buffer_ptr)
        return;

    float* bones = (static_cast<float*>(buffer_ptr));
    for (u8 i = 0; i < bone_count; i++)
        CopyMat3ToGPU(bones + i * 12, bone_transforms[i]);
}

void PlatformBindTransform(const Mat3& transform, float depth, float depth_scale) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_OBJECT);
    if (!buffer_ptr)
        return;

    ObjectBuffer* buffer = static_cast<ObjectBuffer*>(buffer_ptr);
    buffer->depth = depth;
    buffer->depth_scale = depth_scale;
    buffer->depth_min = GetApplicationTraits()->renderer.min_depth;
    buffer->depth_max = GetApplicationTraits()->renderer.max_depth;
    CopyMat3ToGPU(&buffer->transform, transform);
}

void PlatformBindVertexUserData(const u8* data, u32 size) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_VERTEX_USER);
    if (!buffer_ptr)
        return;

    memcpy(buffer_ptr, data, size);
}

void PlatformBindFragmentUserData(const u8* data, u32 size) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_FRAGMENT_USER);
    if (!buffer_ptr)
        return;

    memcpy(buffer_ptr, data, size);
}

void PlatformBindCamera(const Mat3& view_matrix) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_CAMERA);
    if (!buffer_ptr)
        return;

    CopyMat3ToGPU(buffer_ptr, view_matrix);
}

void PlatformBindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission) {
    void* buffer_ptr = AcquireUniformBuffer(UNIFORM_BUFFER_COLOR);
    if (!buffer_ptr)
        return;

    ColorBuffer buffer = {
        .color = color,
        .emission = emission,
        .uv_offset = color_uv_offset
    };

    memcpy(buffer_ptr, &buffer, sizeof(ColorBuffer));
}

void PlatformFree(PlatformBuffer* buffer) {
    if (buffer)
        vkDestroyBuffer(g_vulkan.device, reinterpret_cast<VkBuffer>(buffer), nullptr);
}

PlatformBuffer* PlatformCreateVertexBuffer(const MeshVertex* vertices, u16 vertex_count, const char* name) {
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
    if (VK_SUCCESS != vkAllocateMemory(g_vulkan.device, &vk_alloc_info, nullptr, &vk_vertex_memory)) {
        vkDestroyBuffer(g_vulkan.device, vf_vertex_buffer, nullptr);
        return nullptr;
    }

    vkBindBufferMemory(g_vulkan.device, vf_vertex_buffer, vk_vertex_memory, 0);

    void* mapped_data;
    if (VK_SUCCESS == vkMapMemory(g_vulkan.device, vk_vertex_memory, 0, vk_buffer_info.size, 0, &mapped_data)) {
        memcpy(mapped_data, vertices, vk_buffer_info.size);
        vkUnmapMemory(g_vulkan.device, vk_vertex_memory);
    }

    SetVulkanObjectName(
        VK_OBJECT_TYPE_BUFFER,
        reinterpret_cast<uint64_t>(vf_vertex_buffer),
        name);

    return reinterpret_cast<PlatformBuffer *>(vf_vertex_buffer);
}

PlatformBuffer* PlatformCreateIndexBuffer(const u16* indices, u16 index_count, const char* name) {
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
    if (vkAllocateMemory(g_vulkan.device, &vk_alloc_info, nullptr, &index_memory) != VK_SUCCESS) {
        vkDestroyBuffer(g_vulkan.device, index_buffer, nullptr);
        return nullptr;
    }

    vkBindBufferMemory(g_vulkan.device, index_buffer, index_memory, 0);

    void* data;
    if (vkMapMemory(g_vulkan.device, index_memory, 0, vk_buffer_info.size, 0, &data) == VK_SUCCESS) {
        memcpy(data, indices, vk_buffer_info.size);
        vkUnmapMemory(g_vulkan.device, index_memory);
    }

    SetVulkanObjectName(
        VK_OBJECT_TYPE_BUFFER,
        reinterpret_cast<uint64_t>(index_buffer),
        name);

    return reinterpret_cast<PlatformBuffer *>(index_buffer);
}

void PlatformBindVertexBuffer(PlatformBuffer* buffer) {
    assert(buffer);
    assert(g_vulkan.command_buffer);
    VkBuffer vertex_buffers[] = {(VkBuffer)buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(g_vulkan.command_buffer, 0, 1, vertex_buffers, offsets);
}

void PlatformBindIndexBuffer(PlatformBuffer* buffer) {
    assert(buffer);
    assert(g_vulkan.command_buffer);
    vkCmdBindIndexBuffer(g_vulkan.command_buffer, (VkBuffer)buffer, 0, VK_INDEX_TYPE_UINT16);
}

void PlatformDrawIndexed(u16 index_count) {
    assert(g_vulkan.command_buffer);
    assert(index_count > 0);
    vkCmdDrawIndexed(g_vulkan.command_buffer, index_count, 1, 0, 0, 0);
}

static bool CreateTextureInternal(PlatformTexture* texture, void* data, const SamplerOptions& sampler_options, const char* name) {
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

    if (data) {
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

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

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
        .pSetLayouts = &g_vulkan.sampler_descriptor_set_layout
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

PlatformTexture* PlatformCreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name) {
    PlatformTexture* texture = new PlatformTexture{};
    texture->size = {(int)width, (int)height};
    texture->channels = channels;
    if (!CreateTextureInternal(texture, data, sampler_options, name)) {
        Free(texture);
        return nullptr;
    }

    return texture;
}

void PlatformFree(PlatformTexture* texture) {
    if (!texture)
        return;

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

void PlatformBindTexture(PlatformTexture* texture, int slot) {
    (void)slot;

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

void PlatformBeginScenePass(Color clear_color) {
    assert(g_vulkan.command_buffer);
    assert(g_vulkan.scene_target.framebuffer);
    assert(g_vulkan.scene_render_pass);

    VkClearValue clear_values[3] = {};
    clear_values[0].color = {clear_color.r, clear_color.g, clear_color.b, clear_color.a};
    clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clear_values[2].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo vk_render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.scene_render_pass,
        .framebuffer = g_vulkan.scene_target.framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = 3,
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &vk_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x = 0.0f,
        .y = static_cast<float>(g_vulkan.swapchain_extent.height),
        .width = static_cast<float>(g_vulkan.swapchain_extent.width),
        .height = -static_cast<float>(g_vulkan.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

void PlatformEndScenePass() {
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

void PlatformBeginPostProcPass() {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = g_vulkan.scene_target.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vkCmdPipelineBarrier(
        g_vulkan.command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.post_proc_render_pass,
        .framebuffer = g_vulkan.postprocess_framebuffers[g_vulkan.current_image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = 0,
        .pClearValues = nullptr,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(g_vulkan.swapchain_extent.width),
        .height = static_cast<float>(g_vulkan.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

void PlatformEndPostProcPass() {
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

void PlatformEnablePostProcess(bool enabled) {
    g_vulkan.postprocess_enabled = enabled;
}

void PlatformBeginUIPass() {
    VkClearValue clear_values[3] = {};
    clear_values[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clear_values[1].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clear_values[2].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.ui_render_pass,
        .framebuffer = g_vulkan.ui_framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = 3,
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x = 0.0f,
        .y = static_cast<float>(g_vulkan.swapchain_extent.height),
        .width = static_cast<float>(g_vulkan.swapchain_extent.width),
        .height = -static_cast<float>(g_vulkan.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

void PlatformEndUIPass() {
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

void PlatformBeginCompositePass() {
    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.composite_render_pass,
        .framebuffer = g_vulkan.composite_framebuffers[g_vulkan.current_image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = 0,
        .pClearValues = nullptr,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x = 0.0f,
        .y = static_cast<float>(g_vulkan.swapchain_extent.height),
        .width = static_cast<float>(g_vulkan.swapchain_extent.width),
        .height = -static_cast<float>(g_vulkan.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

void PlatformEndCompositePass() {
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

void PlatformBindOffscreenTexture() {
    vkCmdBindDescriptorSets(
        g_vulkan.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        g_vulkan.pipeline_layout,
        VK_SPACE_TEXTURE,
        1,
        &g_vulkan.scene_target.descriptor_set,
        0,
        nullptr
    );
}

void PlatformBindUITexture() {
    vkCmdBindDescriptorSets(
        g_vulkan.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        g_vulkan.pipeline_layout,
        VK_SPACE_TEXTURE,
        1,
        &g_vulkan.ui_target.descriptor_set,
        0,
        nullptr
    );
}

#if 0
static void EndRenderPass() {
    vkCmdEndRenderPass(g_vulkan.command_buffer);

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vulkan.composite_render_pass,
        .framebuffer = g_vulkan.composite_framebuffers[g_vulkan.current_image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        },
        .clearValueCount = 0,
        .pClearValues = nullptr,
    };

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x = 0.0f,
        .y = static_cast<float>(g_vulkan.swapchain_extent.height),
        .width = static_cast<float>(g_vulkan.swapchain_extent.width),
        .height = -static_cast<float>(g_vulkan.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_vulkan.swapchain_extent
    };
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}
#endif

void PlatformSetViewport(const noz::Rect& viewport) {
    VkViewport vk_viewport;
    VkRect2D scissor;

    if (viewport.width > 0 && viewport.height > 0) {
        float screen_height = (float)g_vulkan.swapchain_extent.height;
        float vk_y = screen_height - viewport.y - viewport.height;
        vk_viewport = {
            .x = viewport.x,
            .y = vk_y + viewport.height,
            .width = viewport.width,
            .height = -viewport.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        scissor = {
            .offset = {(i32)viewport.x, (i32)vk_y},
            .extent = {(u32)viewport.width, (u32)viewport.height}
        };
    } else {
        vk_viewport = {
            .x = 0.0f,
            .y = (float)g_vulkan.swapchain_extent.height,
            .width = (float)g_vulkan.swapchain_extent.width,
            .height = -(float)g_vulkan.swapchain_extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        scissor = {
            .offset = {0, 0},
            .extent = g_vulkan.swapchain_extent
        };
    }

    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &vk_viewport);
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
}

static VkShaderModule CreateShaderModule(const void* code, u32 code_size, const char* name) {
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
    PlatformShader* shader,
    const void* vertex_code,
    u32 vertex_code_size,
    const void* fragment_code,
    u32 fragment_code_size,
    ShaderFlags flags,
    const char* name) {
    assert(vertex_code);
    assert(vertex_code_size > 0);
    assert(fragment_code);
    assert(fragment_code_size > 0);
    assert(name);

    VkVertexInputAttributeDescription attr_descs[] = {
        // Position
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0
        },
        // Depth
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32_SFLOAT,
            .offset = offsetof(MeshVertex, depth)
        },
        // UV
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MeshVertex, uv)
        },
        // Normal
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MeshVertex, normal)
        },
        // Bone indices
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SINT,
            .offset = offsetof(MeshVertex, bone_indices)
        },
        // Bone Weights
        {
            .location = 5,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(MeshVertex, bone_weights)
        },
    };

    VkVertexInputBindingDescription binding_desc = {
        .binding = 0,
        .stride = sizeof(MeshVertex),
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
    shader->fragment_module = CreateShaderModule(fragment_code, fragment_code_size, name);

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    shader_stages.push_back({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader->vertex_module,
        .pName = "main",
    });

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

    bool is_postprocess = (flags & SHADER_FLAGS_POSTPROCESS) != 0;
    bool is_ui_composite = (flags & SHADER_FLAGS_UI_COMPOSITE) != 0;
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = is_postprocess || is_ui_composite
            ? VK_SAMPLE_COUNT_1_BIT
            : g_vulkan.msaa_samples,
        .sampleShadingEnable = VK_FALSE,
    };

    bool depth_test = (flags & SHADER_FLAGS_DEPTH) != 0;
    bool depth_less = (flags & SHADER_FLAGS_DEPTH_LESS) != 0;
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depth_test,
        .depthWriteEnable = depth_test,
        .depthCompareOp =  depth_less ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_LESS_OR_EQUAL,
    };

    bool is_premultiplied = (flags & SHADER_FLAGS_PREMULTIPLIED_ALPHA) != 0;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    if (is_ui_composite || is_premultiplied) {
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    } else if (flags & SHADER_FLAGS_BLEND) {
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    } else {
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

    VkRenderPass render_pass = g_vulkan.render_pass;
    if (is_ui_composite)
        render_pass = g_vulkan.composite_render_pass;
    else if (is_postprocess)
        render_pass = g_vulkan.post_proc_render_pass;

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
        .renderPass = render_pass,
        .subpass = 0
    };

    return vkCreateGraphicsPipelines(g_vulkan.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &shader->pipeline) == VK_SUCCESS;
}

static void InitMSAA() {
    OffscreenTarget target = g_vulkan.msaa_target;
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
    VK_CHECK(vkCreateImage(g_vulkan.device, &image_info, nullptr, &target.image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, target.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &target.memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, target.image, target.memory, 0));

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = target.image,
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

    VK_CHECK(vkCreateImageView(g_vulkan.device, &view_info, nullptr, &target.view));
    VK_NAME(VK_OBJECT_TYPE_IMAGE,target.image,"MSAA");
    VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW,target.view,"MSAA");
}

static void InitDepth() {
    OffscreenTarget& target = g_vulkan.depth_target;
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = g_vulkan.swapchain_extent.width;
    image_info.extent.height = g_vulkan.swapchain_extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = depth_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.samples = g_vulkan.msaa_samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateImage(g_vulkan.device, &image_info, nullptr, &target.image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, target.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &g_vulkan.depth_target.memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, g_vulkan.depth_target.image, g_vulkan.depth_target.memory, 0));

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = target.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VK_CHECK(vkCreateImageView(g_vulkan.device, &view_info, nullptr, &g_vulkan.depth_target.view));
    VK_NAME(VK_OBJECT_TYPE_IMAGE,g_vulkan.depth_target.image,"Depth");
    VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW,g_vulkan.depth_target.view,"Depth");
}

static void InitScene() {
    VkFormat color_format = g_vulkan.swapchain_image_format;
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = color_format,
        .extent = {
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK(vkCreateImage(g_vulkan.device, &image_info, nullptr, &g_vulkan.scene_target.image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.scene_target.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &g_vulkan.scene_target.memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, g_vulkan.scene_target.image, g_vulkan.scene_target.memory, 0));

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vulkan.scene_target.image,
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

    VK_CHECK(vkCreateImageView(g_vulkan.device, &view_info, nullptr, &g_vulkan.scene_target.view));

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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

    VK_CHECK(vkCreateSampler(g_vulkan.device, &sampler_info, nullptr, &g_vulkan.scene_target.sampler));

    VkDescriptorSetAllocateInfo desc_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vulkan.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.sampler_descriptor_set_layout
    };

    VK_CHECK(vkAllocateDescriptorSets(g_vulkan.device, &desc_alloc_info, &g_vulkan.scene_target.descriptor_set));

    VkDescriptorImageInfo desc_image_info = {
        .sampler = g_vulkan.scene_target.sampler,
        .imageView = g_vulkan.scene_target.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = g_vulkan.scene_target.descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &desc_image_info
    };
    vkUpdateDescriptorSets(g_vulkan.device, 1, &write, 0, nullptr);

    VK_NAME(VK_OBJECT_TYPE_IMAGE, g_vulkan.scene_target.image, "Scene");
    VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, g_vulkan.scene_target.view, "Scene");
    VK_NAME(VK_OBJECT_TYPE_SAMPLER, g_vulkan.scene_target.sampler, "Scene");
    VK_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, g_vulkan.scene_target.descriptor_set, "Scene");

    VkImageView attachments[] = {
        g_vulkan.msaa_target.view,
        g_vulkan.scene_target.view,
        g_vulkan.depth_target.view
    };

    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = g_vulkan.scene_render_pass,
        .attachmentCount = 3,
        .pAttachments = attachments,
        .width = g_vulkan.swapchain_extent.width,
        .height = g_vulkan.swapchain_extent.height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &framebuffer_info, nullptr, &g_vulkan.scene_target.framebuffer));
    VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, g_vulkan.scene_target.framebuffer, "Scene");
}

static void InitSceneRenderPass() {
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
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorAttachmentResolveRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = g_vulkan.msaa_samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, colorAttachmentResolve, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 3,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    VK_CHECK(vkCreateRenderPass(g_vulkan.device, &renderPassInfo, nullptr, &g_vulkan.scene_render_pass));
    VK_NAME(VK_OBJECT_TYPE_RENDER_PASS, g_vulkan.scene_render_pass, "Scene");
}

static void InitPostProc() {
    g_vulkan.postprocess_framebuffers.resize(g_vulkan.swapchain_framebuffers.size());

    for (size_t i = 0; i < g_vulkan.postprocess_framebuffers.size(); i++) {
        VkImageView attachments[] = { g_vulkan.swapchain_framebuffers[i].view };
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = g_vulkan.post_proc_render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .layers = 1
        };

        VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &framebuffer_info, nullptr, &g_vulkan.postprocess_framebuffers[i]));

        Text postproc_name = {};
        Format(postproc_name, "PostProcess %zu", i);
        VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER,g_vulkan.postprocess_framebuffers[i], postproc_name.value);
    }
}

static void InitPostProcPass() {
    VkAttachmentDescription color_attachment = {
        .format = g_vulkan.swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(g_vulkan.device, &render_pass_info, nullptr, &g_vulkan.post_proc_render_pass));
    VK_NAME(VK_OBJECT_TYPE_RENDER_PASS, g_vulkan.post_proc_render_pass, "PostProc");
}

static void InitUIPass() {
    VkAttachmentDescription colorAttachment = {
        .format = g_vulkan.swapchain_image_format,
        .samples = g_vulkan.msaa_samples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription colorAttachmentResolve = {
        .format = g_vulkan.swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment = {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = g_vulkan.msaa_samples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorAttachmentResolveRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &colorAttachmentResolveRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[] = {colorAttachment, colorAttachmentResolve, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 3,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(g_vulkan.device, &renderPassInfo, nullptr, &g_vulkan.ui_render_pass));
    VK_NAME(VK_OBJECT_TYPE_RENDER_PASS, g_vulkan.ui_render_pass, "UI");
}

static void InitCompositePass() {
    VkAttachmentDescription color_attachment = {
        .format = g_vulkan.swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(g_vulkan.device, &render_pass_info, nullptr, &g_vulkan.composite_render_pass));
    VK_NAME(VK_OBJECT_TYPE_RENDER_PASS, g_vulkan.composite_render_pass, "Composite");
}

static void InitUI() {
    VkImageCreateInfo msaa_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = g_vulkan.swapchain_image_format,
        .extent = {
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = g_vulkan.msaa_samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK(vkCreateImage(g_vulkan.device, &msaa_info, nullptr, &g_vulkan.ui_msaa_color_image));

    VkMemoryRequirements msaa_mem_req;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.ui_msaa_color_image, &msaa_mem_req);

    VkMemoryAllocateInfo msaa_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = msaa_mem_req.size,
        .memoryTypeIndex = FindMemoryType(msaa_mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &msaa_alloc, nullptr, &g_vulkan.ui_msaa_color_image_memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, g_vulkan.ui_msaa_color_image, g_vulkan.ui_msaa_color_image_memory, 0));

    VkImageViewCreateInfo msaa_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vulkan.ui_msaa_color_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = g_vulkan.swapchain_image_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VK_CHECK(vkCreateImageView(g_vulkan.device, &msaa_view_info, nullptr, &g_vulkan.ui_msaa_color_image_view));

    VK_NAME(VK_OBJECT_TYPE_IMAGE, g_vulkan.ui_msaa_color_image, "UI MSAA");
    VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, g_vulkan.ui_msaa_color_image_view, "UI MSAA");

    VkImageCreateInfo depth_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .extent = {
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = g_vulkan.msaa_samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK(vkCreateImage(g_vulkan.device, &depth_info, nullptr, &g_vulkan.ui_depth_image));

    VkMemoryRequirements depth_mem_req;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.ui_depth_image, &depth_mem_req);

    VkMemoryAllocateInfo depth_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = depth_mem_req.size,
        .memoryTypeIndex = FindMemoryType(depth_mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &depth_alloc, nullptr, &g_vulkan.ui_depth_image_memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, g_vulkan.ui_depth_image, g_vulkan.ui_depth_image_memory, 0));

    VkImageViewCreateInfo depth_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vulkan.ui_depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VK_CHECK(vkCreateImageView(g_vulkan.device, &depth_view_info, nullptr, &g_vulkan.ui_depth_image_view));

    SetVulkanObjectName(
        VK_OBJECT_TYPE_IMAGE,
        reinterpret_cast<uint64_t>(g_vulkan.ui_depth_image),
        "UI Depth");
    SetVulkanObjectName(
        VK_OBJECT_TYPE_IMAGE_VIEW,
        reinterpret_cast<uint64_t>(g_vulkan.ui_depth_image_view),
        "UI Depth");

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = g_vulkan.swapchain_image_format,
        .extent = {
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK(vkCreateImage(g_vulkan.device, &image_info, nullptr, &g_vulkan.ui_target.image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.ui_target.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &g_vulkan.ui_target.memory));
    VK_CHECK(vkBindImageMemory(g_vulkan.device, g_vulkan.ui_target.image, g_vulkan.ui_target.memory, 0));

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vulkan.ui_target.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = g_vulkan.swapchain_image_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VK_CHECK(vkCreateImageView(g_vulkan.device, &view_info, nullptr, &g_vulkan.ui_target.view));

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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

    VK_CHECK(vkCreateSampler(g_vulkan.device, &sampler_info, nullptr, &g_vulkan.ui_target.sampler));

    VkDescriptorSetAllocateInfo desc_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vulkan.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.sampler_descriptor_set_layout
    };

    VK_CHECK(vkAllocateDescriptorSets(g_vulkan.device, &desc_alloc_info, &g_vulkan.ui_target.descriptor_set));

    VkDescriptorImageInfo desc_image_info = {
        .sampler = g_vulkan.ui_target.sampler,
        .imageView = g_vulkan.ui_target.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = g_vulkan.ui_target.descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &desc_image_info
    };
    vkUpdateDescriptorSets(g_vulkan.device, 1, &write, 0, nullptr);

    VK_NAME(VK_OBJECT_TYPE_IMAGE,g_vulkan.ui_target.image, "UI");
    VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW,g_vulkan.ui_target.view, "UI");
    VK_NAME(VK_OBJECT_TYPE_SAMPLER,g_vulkan.ui_target.sampler, "UI");
    VK_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET,g_vulkan.ui_target.descriptor_set, "UI");

    VkImageView attachments[] = {
        g_vulkan.ui_msaa_color_image_view,
        g_vulkan.ui_target.view,
        g_vulkan.ui_depth_image_view
    };
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = g_vulkan.ui_render_pass,
        .attachmentCount = 3,
        .pAttachments = attachments,
        .width = g_vulkan.swapchain_extent.width,
        .height = g_vulkan.swapchain_extent.height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &framebuffer_info, nullptr, &g_vulkan.ui_framebuffer));
    VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, g_vulkan.ui_framebuffer, "UI Framebuffer");
}

static void InitComposite() {
    g_vulkan.composite_framebuffers.resize(g_vulkan.swapchain_framebuffers.size());

    for (size_t i = 0; i < g_vulkan.composite_framebuffers.size(); i++) {
        VkImageView attachments[] = { g_vulkan.swapchain_framebuffers[i].view };
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = g_vulkan.composite_render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = g_vulkan.swapchain_extent.width,
            .height = g_vulkan.swapchain_extent.height,
            .layers = 1
        };

        VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &framebuffer_info, nullptr, &g_vulkan.composite_framebuffers[i]));

        Text name = {};
        Format(name, "UI Framebuffer %zu", i);
        VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, g_vulkan.composite_framebuffers[i], name.value);
    }
}

PlatformShader* PlatformCreateShader(
    const void* vertex,
    u32 vertex_size,
    const void* fragment,
    u32 fragment_size,
    ShaderFlags flags,
    const char* name) {

    PlatformShader* shader = static_cast<PlatformShader *>(Alloc(ALLOCATOR_DEFAULT, sizeof(PlatformShader)));
    if (!CreateShaderInternal(shader, vertex, vertex_size, fragment, fragment_size, flags, name)) {
        Free(shader);
        return nullptr;
    }

    return shader;
}

void PlatformBindShader(PlatformShader* shader) {
    vkCmdBindPipeline(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
}

static void RecreateSwapchainObjects() {
    assert(g_vulkan.device);
    assert(g_vulkan.swapchain);

    DestroyFrameBuffers();
    InitSwapchains();

#if 0
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

    if (indices.graphics_family != indices.present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCreateSwapchainKHR(g_vulkan.device, &createInfo, nullptr, &g_vulkan.swapchain);

    g_vulkan.swapchain_extent = extent;
    g_vulkan.swapchain_image_format = surfaceFormat.format;

    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, nullptr);
    g_vulkan.swapchain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &imageCount, g_vulkan.swapchain_images.data());

    // image views
    g_vulkan.swapchain_image_views.resize(g_vulkan.swapchain_images.size());
    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++) {
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

        Text image_name = {};
        Format(image_name, "Swapchain%zu", i);
        VK_NAME(VK_OBJECT_TYPE_IMAGE, g_vulkan.swapchain_images[i], image_name.value);
        VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, g_vulkan.swapchain_image_views[i], image_name.value);
    }
#endif

    InitMSAA();
    InitDepth();
    InitScene();
    InitPostProc();
    InitUI();
    InitComposite();
    InitSwapchainFrameBuffers();
}

void ResizeRenderDriver(const Vec2Int& size) {
    assert(g_vulkan.device);
    assert(g_vulkan.hwnd);
    assert(g_vulkan.swapchain);
    assert(size.x > 0);
    assert(size.y > 0);

    vkDeviceWaitIdle(g_vulkan.device);

    RecreateSwapchainObjects();
}

void PlatformBeginRender() {
    WaitRenderDriver();

    for (u32 i = 0; i < UNIFORM_BUFFER_COUNT; i++)
        g_vulkan.uniform_buffers[i].offset = 0;

    VkResult result = vkAcquireNextImageKHR(
        g_vulkan.device,
        g_vulkan.swapchain,
        UINT64_MAX,
        g_vulkan.image_available_semaphore,
        VK_NULL_HANDLE,
        &g_vulkan.current_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
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
    } else if (result != VK_SUCCESS) {
        Exit("Failed to acquire swapchain image");
    }

    VkCommandBufferBeginInfo vg_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(g_vulkan.command_buffer, &vg_begin_info);
}

void PlatformEndRender() {
    vkEndCommandBuffer(g_vulkan.command_buffer);

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

void PlatformFree(PlatformShader* shader) {
    assert(shader);
    assert(g_vulkan.device);
    if (shader->fragment_module != VK_NULL_HANDLE)
        vkDestroyShaderModule(g_vulkan.device, static_cast<VkShaderModule>(shader->fragment_module), nullptr);
    if (shader->vertex_module != VK_NULL_HANDLE)
        vkDestroyShaderModule(g_vulkan.device, static_cast<VkShaderModule>(shader->vertex_module), nullptr);
    if (shader->pipeline)
        vkDestroyPipeline(g_vulkan.device, static_cast<VkPipeline>(shader->pipeline), nullptr);

    Free(shader);
}

static void InitLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(g_vulkan.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector uniqueQueueFamilies = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family == indices.present_family)
        uniqueQueueFamilies.resize(1);

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies) {
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

    VK_CHECK(vkCreateDevice(g_vulkan.physical_device, &createInfo, nullptr, &g_vulkan.device));

    InitDeviceFunctions(g_vulkan.instance);

    vkGetDeviceQueue(g_vulkan.device, indices.graphics_family, 0, &g_vulkan.graphics_queue);
    vkGetDeviceQueue(g_vulkan.device, indices.present_family, 0, &g_vulkan.present_queue);
}

static void InitInstance() {
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
    if (CheckValidationLayerSupport()) {
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &validationLayer;
    }
#endif

    VK_CHECK(vkCreateInstance(&create_info, nullptr, &g_vulkan.instance));

    InitInstanceFunctions(g_vulkan.instance);
}

void InitRenderDriver(const RendererTraits* traits, HWND hwnd) {
    g_vulkan = {};
    g_vulkan.traits = *traits;
    g_vulkan.hwnd = hwnd;
    g_vulkan.depth_conversion_factor = 1.0f / (traits->max_depth - traits->min_depth);

    if (!InitVulkanLibrary()) {
        Exit("Failed to load Vulkan library. Make sure Vulkan drivers are installed.");
        return;
    }

    InitInstance();
    InitDebugMessenger();
    InitSurface();
    InitPhysicsDevice();
    InitLogicalDevice();
    InitDescriptorSetLayout();
    InitDescriptorPool();
    InitDescriptorSets();
    InitUniformBuffers();
    InitPipeline();
    InitSyncObjects();

    InitSwapchains();
    InitMSAA();
    InitScene();
    InitPostProc();
    InitUI();
    InitComposite();
    InitSwapchainFrameBuffers();
    InitCommandPool();
    InitCommandBuffer();

    InitRenderPass();
    InitSceneRenderPass();
    InitPostProcPass();
    InitUIPass();
    InitCompositePass();

}

void ShutdownRenderDriver() {
    if (g_vulkan.device) {
        vkDeviceWaitIdle(g_vulkan.device);
        vkDestroySemaphore(g_vulkan.device, g_vulkan.render_finished_semaphore, nullptr);
        vkDestroySemaphore(g_vulkan.device, g_vulkan.image_available_semaphore, nullptr);
        vkDestroyFence(g_vulkan.device, g_vulkan.in_flight_fence, nullptr);
        vkDestroyCommandPool(g_vulkan.device, g_vulkan.command_pool, nullptr);

        DestroyFrameBuffers();

        vkDestroyRenderPass(g_vulkan.device, g_vulkan.render_pass, nullptr);
        if (g_vulkan.scene_render_pass)
            vkDestroyRenderPass(g_vulkan.device, g_vulkan.scene_render_pass, nullptr);
        if (g_vulkan.post_proc_render_pass)
            vkDestroyRenderPass(g_vulkan.device, g_vulkan.post_proc_render_pass, nullptr);
        if (g_vulkan.ui_render_pass)
            vkDestroyRenderPass(g_vulkan.device, g_vulkan.ui_render_pass, nullptr);
        if (g_vulkan.composite_render_pass)
            vkDestroyRenderPass(g_vulkan.device, g_vulkan.composite_render_pass, nullptr);
        if (g_vulkan.swapchain)
            vkDestroySwapchainKHR(g_vulkan.device, g_vulkan.swapchain, nullptr);

        vkDestroyDevice(g_vulkan.device, nullptr);
    }

    if (g_vulkan.surface)
        vkDestroySurfaceKHR(g_vulkan.instance, g_vulkan.surface, nullptr);

#ifdef _DEBUG
    if (g_vulkan.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                g_vulkan.instance,
                "vkDestroyDebugUtilsMessengerEXT"));
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
