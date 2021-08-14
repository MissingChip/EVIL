#pragma once

#include <xcb/xcb.h>

#include "vma/include/vk_mem_alloc.h"

#include "evil/utility.h"
#include "evil/vulkan_utility.h"

#define FRAME_RESOURCE_COUNT 3

enum EResult {
    E_SUCCESS = 0,
    E_FAILURE = 1,
    E_NO_SUITABLE_DEVICE,
    E_NO_PRESENT_QUEUE
};
unenum(EResult);

struct EGraphicsPresent {
    VkQueue graphics;
    VkQueue present;
};
unstruct(EGraphicsPresent);

typedef VmaAllocation EAllocation;

struct EBuffer {
    VkBuffer handle;
    EAllocation allocation;
};
unstruct(EBuffer);

struct EFrameResources {
    VkCommandBuffer command_buffer;
    VkFramebuffer framebuffer;
    VkFence fence;
    VkSemaphore available_semaphore;
    VkSemaphore finished_semaphore;
};
unstruct(EFrameResources);

struct EInstance {
    VkInstance handle;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VmaAllocator allocator;
};
unstruct(EInstance);

struct EState {
    EInstance instance;
    VkExtent2D extent;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surface_format;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
    EGraphicsPresent queues;
    EBuffer staging_buffer;
    EBuffer vertex_buffer;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    VkCommandPool command_pool;
    uint32_t nframe_resources;
    EFrameResources frame_resources[FRAME_RESOURCE_COUNT];
    VkImageView image_views[16];
    VkImage test_image;
};
unstruct(EState);

struct EWindowState {
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_intern_atom_reply_t* delete_window_atom;
    VkExtent2D extent;
};
unstruct(EWindowState);

struct ECBuffer {
    EInstance e;
    VkBufferCreateFlags flags;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    uint32_t family_index_count;
    const uint32_t* family_indices;
    VmaAllocationCreateInfo allocation_info;
};
unstruct(ECBuffer);

struct ECSwapchain {
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkExtent2D extent; // preferred
    VkPresentModeKHR present_mode; // preferred
    uint32_t image_count; // preferred
    VkSurfaceFormatKHR format; // preferred
    VkImageUsageFlags usage; // optional (0 for default)
    VkSharingMode sharing_mode;
    VkSwapchainKHR old_swapchain;
};
unstruct(ECSwapchain);

struct ECRenderPass {
    VkDevice device;
    VkFormat format;
};
unstruct(ECRenderPass);

struct EAppInfo {
    const char* name;
    uint32_t version;
    const char* engine_name;
    uint32_t engine_version;
    uint32_t api_version;
};
unstruct(EAppInfo);

struct EDeviceRequirements {
    float prefer_discrete;
    int require_swapchain;
};
unstruct(EDeviceRequirements);

struct EExtras {
    uint32_t extension_count;
    const char** extensions;
    uint32_t layer_count;
    const char** layers;
};
unstruct(EExtras);
