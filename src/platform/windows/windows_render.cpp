//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include "windows_vulkan.h"

// TextureImpl structure to access Vulkan objects
struct TextureImpl : Texture
{
    VkImage vk_image;
    VkImageView vk_image_view;
    VkDeviceMemory vk_memory;
    VkSampler vk_sampler;
    SamplerOptions sampler_options;
    Vec2Int size;
};

extern platform::ShaderModule* GetVertexShader(Shader* shader);
extern platform::ShaderModule* GetFragmentShader(Shader* shader);

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

    VkBuffer camera_uniform_buffer;          // vertex_register_camera (0)
    VkDeviceMemory camera_uniform_memory;
    void* camera_uniform_mapped;
    
    VkBuffer transform_uniform_buffer;       // vertex_register_object (1) 
    VkDeviceMemory transform_uniform_memory;
    void* transform_uniform_mapped;
    
    VkBuffer bones_uniform_buffer;           // vertex_register_bone (2)
    VkDeviceMemory bones_uniform_memory;
    void* bones_uniform_mapped;
    
    VkBuffer color_uniform_buffer;           // fragment_register_color (0)
    VkDeviceMemory color_uniform_memory;
    void* color_uniform_mapped;

    // Descriptor sets for uniform buffers - separate sets for different spaces
    VkDescriptorSetLayout vertex_descriptor_set_layout;    // space1 - vertex uniforms
    VkDescriptorSetLayout texture_descriptor_set_layout;   // space2 - textures/samplers
    VkDescriptorSetLayout fragment_descriptor_set_layout;  // space3 - fragment uniforms
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet vertex_descriptor_set;    // space1 - camera, transform, bones
    VkDescriptorSet texture_descriptor_set;   // space2 - textures/samplers
    VkDescriptorSet fragment_descriptor_set;  // space3 - color, light
    
    // Default white texture and sampler
    VkImage default_texture;
    VkDeviceMemory default_texture_memory;
    VkImageView default_texture_view;
    VkSampler default_sampler;

    platform::Window* window;
    RendererTraits traits;
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

static VulkanRenderer g_vulkan = {};
static uint32_t g_current_image_index = 0;
static VkPipelineLayout g_current_pipeline_layout = VK_NULL_HANDLE;
static HMODULE vulkan_library = nullptr;

// Helper function to find suitable memory type
uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(g_vulkan.physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    // Fallback: just find any available type
    for (uint32_t i = 0; i < 32; i++) {
        if (type_filter & (1 << i)) {
            return i;
        }
    }
    return 0;
}

static bool CreateUniformBuffer(size_t size, VkBuffer* buffer, VkDeviceMemory* memory, void** mapped_ptr, const char* name = nullptr)
{
    // Create buffer
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(g_vulkan.device, &buffer_info, nullptr, buffer) != VK_SUCCESS)
        return false;
    
    // Allocate memory
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, *buffer, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(g_vulkan.device, *buffer, nullptr);
        return false;
    }
    
    vkBindBufferMemory(g_vulkan.device, *buffer, *memory, 0);
    
    // Map memory persistently
    if (vkMapMemory(g_vulkan.device, *memory, 0, size, 0, mapped_ptr) != VK_SUCCESS)
    {
        vkFreeMemory(g_vulkan.device, *memory, nullptr);
        vkDestroyBuffer(g_vulkan.device, *buffer, nullptr);
        return false;
    }
    
    // Set debug name for RenderDoc/debugging tools
    if (name && g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_BUFFER;
        name_info.objectHandle = (uint64_t)*buffer;
        name_info.pObjectName = name;
        
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
    
    return true;
}

