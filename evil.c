
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "evil.h"

#define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#define WINDOW_MANAGER_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

typedef uint32_t u32;

const char* ez_required_instance_extensions[] = {
    WINDOW_MANAGER_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME
};
const uint32_t ez_required_instance_extension_count = arraysize(ez_required_instance_extensions);

const char* ez_required_instance_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const uint32_t ez_required_instance_layer_count = arraysize(ez_required_instance_layers);

const char* ez_required_device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const uint32_t ez_required_device_extension_count = arraysize(ez_required_device_extensions);

struct VertexData {
    float   x, y, z, w;
    float   r, g, b, a;
};
unstruct(VertexData);

VkResult ezInitialize(EState* es, EWindowState* w){
    VkResult r;
    memset(es, 0, sizeof(*es));
    es->extent = w->extent;
    es->nframe_resources = FRAME_RESOURCE_COUNT;
    ezCreateInstance(&es->instance);
    eCreatePresentSurface(es, w);
    EDeviceRequirements device_requirements = {
        .prefer_discrete = 1.0,
        .require_swapchain = 1
    };
    eChoosePhysicalDevice(&device_requirements, es);
    eChooseQueueFamily(es);
    EExtras device_extensions = {
        .extension_count = ez_required_device_extension_count,
        .extensions = ez_required_device_extensions
    };
    eCreateLogicalDevice(es, &device_extensions);
    VmaAllocatorCreateInfo allocator_info = {
        .vulkanApiVersion = VK_API_VERSION,
        .physicalDevice = es->physical_device,
        .device = es->device,
        .instance = es->instance
    };
    vmaCreateAllocator(&allocator_info, &es->allocator);
    ECSwapchain swapchain_ci = {
        .physical_device = es->physical_device,
        .device = es->device,
        .surface = es->surface,
        .extent = es->extent,
        .present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
        .image_count = 3,
        .format = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR },
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
        .old_swapchain = NULL
    };
    eCreateSwapchain(&swapchain_ci, &es->swapchain);
    es->surface_format = swapchain_ci.format;
    ezCreateSwapchainImageViews(es);
    ECRenderPass render_pass_ci = {
        .device = es->device,
        .format = swapchain_ci.format.format
    };
    eCreateRenderPass(&render_pass_ci, &es->render_pass);
    ezCreatePipeline(es, &es->pipeline);
    eCreateCommandPool( es->device, es->graphics_queue_family, &es->command_pool );
    for(int i = 0; i < es->nframe_resources; i++){
        eCreateSemaphore(es->device, &es->frame_resources[i].available_semaphore);
        eCreateSemaphore(es->device, &es->frame_resources[i].finished_semaphore);
        eCreateFence(es->device, &es->frame_resources[i].fence, 1);
        eAllocatePrimaryCommandBuffers(es->device, es->command_pool, 1, &es->frame_resources[i].command_buffer);
    }
    ezCreateQuadVertexBuffer(es, &es->vertex_buffer);
}

EResult eCheckForInstanceExtensions(uint32_t extension_count, const char* instance_extensions[]){
    uint32_t available_count = 0;
    vkEnumerateInstanceExtensionProperties( NULL, &available_count, NULL);
    VkExtensionProperties extension_properties[available_count];
    vkEnumerateInstanceExtensionProperties(NULL, &available_count, extension_properties);
    for(uint32_t i = 0; i < extension_count; i++){
        for(uint32_t j = 0; j < available_count; j++){
            if(strcmp(instance_extensions[i], extension_properties[j].extensionName) == 0){
                return E_FAILURE;
            }
        }
    }
    return E_SUCCESS;
}

EResult eCheckForInstanceLayers(uint32_t layer_count, const char* instance_layers[]){
    uint32_t available_count = 0;
    vkEnumerateInstanceLayerProperties( &available_count, NULL);
    VkExtensionProperties layer_properties[available_count];
    vkEnumerateInstanceExtensionProperties( NULL, &available_count, layer_properties );
    for(uint32_t i = 0; i < layer_count; i++){
        for(uint32_t j = 0; j < available_count; j++){
            if(strcmp(instance_layers[i], layer_properties[j].extensionName) == 0){
                return E_FAILURE;
            }
        }
    }
    return E_SUCCESS;
}

