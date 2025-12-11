//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../vulkan/vulkan_render.h"

const char* VK_UNIFORM_BUFFER_NAMES[] = {
    "CameraBuffer",
    "TransformBuffer",
    "SkeletonBuffer",
    "VertexUserBuffer",
    "ColorBuffer",
    "FragmentUserBuffer"
};

static_assert(sizeof(VK_UNIFORM_BUFFER_NAMES) / sizeof(const char*) == UNIFORM_BUFFER_COUNT);

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



static void InitDescriptorSetLayout(VkShaderStageFlags stage_flags, UniformBufferType buffer_type) {
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
    InitDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_CAMERA);
    InitDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_OBJECT);
    InitDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_SKELETON);
    InitDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT, UNIFORM_BUFFER_VERTEX_USER);
    InitDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_COLOR);
    InitDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_FRAGMENT_USER);
    InitDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT, UNIFORM_BUFFER_FRAGMENT_USER);

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
#endif

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

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
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
    // Prefer non-sRGB format to avoid automatic gamma correction
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
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

static void InitSwapchain() {
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
    g_vulkan.swapchain_images.resize(image_count);

    std::vector<VkImage> swapchain_images(image_count);
    vkGetSwapchainImagesKHR(g_vulkan.device, g_vulkan.swapchain, &image_count, swapchain_images.data());

    g_vulkan.swapchain_image_format = surface_format.format;
    g_vulkan.swapchain_extent = extent;

    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++) {
        SwapchainImage& img = g_vulkan.swapchain_images[i];
        img.image = swapchain_images[i];

        VkImageViewCreateInfo vk_image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img.image,
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

        VK_CHECK(vkCreateImageView(g_vulkan.device, &vk_image_create_info, nullptr, &img.view));

        Text image_name = {};
        Format(image_name, "Swapchain%zu", i);
        VK_NAME(VK_OBJECT_TYPE_IMAGE, img.image, image_name.value);
        VK_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, img.view, image_name.value);
    }
}

static void InitSwapchainFrameBuffers() {
    for (size_t i = 0; i < g_vulkan.swapchain_images.size(); i++) {
        SwapchainImage& img = g_vulkan.swapchain_images[i];
        VkImageView vk_attachments[] = {
            g_vulkan.msaa_target.view,
            img.view,
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
        VK_CHECK(vkCreateFramebuffer(g_vulkan.device, &vk_frame_buffer_info, nullptr, &img.framebuffer));

        Text name = {};
        Format(name, "Swapchain%zu", i);
        VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, img.framebuffer, name.value);
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
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = g_vulkan.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(g_vulkan.device, &alloc_info, &g_vulkan.command_buffer));
}

static void InitSyncObjects() {
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VK_CHECK(vkCreateSemaphore(g_vulkan.device, &semaphore_info, nullptr, &g_vulkan.image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(g_vulkan.device, &semaphore_info, nullptr, &g_vulkan.render_finished_semaphore));
    VK_CHECK(vkCreateFence(g_vulkan.device, &fence_info, nullptr, &g_vulkan.in_flight_fence));
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

static void InitMSAA() {
    OffscreenTarget& target = g_vulkan.msaa_target;
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
    g_vulkan.postprocess_framebuffers.resize(g_vulkan.swapchain_images.size());

    for (size_t i = 0; i < g_vulkan.postprocess_framebuffers.size(); i++) {
        VkImageView attachments[] = { g_vulkan.swapchain_images[i].view };
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
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
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
    g_vulkan.composite_framebuffers.resize(g_vulkan.swapchain_images.size());

    for (size_t i = 0; i < g_vulkan.composite_framebuffers.size(); i++) {
        VkImageView attachments[] = { g_vulkan.swapchain_images[i].view };
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
        Format(name, "Composite Framebuffer %zu", i);
        VK_NAME(VK_OBJECT_TYPE_FRAMEBUFFER, g_vulkan.composite_framebuffers[i], name.value);
    }
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
        .apiVersion = VK_API_VERSION_1_1,
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
    InitUniformBuffers();
    InitDescriptorSets();
    InitPipeline();
    InitCommandPool();
    InitCommandBuffer();
    InitSyncObjects();

    InitSwapchain();
    InitMSAA();
    InitDepth();
    InitRenderPass();
    InitSceneRenderPass();
    InitPostProcPass();
    InitUIPass();
    InitCompositePass();
    InitScene();
    InitPostProc();
    InitUI();
    InitComposite();
    InitSwapchainFrameBuffers();
}

void ReinitSwapchain() {
    assert(g_vulkan.device);
    assert(g_vulkan.swapchain);

    ShutdownFrameBuffers();
    InitSwapchain();
    InitMSAA();
    InitDepth();
    InitScene();
    InitPostProc();
    InitUI();
    InitComposite();
    InitSwapchainFrameBuffers();
}