static void CreateDefaultTexture()
{
    // Create 1x1 white texture
    uint32_t white_pixel = 0xFFFFFFFF;
    
    // First, create a staging buffer to upload the pixel data
    VkBufferCreateInfo staging_buffer_info = {};
    staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_info.size = sizeof(uint32_t);
    staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer staging_buffer;
    if (vkCreateBuffer(g_vulkan.device, &staging_buffer_info, nullptr, &staging_buffer) != VK_SUCCESS)
        Exit("Failed to create staging buffer for default texture");
    
    // Allocate memory for staging buffer
    VkMemoryRequirements staging_mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, staging_buffer, &staging_mem_requirements);
    
    VkMemoryAllocateInfo staging_alloc_info = {};
    staging_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc_info.allocationSize = staging_mem_requirements.size;
    staging_alloc_info.memoryTypeIndex = FindMemoryType(staging_mem_requirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VkDeviceMemory staging_memory;
    if (vkAllocateMemory(g_vulkan.device, &staging_alloc_info, nullptr, &staging_memory) != VK_SUCCESS)
        Exit("Failed to allocate staging buffer memory");
    
    vkBindBufferMemory(g_vulkan.device, staging_buffer, staging_memory, 0);
    
    // Copy white pixel to staging buffer
    void* data;
    if (vkMapMemory(g_vulkan.device, staging_memory, 0, sizeof(uint32_t), 0, &data) == VK_SUCCESS)
    {
        memcpy(data, &white_pixel, sizeof(uint32_t));
        vkUnmapMemory(g_vulkan.device, staging_memory);
    }
    
    // Create image
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = 1;
    image_info.extent.height = 1;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(g_vulkan.device, &image_info, nullptr, &g_vulkan.default_texture) != VK_SUCCESS)
        Exit("Failed to create default texture");
    
    // Allocate memory for image
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, g_vulkan.default_texture, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &g_vulkan.default_texture_memory) != VK_SUCCESS)
        Exit("Failed to allocate default texture memory");
    
    vkBindImageMemory(g_vulkan.device, g_vulkan.default_texture, g_vulkan.default_texture_memory, 0);
    
    // Record commands to copy from staging buffer to image
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(g_vulkan.command_buffer, &begin_info);
    
    // Transition image to transfer destination layout
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = g_vulkan.default_texture;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(g_vulkan.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    // Copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {1, 1, 1};
    
    vkCmdCopyBufferToImage(g_vulkan.command_buffer, staging_buffer, g_vulkan.default_texture,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    // Transition image to shader read optimal layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(g_vulkan.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(g_vulkan.command_buffer);
    
    // Submit commands
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &g_vulkan.command_buffer;
    
    vkQueueSubmit(g_vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_vulkan.graphics_queue);
    
    // Cleanup staging resources
    vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);
    vkFreeMemory(g_vulkan.device, staging_memory, nullptr);
    
    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = g_vulkan.default_texture;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(g_vulkan.device, &view_info, nullptr, &g_vulkan.default_texture_view) != VK_SUCCESS)
        Exit("Failed to create default texture image view");
    
    // Set debug name
    if (g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_IMAGE;
        name_info.objectHandle = (uint64_t)g_vulkan.default_texture;
        name_info.pObjectName = "DefaultWhiteTexture";
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
}

static void CreateDefaultSampler()
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    
    if (vkCreateSampler(g_vulkan.device, &sampler_info, nullptr, &g_vulkan.default_sampler) != VK_SUCCESS)
        Exit("Failed to create default sampler");
    
    // Set debug name
    if (g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_SAMPLER;
        name_info.objectHandle = (uint64_t)g_vulkan.default_sampler;
        name_info.pObjectName = "DefaultSampler";
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
}

static void CreateDescriptorSetLayout()
{
    // Create vertex descriptor set layout (space1)
    VkDescriptorSetLayoutBinding vertex_bindings[3] = {};
    
    // vertex_register_camera (0) - Camera uniform
    vertex_bindings[0].binding = 0;
    vertex_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vertex_bindings[0].descriptorCount = 1;
    vertex_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_bindings[0].pImmutableSamplers = nullptr;
    
    // vertex_register_object (1) - Transform uniform
    vertex_bindings[1].binding = 1;
    vertex_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vertex_bindings[1].descriptorCount = 1;
    vertex_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_bindings[1].pImmutableSamplers = nullptr;
    
    // vertex_register_bone (2) - Bones uniform
    vertex_bindings[2].binding = 2;
    vertex_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vertex_bindings[2].descriptorCount = 1;
    vertex_bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_bindings[2].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo vertex_layout_info = {};
    vertex_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vertex_layout_info.bindingCount = 3;
    vertex_layout_info.pBindings = vertex_bindings;
    
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &vertex_layout_info, nullptr, &g_vulkan.vertex_descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create vertex descriptor set layout");
    
    // Create texture descriptor set layout (space2)
    VkDescriptorSetLayoutBinding texture_bindings[4] = {}; // 4 texture slots: shadow_map + user0/1/2
    
    // sampler_register_shadow_map (0) - Shadow map texture
    texture_bindings[0].binding = 0;
    texture_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[0].descriptorCount = 1;
    texture_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_bindings[0].pImmutableSamplers = nullptr;
    
    // sampler_register_user0 (1) - User texture 0
    texture_bindings[1].binding = 1;
    texture_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[1].descriptorCount = 1;
    texture_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_bindings[1].pImmutableSamplers = nullptr;
    
    // sampler_register_user1 (2) - User texture 1
    texture_bindings[2].binding = 2;
    texture_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[2].descriptorCount = 1;
    texture_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_bindings[2].pImmutableSamplers = nullptr;
    
    // sampler_register_user2 (3) - User texture 2
    texture_bindings[3].binding = 3;
    texture_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[3].descriptorCount = 1;
    texture_bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_bindings[3].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo texture_layout_info = {};
    texture_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_layout_info.bindingCount = 4;
    texture_layout_info.pBindings = texture_bindings;
    
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &texture_layout_info, nullptr, &g_vulkan.texture_descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create texture descriptor set layout");
    
    // Create fragment descriptor set layout (space3)
    VkDescriptorSetLayoutBinding fragment_bindings[1] = {};
    
    // fragment_register_color (0) - Color uniform
    fragment_bindings[0].binding = 0;
    fragment_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fragment_bindings[0].descriptorCount = 1;
    fragment_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_bindings[0].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo fragment_layout_info = {};
    fragment_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    fragment_layout_info.bindingCount = 1;
    fragment_layout_info.pBindings = fragment_bindings;
    
    if (vkCreateDescriptorSetLayout(g_vulkan.device, &fragment_layout_info, nullptr, &g_vulkan.fragment_descriptor_set_layout) != VK_SUCCESS)
        Exit("Failed to create fragment descriptor set layout");
}