VkResult ezCreateInstance(VkInstance* v){
    eCheckForInstanceExtensions(ez_required_instance_extension_count, ez_required_instance_extensions);
    eCheckForInstanceLayers(ez_required_instance_layer_count, ez_required_instance_layers);
    VkApplicationInfo app_info = {
        VSTRUCT(APPLICATION_INFO)
        .pApplicationName = "Vulkan Application Name",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName = "Vulkan Engine Name",
        .engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion = VK_API_VERSION
    };
    VkInstanceCreateInfo instance_ci = {
        VSTRUCTC(INSTANCE)
        .pApplicationInfo = &app_info,
        .enabledLayerCount = ez_required_instance_layer_count,
        .ppEnabledLayerNames = ez_required_instance_layers,
        .enabledExtensionCount = ez_required_instance_extension_count,
        .ppEnabledExtensionNames = ez_required_instance_extensions
    };
    return vkCreateInstance(&instance_ci, NULL, v);
}

VkShaderModule eCreateShaderModuleFromFile(VkDevice device, const char* filename){
    FILE* file = fopen(filename, "r");
    if(!file) return VK_NULL_HANDLE;
    struct stat stats;
    fstat(fileno(file), &stats);
    size_t codesize = stats.st_size;
    char contents[codesize];
    fread(contents, 1, codesize, file);
    VkShaderModuleCreateInfo shader_module_create_info = {
        VSTRUCTC(SHADER_MODULE)
        .codeSize = codesize,
        .pCode = (uint32_t*)contents
    };
    VkResult r;
    VkShaderModule shader_module;
    r = vkCreateShaderModule(device, &shader_module_create_info, NULL, &shader_module);
    VV(r) return VK_NULL_HANDLE;
    return shader_module;
}

VkResult ezCreateSwapchainImageViews(EState* es){
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(es->device, es->swapchain, &image_count, NULL);
    return eCreateImageViews(es->device, es->swapchain, es->surface_format.format, image_count, es->image_views);
}

