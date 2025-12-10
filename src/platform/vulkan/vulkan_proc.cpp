//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../vulkan/vulkan_render.h"

void InitInstanceFunctions(VkInstance instance) {
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
    vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
}

void InitDeviceFunctions(VkInstance instance) {
    vkDestroyDevice = (PFN_vkDestroyDevice)vkGetInstanceProcAddr(instance, "vkDestroyDevice");
    vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetInstanceProcAddr(instance, "vkGetDeviceQueue");
    vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkGetInstanceProcAddr(instance, "vkDeviceWaitIdle");
    vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR");
    vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(instance, "vkDestroySwapchainKHR");
    vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR");
    vkCreateImageView = (PFN_vkCreateImageView)vkGetInstanceProcAddr(instance, "vkCreateImageView");
    vkDestroyImageView = (PFN_vkDestroyImageView)vkGetInstanceProcAddr(instance, "vkDestroyImageView");
    vkCreateRenderPass = (PFN_vkCreateRenderPass)vkGetInstanceProcAddr(instance, "vkCreateRenderPass");
    vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkGetInstanceProcAddr(instance, "vkDestroyRenderPass");
    vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkGetInstanceProcAddr(instance, "vkCreateFramebuffer");
    vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkGetInstanceProcAddr(instance, "vkDestroyFramebuffer");
    vkCreateCommandPool = (PFN_vkCreateCommandPool)vkGetInstanceProcAddr(instance, "vkCreateCommandPool");
    vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vkGetInstanceProcAddr(instance, "vkDestroyCommandPool");
    vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vkGetInstanceProcAddr(instance, "vkAllocateCommandBuffers");
    vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetInstanceProcAddr(instance, "vkBeginCommandBuffer");
    vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetInstanceProcAddr(instance, "vkEndCommandBuffer");
    vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderPass");
    vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetInstanceProcAddr(instance, "vkCmdEndRenderPass");
    vkCmdSetViewport = (PFN_vkCmdSetViewport)vkGetInstanceProcAddr(instance, "vkCmdSetViewport");
    vkCmdSetScissor = (PFN_vkCmdSetScissor)vkGetInstanceProcAddr(instance, "vkCmdSetScissor");
    vkCreateBuffer = (PFN_vkCreateBuffer)vkGetInstanceProcAddr(instance, "vkCreateBuffer");
    vkDestroyBuffer = (PFN_vkDestroyBuffer)vkGetInstanceProcAddr(instance, "vkDestroyBuffer");
    vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements");
    vkAllocateMemory = (PFN_vkAllocateMemory)vkGetInstanceProcAddr(instance, "vkAllocateMemory");
    vkFreeMemory = (PFN_vkFreeMemory)vkGetInstanceProcAddr(instance, "vkFreeMemory");
    vkBindBufferMemory = (PFN_vkBindBufferMemory)vkGetInstanceProcAddr(instance, "vkBindBufferMemory");
    vkMapMemory = (PFN_vkMapMemory)vkGetInstanceProcAddr(instance, "vkMapMemory");
    vkUnmapMemory = (PFN_vkUnmapMemory)vkGetInstanceProcAddr(instance, "vkUnmapMemory");
    vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetInstanceProcAddr(instance, "vkCmdBindVertexBuffers");
    vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkGetInstanceProcAddr(instance, "vkCmdBindIndexBuffer");
    vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkGetInstanceProcAddr(instance, "vkCmdDrawIndexed");
    vkCmdDraw = (PFN_vkCmdDraw)vkGetInstanceProcAddr(instance, "vkCmdDraw");
    vkCreateSemaphore = (PFN_vkCreateSemaphore)vkGetInstanceProcAddr(instance, "vkCreateSemaphore");
    vkDestroySemaphore = (PFN_vkDestroySemaphore)vkGetInstanceProcAddr(instance, "vkDestroySemaphore");
    vkCreateFence = (PFN_vkCreateFence)vkGetInstanceProcAddr(instance, "vkCreateFence");
    vkDestroyFence = (PFN_vkDestroyFence)vkGetInstanceProcAddr(instance, "vkDestroyFence");
    vkWaitForFences = (PFN_vkWaitForFences)vkGetInstanceProcAddr(instance, "vkWaitForFences");
    vkResetFences = (PFN_vkResetFences)vkGetInstanceProcAddr(instance, "vkResetFences");
    vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(instance, "vkAcquireNextImageKHR");
    vkQueueSubmit = (PFN_vkQueueSubmit)vkGetInstanceProcAddr(instance, "vkQueueSubmit");
    vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(instance, "vkQueuePresentKHR");
    vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)vkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines");
    vkDestroyPipeline = (PFN_vkDestroyPipeline)vkGetInstanceProcAddr(instance, "vkDestroyPipeline");
    vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkGetInstanceProcAddr(instance, "vkCreatePipelineLayout");
    vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkGetInstanceProcAddr(instance, "vkDestroyPipelineLayout");
    vkCreateShaderModule = (PFN_vkCreateShaderModule)vkGetInstanceProcAddr(instance, "vkCreateShaderModule");
    vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkGetInstanceProcAddr(instance, "vkDestroyShaderModule");
    vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetInstanceProcAddr(instance, "vkCmdBindPipeline");
    vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetInstanceProcAddr(instance, "vkCmdBindDescriptorSets");
    vkCmdPushConstants = (PFN_vkCmdPushConstants)vkGetInstanceProcAddr(instance, "vkCmdPushConstants");
    vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vkGetInstanceProcAddr(instance, "vkCreateDescriptorSetLayout");
    vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkGetInstanceProcAddr(instance, "vkDestroyDescriptorSetLayout");
    vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vkGetInstanceProcAddr(instance, "vkCreateDescriptorPool");
    vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkGetInstanceProcAddr(instance, "vkDestroyDescriptorPool");
    vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkGetInstanceProcAddr(instance, "vkAllocateDescriptorSets");
    vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vkGetInstanceProcAddr(instance, "vkUpdateDescriptorSets");
    vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkGetInstanceProcAddr(instance, "vkFlushMappedMemoryRanges");
    vkCreateImage = (PFN_vkCreateImage)vkGetInstanceProcAddr(instance, "vkCreateImage");
    vkDestroyImage = (PFN_vkDestroyImage)vkGetInstanceProcAddr(instance, "vkDestroyImage");
    vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements");
    vkBindImageMemory = (PFN_vkBindImageMemory)vkGetInstanceProcAddr(instance, "vkBindImageMemory");
    vkCreateSampler = (PFN_vkCreateSampler)vkGetInstanceProcAddr(instance, "vkCreateSampler");
    vkDestroySampler = (PFN_vkDestroySampler)vkGetInstanceProcAddr(instance, "vkDestroySampler");
    vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier");
    vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkGetInstanceProcAddr(instance, "vkCmdCopyBufferToImage");
    vkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkGetInstanceProcAddr(instance, "vkQueueWaitIdle");
    vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
}