static void CreateDescriptorPool()
{
    VkDescriptorPoolSize pool_sizes[2] = {};
    
    // Uniform buffers pool
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 4; // 4 uniform buffers total
    
    // Combined image samplers pool  
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 4; // 4 texture slots total
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = 3; // Three descriptor sets: vertex (space1), textures (space2), fragment (space3)
    
    if (vkCreateDescriptorPool(g_vulkan.device, &pool_info, nullptr, &g_vulkan.descriptor_pool) != VK_SUCCESS)
        Exit("Failed to create descriptor pool");
}

static void CreateDescriptorSets()
{
    // Allocate all three descriptor sets
    VkDescriptorSetLayout layouts[3] = {
        g_vulkan.vertex_descriptor_set_layout, 
        g_vulkan.texture_descriptor_set_layout,
        g_vulkan.fragment_descriptor_set_layout
    };
    VkDescriptorSet descriptor_sets[3];
    
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = g_vulkan.descriptor_pool;
    alloc_info.descriptorSetCount = 3;
    alloc_info.pSetLayouts = layouts;
    
    if (vkAllocateDescriptorSets(g_vulkan.device, &alloc_info, descriptor_sets) != VK_SUCCESS)
        Exit("Failed to allocate descriptor sets");
    
    g_vulkan.vertex_descriptor_set = descriptor_sets[0];
    g_vulkan.texture_descriptor_set = descriptor_sets[1];
    g_vulkan.fragment_descriptor_set = descriptor_sets[2];
    
    // Update vertex descriptor set (space1) - camera, transform, bones
    VkDescriptorBufferInfo vertex_buffer_infos[3] = {};
    
    // Camera buffer (binding 0)
    vertex_buffer_infos[0].buffer = g_vulkan.camera_uniform_buffer;
    vertex_buffer_infos[0].offset = 0;
    vertex_buffer_infos[0].range = sizeof(RenderCamera);
    
    // Transform buffer (binding 1)
    vertex_buffer_infos[1].buffer = g_vulkan.transform_uniform_buffer;
    vertex_buffer_infos[1].offset = 0;
    vertex_buffer_infos[1].range = sizeof(RenderTransform);
    
    // Bones buffer (binding 2)
    vertex_buffer_infos[2].buffer = g_vulkan.bones_uniform_buffer;
    vertex_buffer_infos[2].offset = 0;
    vertex_buffer_infos[2].range = sizeof(RenderTransform) * 64;
    
    VkWriteDescriptorSet vertex_writes[3] = {};
    for (int i = 0; i < 3; i++)
    {
        vertex_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertex_writes[i].dstSet = g_vulkan.vertex_descriptor_set;
        vertex_writes[i].dstBinding = i;
        vertex_writes[i].dstArrayElement = 0;
        vertex_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vertex_writes[i].descriptorCount = 1;
        vertex_writes[i].pBufferInfo = &vertex_buffer_infos[i];
    }
    
    // Update texture descriptor set (space2) - all texture slots with default texture
    VkDescriptorImageInfo texture_image_infos[4] = {};
    
    for (int i = 0; i < 4; i++)
    {
        texture_image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture_image_infos[i].imageView = g_vulkan.default_texture_view;
        texture_image_infos[i].sampler = g_vulkan.default_sampler;
    }
    
    VkWriteDescriptorSet texture_writes[4] = {};
    for (int i = 0; i < 4; i++)
    {
        texture_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texture_writes[i].dstSet = g_vulkan.texture_descriptor_set;
        texture_writes[i].dstBinding = i;
        texture_writes[i].dstArrayElement = 0;
        texture_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_writes[i].descriptorCount = 1;
        texture_writes[i].pImageInfo = &texture_image_infos[i];
    }
    
    // Update fragment descriptor set (space3) - color
    VkDescriptorBufferInfo fragment_buffer_info = {};
    fragment_buffer_info.buffer = g_vulkan.color_uniform_buffer;
    fragment_buffer_info.offset = 0;
    fragment_buffer_info.range = 16;
    
    VkWriteDescriptorSet fragment_write = {};
    fragment_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    fragment_write.dstSet = g_vulkan.fragment_descriptor_set;
    fragment_write.dstBinding = 0;
    fragment_write.dstArrayElement = 0;
    fragment_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fragment_write.descriptorCount = 1;
    fragment_write.pBufferInfo = &fragment_buffer_info;
    
    // Update all descriptor sets
    VkWriteDescriptorSet all_writes[8]; // 3 vertex + 4 texture + 1 fragment
    memcpy(all_writes, vertex_writes, sizeof(vertex_writes));
    memcpy(all_writes + 3, texture_writes, sizeof(texture_writes));
    all_writes[7] = fragment_write;
    
    vkUpdateDescriptorSets(g_vulkan.device, 8, all_writes, 0, nullptr);
    
    // Set debug names for descriptor sets
    if (g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
        
        // Vertex descriptor set (space1)
        name_info.objectHandle = (uint64_t)g_vulkan.vertex_descriptor_set;
        name_info.pObjectName = "VertexUniforms_Space1";
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
        
        // Texture descriptor set (space2)
        name_info.objectHandle = (uint64_t)g_vulkan.texture_descriptor_set;
        name_info.pObjectName = "TextureSamplers_Space2";
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
        
        // Fragment descriptor set (space3)
        name_info.objectHandle = (uint64_t)g_vulkan.fragment_descriptor_set;
        name_info.pObjectName = "FragmentUniforms_Space3";
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
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
    LoadDeviceFunctions(g_vulkan.instance);

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
    // Force VSync by always returning FIFO mode (guaranteed to be available)
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
        VkImageViewCreateInfo vk_image_create_info = {};
        vk_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vk_image_create_info.image = g_vulkan.swapchain_images[i];
        vk_image_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vk_image_create_info.format = g_vulkan.swapchain_image_format;
        vk_image_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_image_create_info.subresourceRange.baseMipLevel = 0;
        vk_image_create_info.subresourceRange.levelCount = 1;
        vk_image_create_info.subresourceRange.baseArrayLayer = 0;
        vk_image_create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(g_vulkan.device, &vk_image_create_info, nullptr, &g_vulkan.swapchain_image_views[i]);
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
        VkImageView vk_attachments[] = {g_vulkan.swapchain_image_views[i]};

        VkFramebufferCreateInfo vk_frame_buffer_info = {};
        vk_frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vk_frame_buffer_info.renderPass = g_vulkan.render_pass;
        vk_frame_buffer_info.attachmentCount = 1;
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
    
    // Create uniform buffers
    CreateUniformBuffer(sizeof(RenderCamera), &g_vulkan.camera_uniform_buffer, &g_vulkan.camera_uniform_memory, &g_vulkan.camera_uniform_mapped, "CameraUniformBuffer");
    CreateUniformBuffer(sizeof(RenderTransform), &g_vulkan.transform_uniform_buffer, &g_vulkan.transform_uniform_memory, &g_vulkan.transform_uniform_mapped, "TransformUniformBuffer");
    CreateUniformBuffer(sizeof(RenderTransform) * 64, &g_vulkan.bones_uniform_buffer, &g_vulkan.bones_uniform_memory, &g_vulkan.bones_uniform_mapped, "BonesUniformBuffer"); // Max 64 bones
    CreateUniformBuffer(16, &g_vulkan.color_uniform_buffer, &g_vulkan.color_uniform_memory, &g_vulkan.color_uniform_mapped, "ColorUniformBuffer"); // Color data

    // Create default texture and sampler
    CreateDefaultTexture();
    CreateDefaultSampler();
    
    // Create descriptor set layout, pool, and sets
    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    CreateDescriptorSets();

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

VkCommandPool GetVulkanCommandPool()
{
    return g_vulkan.command_pool;
}

VkQueue GetVulkanGraphicsQueue()
{
    return g_vulkan.graphics_queue;
}

platform::Pipeline* platform::CreatePipeline(Shader* shader)
{
    if (!g_vulkan.device)
        return nullptr;

    VkDescriptorSetLayout layouts[4] = {
        VK_NULL_HANDLE,                               // space0 - unused
        g_vulkan.vertex_descriptor_set_layout,       // space1 - vertex uniforms
        g_vulkan.texture_descriptor_set_layout,      // space2 - textures/samplers
        g_vulkan.fragment_descriptor_set_layout      // space3 - fragment uniforms
    };
    
    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 4;
    layout_info.pSetLayouts = layouts;
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
    rasterizer.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT; // TODO: Get from shader
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;

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
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    // Dynamic state
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    // Get shader modules from shader object
    ShaderModule* vertex_module = GetVertexShader(shader);
    ShaderModule* fragment_module = GetFragmentShader(shader);
    
    if (!vertex_module || !fragment_module)
        return nullptr;

    VkPipelineShaderStageCreateInfo shader_stages[2] = {};
    
    // Vertex stage
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = (VkShaderModule)vertex_module;
    shader_stages[0].pName = "vs";
    
    // Fragment stage  
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = (VkShaderModule)fragment_module;
    shader_stages[1].pName = "ps";

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
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

    // Set debug name for the pipeline
    if (shader && g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        const Name* shader_name = GetName(shader);
        const char* name_string = GetValue(shader_name, "UnnamedPipeline");
        
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_PIPELINE;
        name_info.objectHandle = (uint64_t)pipeline;
        name_info.pObjectName = name_string;
        
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }

    // Store the layout globally so we can access it for descriptor set binding
    // TODO: Redesign API to properly associate layouts with pipelines
    g_current_pipeline_layout = layout;
    
    // Return the VkPipeline cast as PlatformPipeline (opaque handle)
    return (Pipeline*)pipeline;
}

void platform::DestroyPipeline(Pipeline* pipeline)
{
    assert(pipeline);
    assert(g_vulkan.device);

    vkDestroyPipeline(g_vulkan.device, (VkPipeline)pipeline, nullptr);
}

void platform::BindPipeline(Pipeline* pipeline)
{
    assert(pipeline);
    assert(g_vulkan.command_buffer);

    vkCmdBindPipeline(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline);
    
    // Bind descriptor sets after pipeline is bound
    if (g_current_pipeline_layout != VK_NULL_HANDLE)
    {
        // Bind vertex uniforms to space1
        vkCmdBindDescriptorSets(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                               g_current_pipeline_layout, 1, 1, &g_vulkan.vertex_descriptor_set, 0, nullptr);
        
        // Bind textures/samplers to space2
        vkCmdBindDescriptorSets(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                               g_current_pipeline_layout, 2, 1, &g_vulkan.texture_descriptor_set, 0, nullptr);
        
        // Bind fragment uniforms to space3  
        vkCmdBindDescriptorSets(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                               g_current_pipeline_layout, 3, 1, &g_vulkan.fragment_descriptor_set, 0, nullptr);
    }
}

void platform::BeginRenderFrame()
{
    vkWaitForFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_vulkan.device, 1, &g_vulkan.in_flight_fence);

    VkResult result = vkAcquireNextImageKHR(
        g_vulkan.device,
        g_vulkan.swapchain,
        UINT64_MAX,
        g_vulkan.image_available_semaphore,
        VK_NULL_HANDLE,
        &g_current_image_index);

    // Handle swapchain out of date (window was resized while rendering)
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        RecreateSwapchainObjects();
        result = vkAcquireNextImageKHR(g_vulkan.device, g_vulkan.swapchain, UINT64_MAX,
                                       g_vulkan.image_available_semaphore, VK_NULL_HANDLE, &g_current_image_index);

        // If still failing after recreation, something is seriously wrong
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            Exit("Failed to acquire swapchain image after recreation");
        }
    }
    else if (result != VK_SUCCESS)
    {
        Exit("Failed to acquire swapchain image");
    }

    VkCommandBufferBeginInfo vg_begin_info = {};
    vg_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
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

    VkSubmitInfo vk_submit_info = {};
    vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vk_submit_info.waitSemaphoreCount = 1;
    vk_submit_info.pWaitSemaphores = vk_wait_semaphores;
    vk_submit_info.pWaitDstStageMask = vk_wait_stages;
    vk_submit_info.commandBufferCount = 1;
    vk_submit_info.pCommandBuffers = &g_vulkan.command_buffer;
    vk_submit_info.signalSemaphoreCount = 1;
    vk_submit_info.pSignalSemaphores = vk_signal_semaphores;

    vkQueueSubmit(g_vulkan.graphics_queue, 1, &vk_submit_info, g_vulkan.in_flight_fence);

    // Present
    VkSwapchainKHR vk_swap_chains[] = {g_vulkan.swapchain};
    VkPresentInfoKHR vk_parent_info = {};
    vk_parent_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vk_parent_info.waitSemaphoreCount = 1;
    vk_parent_info.pWaitSemaphores = vk_signal_semaphores;
    vk_parent_info.swapchainCount = 1;
    vk_parent_info.pSwapchains = vk_swap_chains;
    vk_parent_info.pImageIndices = &g_current_image_index;

    vkQueuePresentKHR(g_vulkan.present_queue, &vk_parent_info);
}