VkResult ezCreatePipeline(EState* es, VkPipeline* pipeline){
    VkResult r;
    VkShaderModule vert = eCreateShaderModuleFromFile(es->device, "vert.spv");
    VkShaderModule frag = eCreateShaderModuleFromFile(es->device, "frag.spv");

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {
        {
            VSTRUCTC(PIPELINE_SHADER_STAGE)
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert,
            .pName = "main",
            .pSpecializationInfo = NULL
        },
        {
            VSTRUCTC(PIPELINE_SHADER_STAGE)
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag,
            .pName = "main",
            .pSpecializationInfo = NULL
        }
    };
    uint32_t shader_stage_count = arraysize(shader_stage_create_infos);

    VkVertexInputBindingDescription vertex_binding_descriptions[] = {
        {
            .binding = 0,
            .stride = sizeof(VertexData),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };
    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            .location = 0,
            .binding = vertex_binding_descriptions[0].binding,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(struct VertexData, x)
        },
        {
            .location = 1,
            .binding = vertex_binding_descriptions[0].binding,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(struct VertexData, r)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        VSTRUCTC(PIPELINE_VERTEX_INPUT_STATE)
        .vertexBindingDescriptionCount = arraysize(vertex_binding_descriptions),
        .pVertexBindingDescriptions = vertex_binding_descriptions,
        .vertexAttributeDescriptionCount = arraysize(vertex_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        VSTRUCTC(PIPELINE_INPUT_ASSEMBLY_STATE)
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        VSTRUCTC(PIPELINE_VIEWPORT_STATE)
        .viewportCount = 1,
        .pViewports = NULL,
        .scissorCount = 1,
        .pScissors = NULL
    };
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        VSTRUCTC(PIPELINE_RASTERIZATION_STATE)
        .depthClampEnable = VK_FALSE, // controls whether to clamp the fragment’s depth values as described in Depth Test. Enabling depth clamp will also disable clipping primitives to the z planes of the frustrum as described in Primitive Clipping.
        .rasterizerDiscardEnable = VK_FALSE, // controls whether primitives are discarded immediately before the rasterization stage
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0,
        .depthBiasClamp = 0.0,
        .depthBiasSlopeFactor = 0.0,
        .lineWidth = 1.0
    };
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
        VSTRUCTC(PIPELINE_MULTISAMPLE_STATE)
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // multisampling selection
        .sampleShadingEnable = VK_FALSE, // *can* be used to enable sample shading (forcing multisampling, even when not on geometry edge)
        .minSampleShading = 1.0, // only used if sampleShadingEnable == VK_TRUE
        .pSampleMask = NULL, // only used if sampleShadingEnable == VK_TRUE
        .alphaToCoverageEnable = VK_FALSE, // only used if sampleShadingEnable == VK_TRUE
        .alphaToOneEnable = VK_FALSE // only used if sampleShadingEnable == VK_TRUE
    };
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
        VSTRUCTC(PIPELINE_COLOR_BLEND_STATE)
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0, 0, 0, 0}
    };
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        VSTRUCTC(PIPELINE_DYNAMIC_STATE)
        arraysize(dynamic_states),
        dynamic_states
    };

    VkPipelineLayoutCreateInfo layout_create_info = {
        VSTRUCTC(PIPELINE_LAYOUT)
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };
    VkPipelineLayout pipeline_layout;
    r = vkCreatePipelineLayout( es->device, &layout_create_info, NULL, &pipeline_layout);
    VV(r) return r;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VSTRUCTC(GRAPHICS_PIPELINE)
        .stageCount = shader_stage_count,
        .pStages = shader_stage_create_infos, // pointer to an array of stageCount VkPipelineShaderStageCreateInfo structures describing the set of the shader stages to be included in the graphics pipeline
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info, // pointer to a VkPipelineInputAssemblyStateCreateInfo structure which determines input assembly behavior, as described in **Drawing Commands**
        .pTessellationState = NULL, // pointer to a VkPipelineTessellationStateCreateInfo structure, and is ignored if the pipeline does not include a tessellation control shader stage and tessellation evaluation shader stage
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = NULL, // ignored if the pipeline has rasterization disabled or if the subpass of the render pass the pipeline is created against does not use a depth/stencil attachment
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamic_state_create_info, // used to indicate which properties of the pipeline state object are dynamic and can be changed independently of the pipeline state. This can be NULL, which means no state in the pipeline is considered dynamic
        .layout = pipeline_layout,
        .renderPass = es->render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    r = vkCreateGraphicsPipelines( es->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &es->pipeline );
    VV(r) return r;

    vkDestroyPipelineLayout(es->device, pipeline_layout, NULL);
    vkDestroyShaderModule(es->device, vert, NULL);
    vkDestroyShaderModule(es->device, frag, NULL);
}

VkResult ezDraw(EState* es){
    static size_t resource_index = 0;

    VkResult r;
    VkDevice device = es->device;
    VkSwapchainKHR swapchain = es->swapchain;
    EFrameResources* current = &es->frame_resources[resource_index];
    uint32_t image_index;

    r = vkWaitForFences( device, 1, &current->fence, VK_FALSE, 1000000000);
    VV(r) return r;
    vkResetFences( device, 1, &current->fence );
    r = vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, current->available_semaphore, VK_NULL_HANDLE, &image_index );
    switch( r ) {
      case VK_SUCCESS:
      case VK_SUBOPTIMAL_KHR:
        break;
      case VK_ERROR_OUT_OF_DATE_KHR:
        // return OnWindowSizeChanged();
      default:
        printf("Problem occurred during swap chain image acquisition\n");
        return r;
    }

    r = ezPrepareFrame( es, current->command_buffer, image_index, &current->framebuffer);

    resource_index = (resource_index + 1) % es->nframe_resources;

    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        1,
        &current->available_semaphore,
        &wait_dst_stage_mask,
        1,
        &current->command_buffer,
        1,
        &current->finished_semaphore
    };
    r = vkQueueSubmit( es->queues.graphics, 1, &submit_info, current->fence);
    VV(r) return r;

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current->finished_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &es->swapchain,
        .pImageIndices = &image_index,
        .pResults = NULL
    };
    r = vkQueuePresentKHR( es->queues.present, &present_info );

    switch(r) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            // return eWindowSizeChan(v);
        default:
            printf("Unknown VKResult acquiring next image: %d", r);
            return r;
    }
    return r;
}

