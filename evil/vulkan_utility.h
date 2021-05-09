#pragma once

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>

#define VV(X) if((X) != VK_SUCCESS)
#define VK_API_VERSION VK_MAKE_VERSION(1, 0, 3)
#define VSTRUCTC(NAME) .sType = VK_STRUCTURE_TYPE_##NAME##_CREATE_INFO, .pNext = NULL, .flags = 0,
#define VSTRUCTKHR(NAME) .sType = VK_STRUCTURE_TYPE_##NAME##_CREATE_INFO_KHR, .pNext = NULL, .flags = 0,
#define VSTRUCTF(NAME) .sType = VK_STRUCTURE_TYPE_##NAME##_CREATE_INFO, .pNext = NULL,
#define VSTRUCT(NAME) .sType = VK_STRUCTURE_TYPE_##NAME, .pNext = NULL,