void platform::BindTransform(const RenderTransform* transform)
{
    assert(transform);
    assert(g_vulkan.transform_uniform_mapped);
    
    // Copy transform data to vertex_register_object (1) uniform buffer
    memcpy(g_vulkan.transform_uniform_mapped, transform, sizeof(RenderTransform));
    
    // Flush memory to make sure GPU sees the changes
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = g_vulkan.transform_uniform_memory;
    memory_range.offset = 0;
    memory_range.size = sizeof(RenderTransform);
    vkFlushMappedMemoryRanges(g_vulkan.device, 1, &memory_range);
}

void platform::BindCamera(const RenderCamera* camera)
{
    assert(camera);
    assert(g_vulkan.camera_uniform_mapped);
    
    // Copy camera data to vertex_register_camera (0) uniform buffer
    memcpy(g_vulkan.camera_uniform_mapped, camera, sizeof(RenderCamera));
    
    // Flush memory to make sure GPU sees the changes
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = g_vulkan.camera_uniform_memory;
    memory_range.offset = 0;
    memory_range.size = sizeof(RenderCamera);
    vkFlushMappedMemoryRanges(g_vulkan.device, 1, &memory_range);
}

void platform::BindBoneTransforms(const RenderTransform* bones, int count)
{
    assert(bones);
    assert(count > 0);
    assert(count <= 64); // Max bone count
    assert(g_vulkan.bones_uniform_mapped);
    
    // Copy bone transforms to vertex_register_bone (2) uniform buffer
    size_t bone_size = sizeof(RenderTransform) * count;
    memcpy(g_vulkan.bones_uniform_mapped, bones, bone_size);
    
    // Flush memory to make sure GPU sees the changes
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = g_vulkan.bones_uniform_memory;
    memory_range.offset = 0;
    memory_range.size = bone_size;
    vkFlushMappedMemoryRanges(g_vulkan.device, 1, &memory_range);
}