VkResult ezDestroyState(EState* es){
    vkDeviceWaitIdle(es->device);
    for(int i = 0; i < es->nframe_resources; i++){
        vkFreeCommandBuffers(es->device, es->command_pool, 1, &es->frame_resources[i].command_buffer);
        vkDestroySemaphore(es->device, es->frame_resources[i].available_semaphore, NULL);
        vkDestroySemaphore(es->device, es->frame_resources[i].finished_semaphore, NULL);
        vkDestroyFence(es->device, es->frame_resources[i].fence, NULL);
        vkDestroyFramebuffer(es->device, es->frame_resources[i].framebuffer, NULL);
    }
    vkDestroyCommandPool(es->device, es->command_pool, NULL);
    vkDestroyPipeline(es->device, es->pipeline, NULL);
    vkDestroyRenderPass(es->device, es->render_pass, NULL);
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(es->device, es->swapchain, &image_count, NULL);
    for(int i = 0; i < image_count; i++){
        vkDestroyImageView(es->device, es->image_views[i], NULL);
    }
    vkDestroySwapchainKHR(es->device, es->swapchain, NULL);
    vkDestroySurfaceKHR(es->instance, es->surface, NULL);
    EInstance ei = {es->instance, es->physical_device, es->device, es->allocator};
    eDestroyBuffer(&ei, es->vertex_buffer);
    eDestroyBuffer(&ei, es->staging_buffer);
    vmaDestroyAllocator(es->allocator);
    vkDestroyDevice(es->device, NULL);
    vkDestroyInstance(es->instance, NULL);
}

VkResult ezPrepareFrame(EState* es, VkCommandBuffer command_buffer, uint32_t image_index, VkFramebuffer* framebuffer){
    // VDBG("Preparing frame");
    VkResult r;
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR( es->device, es->swapchain, &image_count, NULL);
    VkImage images[image_count];
    vkGetSwapchainImagesKHR( es->device, es->swapchain, &image_count, images);

    if(*framebuffer != VK_NULL_HANDLE){
        vkDestroyFramebuffer( es->device, *framebuffer, NULL );
        *framebuffer = VK_NULL_HANDLE;
    }

    VkFramebufferCreateInfo framebuffer_create_info = {
        VSTRUCTC(FRAMEBUFFER)
        .renderPass = es->render_pass,
        .attachmentCount = 1,
        .pAttachments = &es->image_views[image_index],
        .width = es->extent.width,
        .height = es->extent.height,
        .layers = 1
    };

    r = vkCreateFramebuffer( es->device, &framebuffer_create_info, NULL, framebuffer );
    VV(r) return r;

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );
    VkImageSubresourceRange image_subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    if( es->queues.graphics != es->queues.present ){
        VkImageMemoryBarrier barrier_present_draw = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = es->present_queue_family,
            .dstQueueFamilyIndex = es->graphics_queue_family,
            .image = images[image_index],
            .subresourceRange = image_subresource_range
        };
        vkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_present_draw );
    }
    VkClearValue clear_value = {{1.0f, 0.2f, 0.5f, 0.0f}};
    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = es->render_pass,
        .framebuffer = *framebuffer,
        .renderArea = {
            {0, 0},
            es->extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_value
    };
    vkCmdBeginRenderPass( command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, es->pipeline );
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = es->extent.width,
        .height = es->extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        {0, 0}, es->extent
    };
    vkCmdSetViewport( command_buffer, 0, 1, &viewport );
    vkCmdSetScissor( command_buffer, 0, 1, &scissor );
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers( command_buffer, 0, 1, &es->vertex_buffer.handle, &offset );
    vkCmdDraw( command_buffer, 4, 1, 0, 0 );
    vkCmdEndRenderPass( command_buffer );
    if( es->queues.graphics != es->queues.present ){
        VkImageMemoryBarrier barrier_present_draw = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = es->graphics_queue_family,
            .dstQueueFamilyIndex = es->present_queue_family,
            .image = images[image_index],
            .subresourceRange = image_subresource_range
        };
        vkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_present_draw );
    }
    return vkEndCommandBuffer( command_buffer );
}

