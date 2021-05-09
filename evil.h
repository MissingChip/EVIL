#pragma once
#ifndef _EVIL_H_
#define _EVIL_H_

#include "utility.h"
#include "evil/vulkan_utility.h"
#include "evil/datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

VkResult ezInitialize(EState* es, EWindowState* w);
VkResult ezCreateInstance(VkInstance* es);
VkResult eCreateWindow(VkExtent2D* extent, EWindowState* w);
VkResult ezCreateQuadVertexBuffer(EState* es, VkBuffer* w);
VkResult ezCreatePipeline(EState* es, VkPipeline* pipeline);
EResult eChoosePhysicalDevice(const EDeviceRequirements* req, EState* e);
VkResult eCreatePresentSurface(EState* es, EWindowState* w);
EResult eChooseQueueFamily(EState* es);
EResult eCreateLogicalDevice(EState* es, EExtras* extras);
void eGetQueue(VkDevice device, uint32_t queue_family, uint32_t queue_index, VkQueue* queue);
VkResult eCreateSemaphore(VkDevice device, VkSemaphore* semaphore);
VkResult eCreateFence(VkDevice device, VkSemaphore* semaphore, int signaled);
VkResult eCreateSwapchain(ECSwapchain* swapchain_ci, VkSwapchainKHR* swapchain);
VkResult eCreateRenderPass(ECRenderPass* render_pass_ci, VkRenderPass* render_pass);
VkShaderModule eCreateShaderModuleFromFile(VkDevice device, const char* filename);
VkResult eAllocateBufferMemory(VkPhysicalDevice physical_device, VkDevice device, VkBuffer buffer, VkDeviceMemory* memory);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _EVIL_H_