void platform::BindLight(const void* light)
{
    assert(light);
    
    // TODO: Create light uniform buffer when needed for fragment_register_light (1)
    (void)light; // Suppress unused parameter warning for now
}

void platform::BindColor(const void* color)
{
    assert(color);
    assert(g_vulkan.color_uniform_mapped);
    
    // Copy color data to fragment_register_color (0) uniform buffer
    memcpy(g_vulkan.color_uniform_mapped, color, 16); // Assume 16-byte color structure
    
    // Flush memory to make sure GPU sees the changes
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = g_vulkan.color_uniform_memory;
    memory_range.offset = 0;
    memory_range.size = 16;
    vkFlushMappedMemoryRanges(g_vulkan.device, 1, &memory_range);
}

platform::Buffer* platform::CreateVertexBuffer(const MeshVertex* vertices, size_t vertex_count, const char* name)
{
    assert(vertices);
    assert(vertex_count > 0);
    assert(g_vulkan.device != VK_NULL_HANDLE);

    VkBufferCreateInfo vk_buffer_info = {};
    vk_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vk_buffer_info.size = vertex_count * sizeof(MeshVertex);
    vk_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vk_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer vf_vertex_buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(g_vulkan.device, &vk_buffer_info, nullptr, &vf_vertex_buffer) != VK_SUCCESS)
        return nullptr;
    
    VkMemoryRequirements vk_mem_reqs = {};
    vkGetBufferMemoryRequirements(g_vulkan.device, vf_vertex_buffer, &vk_mem_reqs);
    
    VkMemoryAllocateInfo vk_alloc_info = {};
    vk_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vk_alloc_info.allocationSize = vk_mem_reqs.size;
    vk_alloc_info.memoryTypeIndex = FindMemoryType(vk_mem_reqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VkDeviceMemory vk_vertex_memory = VK_NULL_HANDLE;
    if (vkAllocateMemory(g_vulkan.device, &vk_alloc_info, nullptr, &vk_vertex_memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(g_vulkan.device, vf_vertex_buffer, nullptr);
        return nullptr;
    }
    
    vkBindBufferMemory(g_vulkan.device, vf_vertex_buffer, vk_vertex_memory, 0);
    
    // Copy vertex data
    void* data;
    if (vkMapMemory(g_vulkan.device, vk_vertex_memory, 0, vk_buffer_info.size, 0, &data) == VK_SUCCESS)
    {
        memcpy(data, vertices, vk_buffer_info.size);
        vkUnmapMemory(g_vulkan.device, vk_vertex_memory);
    }
    
    // Set debug name for RenderDoc/debugging tools
    if (name && g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_BUFFER;
        name_info.objectHandle = (uint64_t)vf_vertex_buffer;
        name_info.pObjectName = name;
        
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
    
    return (Buffer*)vf_vertex_buffer;
}

platform::Buffer* platform::CreateIndexBuffer(const uint16_t* indices, size_t index_count, const char* name)
{
    assert(indices);
    assert(index_count > 0);

    VkBufferCreateInfo vk_buffer_info = {};
    vk_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vk_buffer_info.size = index_count * sizeof(uint16_t);
    vk_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vk_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer index_buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(g_vulkan.device, &vk_buffer_info, nullptr, &index_buffer) != VK_SUCCESS)
        return nullptr;
    
    VkMemoryRequirements vk_mem_reqs = {};
    vkGetBufferMemoryRequirements(g_vulkan.device, index_buffer, &vk_mem_reqs);
    
    VkMemoryAllocateInfo vk_alloc_info = {};
    vk_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vk_alloc_info.allocationSize = vk_mem_reqs.size;
    vk_alloc_info.memoryTypeIndex = FindMemoryType(vk_mem_reqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
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
    
    // Set debug name for RenderDoc/debugging tools
    if (name && g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_BUFFER;
        name_info.objectHandle = (uint64_t)index_buffer;
        name_info.pObjectName = name;
        
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
    
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

bool platform::CreatePlatformTexture(Texture* texture, void* data, size_t width, size_t height, int channels, const char* name)
{
    if (!texture || !data)
        return false;
    
    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    const size_t size = width * height * channels;

    // Create staging buffer for texture data upload
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(g_vulkan.device, &buffer_info, nullptr, &staging_buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, staging_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(g_vulkan.device, &alloc_info, nullptr, &staging_buffer_memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);
        return false;
    }

    vkBindBufferMemory(g_vulkan.device, staging_buffer, staging_buffer_memory, 0);

    // Map staging buffer and copy texture data
    void* mapped_data;
    vkMapMemory(g_vulkan.device, staging_buffer_memory, 0, size, 0, &mapped_data);
    memcpy(mapped_data, data, size);
    vkUnmapMemory(g_vulkan.device, staging_buffer_memory);

    // Create VkImage
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = (channels == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(g_vulkan.device, &image_info, nullptr, &impl->vk_image) != VK_SUCCESS)
    {
        vkFreeMemory(g_vulkan.device, staging_buffer_memory, nullptr);
        vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);
        return false;
    }

    // Allocate memory for image
    VkMemoryRequirements image_mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan.device, impl->vk_image, &image_mem_requirements);

    VkMemoryAllocateInfo image_alloc_info = {};
    image_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_alloc_info.allocationSize = image_mem_requirements.size;
    image_alloc_info.memoryTypeIndex = FindMemoryType(image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(g_vulkan.device, &image_alloc_info, nullptr, &impl->vk_memory) != VK_SUCCESS)
    {
        vkDestroyImage(g_vulkan.device, impl->vk_image, nullptr);
        vkFreeMemory(g_vulkan.device, staging_buffer_memory, nullptr);
        vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);
        return false;
    }

    vkBindImageMemory(g_vulkan.device, impl->vk_image, impl->vk_memory, 0);

    // Create command buffer for texture upload
    VkCommandBufferAllocateInfo cmd_alloc_info = {};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandPool = g_vulkan.command_pool;
    cmd_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(g_vulkan.device, &cmd_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    // Transition image to transfer destination layout
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = impl->vk_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, impl->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to shader read optimal layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(command_buffer);

    // Submit command buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(g_vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_vulkan.graphics_queue);

    // Cleanup staging buffer
    vkFreeMemory(g_vulkan.device, staging_buffer_memory, nullptr);
    vkDestroyBuffer(g_vulkan.device, staging_buffer, nullptr);

    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = impl->vk_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = (channels == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(g_vulkan.device, &view_info, nullptr, &impl->vk_image_view) != VK_SUCCESS)
    {
        vkFreeMemory(g_vulkan.device, impl->vk_memory, nullptr);
        vkDestroyImage(g_vulkan.device, impl->vk_image, nullptr);
        return false;
    }

    // Create sampler based on texture filtering options
    SamplerOptions sampler_opts = GetSamplerOptions(texture);
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = (sampler_opts.mag_filter == TEXTURE_FILTER_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler_info.minFilter = (sampler_opts.min_filter == TEXTURE_FILTER_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler_info.addressModeU = (sampler_opts.clamp_u == TEXTURE_CLAMP_REPEAT) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = (sampler_opts.clamp_v == TEXTURE_CLAMP_REPEAT) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = (sampler_opts.clamp_w == TEXTURE_CLAMP_REPEAT) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(g_vulkan.device, &sampler_info, nullptr, &impl->vk_sampler) != VK_SUCCESS)
    {
        vkDestroyImageView(g_vulkan.device, impl->vk_image_view, nullptr);
        vkFreeMemory(g_vulkan.device, impl->vk_memory, nullptr);
        vkDestroyImage(g_vulkan.device, impl->vk_image, nullptr);
        return false;
    }

    return true;
}

void platform::DestroyPlatformTexture(Texture* texture)
{
    if (!texture)
        return;
        
    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    
    if (impl->vk_sampler != VK_NULL_HANDLE)
        vkDestroySampler(g_vulkan.device, impl->vk_sampler, nullptr);
    if (impl->vk_image_view != VK_NULL_HANDLE)
        vkDestroyImageView(g_vulkan.device, impl->vk_image_view, nullptr);
    if (impl->vk_memory != VK_NULL_HANDLE)
        vkFreeMemory(g_vulkan.device, impl->vk_memory, nullptr);
    if (impl->vk_image != VK_NULL_HANDLE)
        vkDestroyImage(g_vulkan.device, impl->vk_image, nullptr);
        
    impl->vk_sampler = VK_NULL_HANDLE;
    impl->vk_image_view = VK_NULL_HANDLE;
    impl->vk_memory = VK_NULL_HANDLE;
    impl->vk_image = VK_NULL_HANDLE;
}

platform::ImageView* platform::GetTextureImageView(Texture* texture)
{
    if (!texture)
        return nullptr;
    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    return (platform::ImageView*)impl->vk_image_view;
}

platform::Sampler* platform::GetTextureSampler(Texture* texture)
{
    if (!texture)
        return nullptr;
    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    return (platform::Sampler*)impl->vk_sampler;
}

void platform::BindTexture(Texture* texture, int slot)
{
    // Validate slot is in valid range
    if (slot < 0 || slot >= 4)
        return;
    
    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    // Use actual texture if available, otherwise fall back to default white texture
    VkImageView texture_view = (VkImageView)GetTextureImageView(texture);
    VkSampler sampler = (VkSampler)GetTextureSampler(texture);
    
    if (texture_view != VK_NULL_HANDLE && sampler != VK_NULL_HANDLE)
    {
        image_info.imageView = texture_view;
        image_info.sampler = sampler;
        // Using actual texture
    }
    else
    {
        // Fall back to default white texture
        image_info.imageView = g_vulkan.default_texture_view;
        image_info.sampler = g_vulkan.default_sampler;
    }
    
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = g_vulkan.texture_descriptor_set;
    descriptor_write.dstBinding = slot;  // Maps to sampler register (0=shadow, 1=user0, 2=user1, 3=user2)
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(g_vulkan.device, 1, &descriptor_write, 0, nullptr);
}

void platform::BeginRenderPass(Color clear_color)
{
    if (!g_vulkan.command_buffer)
        return;
        
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = g_vulkan.render_pass;
    
    if (g_current_image_index < g_vulkan.swapchain_framebuffers.size())
        renderPassInfo.framebuffer = g_vulkan.swapchain_framebuffers[g_current_image_index];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = g_vulkan.swapchain_extent;
    
    VkClearValue vk_clear_color = {};
    if (clear_color.a > 0)
    {
        vk_clear_color.color = {clear_color.r, clear_color.g, clear_color.b, clear_color.a};
        renderPassInfo.pClearValues = &vk_clear_color;
        renderPassInfo.clearValueCount = 1;
    }
    else
    {
        renderPassInfo.clearValueCount = 0;
    }

    vkCmdBeginRenderPass(g_vulkan.command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Set dynamic viewport and scissor since pipeline uses dynamic state
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)g_vulkan.swapchain_extent.width;
    viewport.height = (float)g_vulkan.swapchain_extent.height;
    viewport.minDepth = -1.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(g_vulkan.command_buffer, 0, 1, &viewport);
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan.swapchain_extent;
    vkCmdSetScissor(g_vulkan.command_buffer, 0, 1, &scissor);
    
    // Bind descriptor sets with uniform buffers - this needs to be done after a pipeline is bound
    // For now, we'll bind it here but ideally should be bound after pipeline binding
    // vkCmdBindDescriptorSets(g_vulkan.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &g_vulkan.descriptor_set, 0, nullptr);
}

void platform::EndRenderPass()
{
    vkCmdEndRenderPass(g_vulkan.command_buffer);
}

platform::ShaderModule* platform::CreateShaderModule(const void* spirv_code, size_t code_size, const char* name)
{
    if (!spirv_code || code_size == 0)
        return nullptr;
        
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = (const uint32_t*)spirv_code;
    
    VkShaderModule shader_module;
    if (vkCreateShaderModule(g_vulkan.device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
        return nullptr;
    
    // Set debug name for RenderDoc/debugging tools
    if (name && g_vulkan.device && vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
        name_info.objectHandle = (uint64_t)shader_module;
        name_info.pObjectName = name;
        
        vkSetDebugUtilsObjectNameEXT(g_vulkan.device, &name_info);
    }
        
    return (ShaderModule*)shader_module;
}

void platform::DestroyShaderModule(ShaderModule* module)
{
    assert(module);
    assert(g_vulkan.device);
    vkDestroyShaderModule(g_vulkan.device, (VkShaderModule)module, nullptr);
}