VkResult eCreateWindow(VkExtent2D* extent, EWindowState* w){
    w->connection = xcb_connect(NULL, NULL);
    const xcb_setup_t      *setup  = xcb_get_setup (w->connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;
    w->window = xcb_generate_id(w->connection); 
    uint32_t value_list[] = {
        screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };
    xcb_create_window(
        w->connection,
        XCB_COPY_FROM_PARENT,
        w->window,
        screen->root,
        0, 0,
        extent->width, extent->height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, 
        value_list
    );
    xcb_flush (w->connection);
    char title[] = "Window title";
    xcb_change_property(
        w->connection,
        XCB_PROP_MODE_REPLACE,
        w->window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen( title ),
        title );
    xcb_map_window(w->connection, w->window);
    w->extent = *extent;
}

VkResult eCreatePresentSurface(EState* vs, EWindowState* w){
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        w->connection,
        w->window
    };
    return vkCreateXcbSurfaceKHR(vs->instance, &surface_create_info, NULL, &vs->surface);
}

EResult eChoosePhysicalDevice(const EDeviceRequirements* req, EState* v){
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(v->instance, &device_count, NULL);
    VkPhysicalDevice physical_devices[device_count];
    vkEnumeratePhysicalDevices(v->instance, &device_count, physical_devices);
    float max_score = -INFINITY;
    uint32_t max_score_idx = 0;
    for(int i = 0; i < device_count; i++){
        float score;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        score = req->prefer_discrete * (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        
        if(req->require_swapchain){
            int found = 0;
            uint32_t extension_count = 0;
            vkEnumerateDeviceExtensionProperties( physical_devices[i], NULL, &extension_count, NULL);
            VkExtensionProperties extensions[extension_count];
            vkEnumerateDeviceExtensionProperties( physical_devices[i], NULL, &extension_count, extensions);
            for(int ext = 0; ext < extension_count; ext++){
                if(strcmp(extensions[ext].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0){
                    found = 1;
                    break;
                }
            }
            if(!found) score = -INFINITY;
        }

        if( VK_VERSION_MAJOR( properties.apiVersion ) < VK_VERSION_MAJOR( VK_API_VERSION ) ||
                properties.limits.maxImageDimension2D < 4096){
            score = -INFINITY;
        }

        if(score > max_score){
            max_score = score;
            max_score_idx = i;
        }
    }
    if(max_score == -INFINITY){
        return E_NO_SUITABLE_DEVICE;
    }
    v->physical_device = physical_devices[max_score_idx];
    return E_SUCCESS;
}

EResult eChooseQueueFamily(EState* vs){
    u32 gq = BAD, pq = BAD;
    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( vs->physical_device, &count, NULL );
    VkQueueFamilyProperties properties[count];
    vkGetPhysicalDeviceQueueFamilyProperties( vs->physical_device, &count, properties );
    VkBool32 present_support[count];
    for(u32 i = 0; i < count; i++){
        vkGetPhysicalDeviceSurfaceSupportKHR( vs->physical_device, i, vs->surface, &present_support[i] );
        if( properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ){
            if( gq == BAD ){
                gq = i;
            }
            if( present_support[i] ){
                vs->graphics_queue_family = vs->present_queue_family = i;
                return E_SUCCESS;
            }
        }
    }
    for( u32 i = 0; i < count && pq == BAD; i++){
        if(present_support[i])
            pq = i;
    }
    if(gq == BAD || pq == BAD)
        return E_NO_PRESENT_QUEUE;
    vs->graphics_queue_family = gq;
    vs->present_queue_family = pq;
    return E_SUCCESS;
}

EResult eCreateLogicalDevice(EState* vs, EExtras* extras){
    float queue_priorities[] = {1.0};
    u32 number_of_queues = 1 + (vs->graphics_queue_family != vs->present_queue_family);
    VkDeviceQueueCreateInfo device_queue_cis[] = {
        {
            VSTRUCTC(DEVICE_QUEUE)
            .queueFamilyIndex = vs->graphics_queue_family,
            .queueCount = arraysize(queue_priorities),
            .pQueuePriorities = queue_priorities
        },
        {
            VSTRUCTC(DEVICE_QUEUE)
            .queueFamilyIndex = vs->present_queue_family,
            .queueCount = arraysize(queue_priorities),
            .pQueuePriorities = queue_priorities
        }
    };
    VkDeviceCreateInfo device_ci = {
        VSTRUCTC(DEVICE)
        .queueCreateInfoCount = number_of_queues,
        .pQueueCreateInfos = device_queue_cis,
        .enabledLayerCount = 0, .ppEnabledLayerNames = NULL, .enabledExtensionCount = 0, .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL
    };
    if(extras){
        device_ci.enabledExtensionCount = extras->extension_count;
        device_ci.ppEnabledExtensionNames = extras->extensions;
        device_ci.enabledLayerCount = extras->layer_count;
        device_ci.ppEnabledLayerNames = extras->layers;
    }
    vkCreateDevice( vs->physical_device, &device_ci, NULL, &vs->device);
    vkGetDeviceQueue( vs->device, vs->graphics_queue_family, 0, &vs->queues.graphics);
    if(number_of_queues > 1){
        vkGetDeviceQueue( vs->device, vs->present_queue_family, 0, &vs->queues.present );
    }
    else{
        vs->queues.present = vs->queues.graphics;
    }
    return E_SUCCESS;
}

VkQueue eGetQueue(VkDevice device, uint32_t queue_family, uint32_t queue_index){
    VkQueue queue;
    vkGetDeviceQueue( device, queue_family, queue_index, &queue);
    return queue;
}

VkResult eCreateSemaphore(VkDevice device, VkSemaphore* semaphore){
    VkSemaphoreCreateInfo semaphore_create_info = {
        VSTRUCTC(SEMAPHORE)
    };
    return vkCreateSemaphore( device, &semaphore_create_info, NULL, semaphore);
}

VkResult eCreateFence(VkDevice device, VkFence* semaphore, int signaled){
    VkFenceCreateInfo fence_create_info = {
        VSTRUCTC(FENCE)
    };
    if(signaled){
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    return vkCreateFence( device, &fence_create_info, NULL, semaphore);
}

VkResult eCreateBuffer(ECBuffer* ci, EBuffer* buffer){
    VkBufferCreateInfo buffer_info = {
        VSTRUCTF(BUFFER)
        .flags = ci->flags,
        .size = ci->size,
        .usage = ci->usage,
        .sharingMode = ci->sharing_mode,
        .queueFamilyIndexCount = ci->family_index_count,
        .pQueueFamilyIndices = ci->family_indices
    };
    VmaAllocationCreateInfo alloc_info = ci->allocation_info;
    vmaCreateBuffer(ci->e.allocator, &buffer_info, &alloc_info, &buffer->handle, &buffer->allocation, NULL);
}

VkResult eMapBufferMemory( EInstance* instance, EBuffer buffer, void** mem){
    return vmaMapMemory(instance->allocator, buffer.allocation, mem);
}
void eUnmapBufferMemory( EInstance* instance, EBuffer buffer){
    vmaUnmapMemory(instance->allocator, buffer.allocation);
}
VkResult eFlushMappedBuffer( EInstance* instance, EBuffer buffer){
    return vmaFlushAllocation(instance->allocator, buffer.allocation, 0, VK_WHOLE_SIZE);
}
void eDestroyBuffer( EInstance* instance, EBuffer buffer){
    vmaDestroyBuffer(instance->allocator, buffer.handle, buffer.allocation);
}

VkResult eCreateSharedBuffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, uint32_t queue_family_count, uint32_t* queue_families, VkBuffer* buffer, VkDeviceMemory* memory){
    VkBufferCreateInfo buffer_create_info = {
        VSTRUCTC(BUFFER)
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = queue_family_count,
        .pQueueFamilyIndices = queue_families
    };
    return vkCreateBuffer( device, &buffer_create_info, NULL, buffer);
}

VkResult eCreateSwapchain(ECSwapchain* ci, VkSwapchainKHR* swapchain){
    VkResult r;
    VkSurfaceCapabilitiesKHR capabilities;
    r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ci->physical_device, ci->surface, &capabilities);
    VV(r) return r;
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( ci->physical_device, ci->surface, &format_count, NULL);
    VkSurfaceFormatKHR formats[format_count];
    r = vkGetPhysicalDeviceSurfaceFormatsKHR( ci->physical_device, ci->surface, &format_count, formats);
    VV(r) return r;
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR( ci->physical_device, ci->surface, &present_mode_count, NULL);
    VkPresentModeKHR present_modes[present_mode_count];
    r = vkGetPhysicalDeviceSurfacePresentModesKHR( ci->physical_device, ci->surface, &present_mode_count, present_modes);
    VV(r) return r;

    uint32_t image_count = capabilities.minImageCount + 1;
    if( image_count < ci->image_count && ci->image_count >= capabilities.minImageCount ) {
        image_count = ci->image_count;
    }
    if( (capabilities.maxImageCount > 0) &&
            (image_count > capabilities.maxImageCount) ) {
        image_count = capabilities.maxImageCount;
    }

    VkSurfaceFormatKHR surface_format = (VkSurfaceFormatKHR){ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    if( (format_count == 1) &&
            (formats[0].format == VK_FORMAT_UNDEFINED) ) {
        surface_format = ci->format;
    }
    else{
        VkBool32 found = VK_FALSE;
        for(int i = 0; i < format_count; i++){
            if(formats[i].format == VK_FORMAT_R8G8B8_UNORM) {
                surface_format = formats[i];
                found = VK_TRUE;
                break;
            }
        }
        if(found == VK_FALSE){
            surface_format = formats[0];
        }
    }
    
    VkExtent2D extent = { ci->extent.width, ci->extent.height };
    if( capabilities.currentExtent.width == -1 ) {
        if( extent.width < capabilities.minImageExtent.width )
            extent.width = capabilities.minImageExtent.width;
        if( extent.height < capabilities.minImageExtent.height )
            extent.height = capabilities.minImageExtent.height;
        if( extent.width > capabilities.maxImageExtent.width )
            extent.width = capabilities.maxImageExtent.width;
        if( extent.height > capabilities.maxImageExtent.height )
            extent.height = capabilities.maxImageExtent.height;
    }
    
    if(ci->usage == 0) ci->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkImageUsageFlags usage = ci->usage;

    VkSurfaceTransformFlagBitsKHR swapchain_transform = 0;
    if(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        swapchain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(int i = 0; i < present_mode_count; i++){
        if(present_modes[i] == ci->present_mode){
            swapchain_present_mode = ci->present_mode;
            break;
        }
    }

    VkSwapchainKHR old_swapchain = ci->old_swapchain;
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = ci->surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1, // Defines the number of layers in a swap chain images (that is, views); typically this value will be one but if we want to create multiview or stereo (stereoscopic 3D) images, we can set it to some higher value.
        .imageUsage = usage,
        .imageSharingMode = ci->sharing_mode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = swapchain_transform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchain_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };

    r = vkCreateSwapchainKHR( ci->device, &swapchain_create_info, NULL, swapchain );
    VV(r) return r;
    if( old_swapchain != VK_NULL_HANDLE ) {
        vkDestroySwapchainKHR( ci->device, old_swapchain, NULL );
    }

    ci->present_mode = swapchain_present_mode;
    ci->extent = extent;
    ci->format = surface_format; 
    vkGetSwapchainImagesKHR( ci->device, *swapchain, &ci->image_count, NULL );
}

VkResult eCreateRenderPass(ECRenderPass* ci, VkRenderPass* render_pass){
    /*
     * Define attachment info for a render pass,
     *  and layout of those attachments (general, color, depth etc) in the attachment references
     *  to be used in subpass descriptions
     * Describe subpasses for the render pass
     *  and dependencies between them in subpass dependencies
     * Create render pass with this info
    */
    VkResult r;
    VkAttachmentDescription attachment_descriptions[] = {
        {
            .flags = 0,
            .format = ci->format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }
    };
    VkAttachmentReference color_attachment_references[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };
    VkSubpassDescription subpass_descriptions[] = {
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = color_attachment_references,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = NULL,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL
        }
    };
    VkSubpassDependency subpass_dependencies[] = {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        },
        {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        }
    };
    uint32_t dependency_count = arraysize(subpass_dependencies);

    VkRenderPassCreateInfo vkci = {
        VSTRUCTC(RENDER_PASS)
        .attachmentCount = 1,
        .pAttachments = attachment_descriptions,
        .subpassCount = 1,
        .pSubpasses = subpass_descriptions,
        .dependencyCount = dependency_count,
        .pDependencies = subpass_dependencies
    };
    return vkCreateRenderPass( ci->device, &vkci, NULL, render_pass);
}

