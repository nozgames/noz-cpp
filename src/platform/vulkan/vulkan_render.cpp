//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include "../vulkan/vulkan_render.h"

VulkanRenderer g_vulkan = {};

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

void ShutdownFrameBuffers() {
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


void ResizeRenderDriver(const Vec2Int& size) {
    assert(g_vulkan.device);
    assert(g_vulkan.hwnd);
    assert(g_vulkan.swapchain);
    assert(size.x > 0);
    assert(size.y > 0);

    vkDeviceWaitIdle(g_vulkan.device);

    ReinitSwapchain();
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
        ReinitSwapchain();
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


void ShutdownRenderDriver() {
    if (g_vulkan.device) {
        vkDeviceWaitIdle(g_vulkan.device);
        vkDestroySemaphore(g_vulkan.device, g_vulkan.render_finished_semaphore, nullptr);
        vkDestroySemaphore(g_vulkan.device, g_vulkan.image_available_semaphore, nullptr);
        vkDestroyFence(g_vulkan.device, g_vulkan.in_flight_fence, nullptr);
        vkDestroyCommandPool(g_vulkan.device, g_vulkan.command_pool, nullptr);

        ShutdownFrameBuffers();

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
