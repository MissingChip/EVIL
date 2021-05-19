#pragma once
#ifndef _EVIL_H_
#define _EVIL_H_

#include <xcb/xcb.h>

#include "evil/utility.h"
#include "evil/vulkan_utility.h"
#include "evil/datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

VkResult ezInitialize(EState* es, EWindowState* w);
VkResult ezCreateInstance(VkInstance* es);
VkResult eCreateWindow(VkExtent2D* extent, EWindowState* w);
VkResult ezCreateQuadVertexBuffer(EState* es, EBuffer* w);
VkResult ezCreatePipeline(EState* es, VkPipeline* pipeline);
VkResult ezCreateSwapchainImageViews(EState* es);
VkResult ezDraw(EState* es);
VkResult ezDestroyState(EState* es);
VkResult ezPrepareFrame(EState* es, VkCommandBuffer command_buffer, uint32_t image_index, VkFramebuffer* framebuffer);
EResult eChoosePhysicalDevice(const EDeviceRequirements* req, EState* e);
VkResult eCreatePresentSurface(EState* es, EWindowState* w);
EResult eChooseQueueFamily(EState* es);
EResult eCreateLogicalDevice(EState* es, EExtras* extras);
VkQueue eGetQueue(VkDevice device, uint32_t queue_family, uint32_t queue_index);
VkResult eCreateSemaphore(VkDevice device, VkSemaphore* semaphore);
VkResult eCreateFence(VkDevice device, VkFence* semaphore, int signaled);
VkResult eCreateBuffer(ECBuffer* buffer_info, EBuffer* buffer);
VkResult eMapBufferMemory( EInstance* instance, EBuffer buffer, void** mem);
void eUnmapBufferMemory( EInstance* instance, EBuffer buffer);
VkResult eFlushMappedBuffer( EInstance* instance, EBuffer buffer);
void eDestroyBuffer( EInstance* instance, EBuffer buffer);
VkResult eCreateSwapchain(ECSwapchain* swapchain_ci, VkSwapchainKHR* swapchain);
VkResult eCreateRenderPass(ECRenderPass* render_pass_ci, VkRenderPass* render_pass);
VkShaderModule eCreateShaderModuleFromFile(VkDevice device, const char* filename);
VkResult eCreateCommandPool(VkDevice device, uint32_t queue_family_index, VkCommandPool* command_pool);
VkResult eAllocatePrimaryCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, VkCommandBuffer* buffers);
VkResult eCreateImageViews(VkDevice device, VkSwapchainKHR swapchain, VkFormat format, uint32_t count, VkImageView* views);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _EVIL_H_
