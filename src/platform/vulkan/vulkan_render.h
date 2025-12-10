//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkDestroyDevice vkDestroyDevice;
extern PFN_vkGetDeviceQueue vkGetDeviceQueue;
extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
extern PFN_vkCreateImageView vkCreateImageView;
extern PFN_vkDestroyImageView vkDestroyImageView;
extern PFN_vkCreateRenderPass vkCreateRenderPass;
extern PFN_vkDestroyRenderPass vkDestroyRenderPass;
extern PFN_vkCreateFramebuffer vkCreateFramebuffer;
extern PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
extern PFN_vkCreateCommandPool vkCreateCommandPool;
extern PFN_vkDestroyCommandPool vkDestroyCommandPool;
extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
extern PFN_vkEndCommandBuffer vkEndCommandBuffer;
extern PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
extern PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
extern PFN_vkCmdSetViewport vkCmdSetViewport;
extern PFN_vkCmdSetScissor vkCmdSetScissor;
extern PFN_vkCreateBuffer vkCreateBuffer;
extern PFN_vkDestroyBuffer vkDestroyBuffer;
extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
extern PFN_vkAllocateMemory vkAllocateMemory;
extern PFN_vkFreeMemory vkFreeMemory;
extern PFN_vkBindBufferMemory vkBindBufferMemory;
extern PFN_vkMapMemory vkMapMemory;
extern PFN_vkUnmapMemory vkUnmapMemory;
extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
extern PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
extern PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
extern PFN_vkCmdDraw vkCmdDraw;
extern PFN_vkCreateSemaphore vkCreateSemaphore;
extern PFN_vkDestroySemaphore vkDestroySemaphore;
extern PFN_vkCreateFence vkCreateFence;
extern PFN_vkDestroyFence vkDestroyFence;
extern PFN_vkWaitForFences vkWaitForFences;
extern PFN_vkResetFences vkResetFences;
extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
extern PFN_vkQueueSubmit vkQueueSubmit;
extern PFN_vkQueuePresentKHR vkQueuePresentKHR;
extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
extern PFN_vkDestroyPipeline vkDestroyPipeline;
extern PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
extern PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
extern PFN_vkCreateShaderModule vkCreateShaderModule;
extern PFN_vkDestroyShaderModule vkDestroyShaderModule;
extern PFN_vkCmdBindPipeline vkCmdBindPipeline;
extern PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
extern PFN_vkCmdPushConstants vkCmdPushConstants;
extern PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
extern PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
extern PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
extern PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
extern PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
extern PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
extern PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
extern PFN_vkCreateImage vkCreateImage;
extern PFN_vkDestroyImage vkDestroyImage;
extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
extern PFN_vkBindImageMemory vkBindImageMemory;
extern PFN_vkCreateSampler vkCreateSampler;
extern PFN_vkDestroySampler vkDestroySampler;
extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
extern PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
extern PFN_vkQueueWaitIdle vkQueueWaitIdle;

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
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    VkCommandBuffer command_buffer;
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
    u32 current_frame;
    VkPipelineLayout pipeline_layout;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkSampleCountFlagBits msaa_samples;
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

struct QueueFamilyIndices {
    u32 graphics_family = UINT32_MAX;
    u32 present_family = UINT32_MAX;

    bool IsComplete() const {
        return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
    }
};

inline VkFilter ToVK(TextureFilter filter) {
    return filter == TEXTURE_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

inline VkSamplerAddressMode ToVK(TextureClamp clamp) {
    return clamp == TEXTURE_CLAMP_REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}

extern bool InitVulkanLibrary();
extern void UnloadVulkanLibrary();
extern void InitInstanceFunctions(VkInstance instance);
extern void InitDeviceFunctions(VkInstance instance);
extern void SetVulkanObjectName(VkObjectType object_type, uint64_t object_handle, const char* name);
extern VkSampleCountFlagBits GetMaxUsableSampleCount();
extern u32 FindMemoryType(u32 type_filter, VkMemoryPropertyFlags properties);
extern QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
extern bool IsDeviceSuitable(VkPhysicalDevice device);
extern SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
extern bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
extern std::vector<const char*> GetRequiredExtensions();
extern bool CheckValidationLayerSupport();
extern void ReinitSwapchain();
extern void ShutdownFrameBuffers();

extern VulkanRenderer g_vulkan;