VkResult ezCreateQuadVertexBuffer( EState* es, EBuffer* buffer){
    VkResult r;
    VertexData vertex_data[] = {
      {
        -0.7f, -0.7f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 0.0f
      },
      {
        -0.7f, 0.7f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f
      },
      {
        0.7f, -0.7f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f
      },
      {
        0.7f, 0.7f, 0.0f, 1.0f,
        0.3f, 0.3f, 0.3f, 0.0f
      }
    };
    size_t vbsize = sizeof(vertex_data);

    ECBuffer buffer_ci = {
        .e = {es->instance, es->physical_device, es->device, es->allocator},
        .flags = 0,
        .size = vbsize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
        .family_index_count = 0,
        .family_indices = NULL,
        .allocation_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        }
    };
    eCreateBuffer(&buffer_ci, &es->vertex_buffer);
    buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_ci.allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    buffer_ci.allocation_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    eCreateBuffer(&buffer_ci, &es->staging_buffer);
    
    void* mem_ptr;
    EInstance ei = {es->instance, es->physical_device, es->device, es->allocator};
    r = eMapBufferMemory( &ei, es->staging_buffer, &mem_ptr );
    VV(r) return r;
    memcpy( mem_ptr, vertex_data, vbsize );
    eFlushMappedBuffer(&ei, es->staging_buffer );
    eUnmapBufferMemory( &ei, es->staging_buffer );

    VkCommandBufferBeginInfo command_buffer_bi = {
        VSTRUCT(COMMAND_BUFFER_BEGIN_INFO)
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };
    VkCommandBuffer command_buffer = es->frame_resources[0].command_buffer;
    vkBeginCommandBuffer(command_buffer, &command_buffer_bi);
    VkBufferCopy copy_info = {
        0,
        0,
        vbsize
    };
    vkCmdCopyBuffer( command_buffer, es->staging_buffer.handle, es->vertex_buffer.handle, 1, &copy_info);
    VkBufferMemoryBarrier memory_barrier = {
        VSTRUCT(BUFFER_MEMORY_BARRIER)
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = es->vertex_buffer.handle,
        0,
        VK_WHOLE_SIZE
    };
    vkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &memory_barrier, 0, NULL);
    vkEndCommandBuffer(command_buffer);
    VkSubmitInfo submit_info = {
        VSTRUCT(SUBMIT_INFO)
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL
    };
    vkQueueSubmit( es->queues.graphics, 1, &submit_info, VK_NULL_HANDLE );
    vkDeviceWaitIdle(es->device);
    return VK_SUCCESS;
}

VkResult eCreateCommandPool(VkDevice device, uint32_t queue_family_index, VkCommandPool* command_pool){
    VkResult r;
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queue_family_index
    };
    return vkCreateCommandPool( device, &command_pool_create_info, NULL, command_pool);
}

VkResult eAllocatePrimaryCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, VkCommandBuffer* buffers) {
    VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };
    return vkAllocateCommandBuffers(device, &allocate_info, buffers);
}

VkResult eCreateImageViews(VkDevice device, VkSwapchainKHR swapchain, VkFormat format, uint32_t count, VkImageView* views){
    uint32_t image_count = count;
    VkImage images[image_count];
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images);

    for(size_t i = 0; i < image_count; i++){
        VkResult r;
        VkImageViewCreateInfo image_view_create_info = {
            VSTRUCTC(IMAGE_VIEW)
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {VSWI, VSWI, VSWI, VSWI},
            .subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1
            }
        };
        r = vkCreateImageView( device, &image_view_create_info, NULL, &views[i]);
        VV(r) return r;
    }
    return VK_SUCCESS;
}