#include "gfx.h"
#include "bal.h"
#include "fbgl.h"
#include "file.h"
#include "array.h"
#include "mathimpl.h"
#include "font.h"

#include "shared/error.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

VkShaderStageFlagBits get_vk_stage(enum BalShaderType type)
{
    switch (type) {
    case BAL_SHADER_VERTEX:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case BAL_SHADER_PIXEL:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    default:
        return 0;
    }
}

static VkPipelineShaderStageCreateInfo *GFX_create_pipeline_shaders(VkDevice device, struct BalSpirv **spirv, u32 shader_len)
{
    VkPipelineShaderStageCreateInfo *vk_shaders = malloc(shader_len * sizeof(*vk_shaders));

    for (u32 i = 0; i < shader_len; ++i) {
        vk_shaders[i] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .stage = get_vk_stage(spirv[i]->stage),
            .module = spirv[i]->module,
            .pName = "main",
            .pSpecializationInfo = 0
        };
    }

    return vk_shaders;
}

static bool GFX_find_shader(struct FbGpu *gpu, struct BalDescriptorTable *table, const char *name, enum BalShaderStage stage, struct BalSpirv **shader)
{
    for (u32 i = 0; i < table->descriptor_count; ++i) {
        struct BalDescriptor *desc = &table->descriptors[i];

        if (desc->type == BAL_TYPE_SPRV) {
            struct BalSpirv *spirv = BAL_PTR(desc->ref);
            BalString shader_name = BAL_PTR(spirv->name);

            if (spirv->stage == stage && strcmp(name, shader_name) == 0) {
                if (!spirv->module) {
                    struct BalBuffer *buf = BAL_PTR(spirv->buffer);

                    VkShaderModuleCreateInfo module_info = {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .pNext = 0,
                        .flags = 0,
                        .codeSize = buf->size,
                        .pCode = (u32 *)buf->data
                    };

                    vkCreateShaderModule(gpu->device, &module_info, 0, (VkShaderModule *)&spirv->module);
                }
                *shader = spirv;
                return true;
            }
        }
    }

    return false;
}

static bool VKGFX_create_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, struct FbGfxVertexLayout layout, struct BalSpirv *vsh, struct BalSpirv *psh, u64 state, VkPipeline *pipeline)
{
    VkPipelineVertexInputStateCreateInfo vertex_info = {0};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = layout.binding_count;
    vertex_info.pVertexBindingDescriptions = layout.bindings;
    vertex_info.vertexAttributeDescriptionCount = layout.attribute_count;
    vertex_info.pVertexAttributeDescriptions = layout.attributes;

    VkPipelineInputAssemblyStateCreateInfo ia_info = {0};
    ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo raster_info = {0};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.frontFace = (state & FB_STATE_CLOCKWISE) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.f;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;

    switch (state & FB_STATE_CULL_BITS) {
        case FB_STATE_CULL_FRONT:
            raster_info.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case FB_STATE_CULL_BACK:
            raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case FB_STATE_CULL_NONE:
            raster_info.cullMode = VK_CULL_MODE_NONE;
            break;
        default:
            break;
    }

    VkPipelineColorBlendAttachmentState attachment = {0};

    VkBlendFactor src = VK_BLEND_FACTOR_ONE;
    switch (state & FB_STATE_SRCBLEND_BITS) {
        case FB_STATE_SRCBLEND_ONE:                 src = VK_BLEND_FACTOR_ONE; break;
        case FB_STATE_SRCBLEND_ZERO:                src = VK_BLEND_FACTOR_ZERO; break;
        case FB_STATE_SRCBLEND_DST_COLOR:           src = VK_BLEND_FACTOR_DST_COLOR; break;
        case FB_STATE_SRCBLEND_ONE_MINUS_DST_COLOR: src = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
        case FB_STATE_SRCBLEND_SRC_ALPHA:           src = VK_BLEND_FACTOR_SRC_ALPHA; break;
        case FB_STATE_SRCBLEND_ONE_MINUS_SRC_ALPHA: src = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
        case FB_STATE_SRCBLEND_DST_ALPHA:           src = VK_BLEND_FACTOR_DST_ALPHA; break;
        case FB_STATE_SRCBLEND_ONE_MINUS_DST_ALPHA: src = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
    }

    VkBlendFactor dst = VK_BLEND_FACTOR_ONE;
    switch (state & FB_STATE_DSTBLEND_BITS) {
        case FB_STATE_DSTBLEND_ONE:                 dst = VK_BLEND_FACTOR_ONE; break;
        case FB_STATE_DSTBLEND_ZERO:                dst = VK_BLEND_FACTOR_ZERO; break;
        case FB_STATE_DSTBLEND_DST_COLOR:           dst = VK_BLEND_FACTOR_DST_COLOR; break;
        case FB_STATE_DSTBLEND_ONE_MINUS_DST_COLOR: dst = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
        case FB_STATE_DSTBLEND_SRC_ALPHA:           dst = VK_BLEND_FACTOR_SRC_ALPHA; break;
        case FB_STATE_DSTBLEND_ONE_MINUS_SRC_ALPHA: dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
        case FB_STATE_DSTBLEND_DST_ALPHA:           dst = VK_BLEND_FACTOR_DST_ALPHA; break;
        case FB_STATE_DSTBLEND_ONE_MINUS_DST_ALPHA: dst = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
    }

    attachment.blendEnable = (VkBool32)(state & FB_STATE_BLEND_ENABLE);
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcColorBlendFactor = src;
    attachment.dstColorBlendFactor = dst;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = src;
    attachment.dstAlphaBlendFactor = dst;
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_info = {0};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &attachment;

    VkPipelineDepthStencilStateCreateInfo depth_info = {0};
    depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_info.depthTestEnable = (VkBool32)(state & FB_STATE_DEPTH_TEST);
    depth_info.depthWriteEnable = (VkBool32)(state & FB_STATE_DEPTH_WRITE);

    VkCompareOp compare_func = VK_COMPARE_OP_ALWAYS;
    switch (state & FB_STATE_DEPTHFUNC_BITS) {
    case FB_STATE_DEPTHFUNC_LESS: compare_func = VK_COMPARE_OP_LESS_OR_EQUAL; break;
    case FB_STATE_DEPTHFUNC_ALWAYS: compare_func = VK_COMPARE_OP_ALWAYS; break;
    case FB_STATE_DEPTHFUNC_GREATER: compare_func = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
    case FB_STATE_DEPTHFUNC_EQUAL: compare_func = VK_COMPARE_OP_EQUAL; break;
    }
    depth_info.depthCompareOp = compare_func;

    VkPipelineMultisampleStateCreateInfo msaa_info = {0};
    msaa_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa_info.rasterizationSamples = 1;

    struct BalSpirv *shaders[2] = { vsh, psh };
    VkPipelineShaderStageCreateInfo *stage_info = GFX_create_pipeline_shaders(device, shaders, 2);

    VkDynamicState dynamic_state[2] = {
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamic_info = {0};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = 2;
    dynamic_info.pDynamicStates = dynamic_state;
    
    VkPipelineViewportStateCreateInfo viewport_info = {0};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkRenderPass render_pass = 0;

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &color_attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    vkCreateRenderPass(device, &renderPassInfo, 0, &render_pass);

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.pVertexInputState = &vertex_info;
    pipeline_info.pInputAssemblyState = &ia_info;
    pipeline_info.pRasterizationState = &raster_info;
    pipeline_info.pColorBlendState = &blend_info;
    pipeline_info.pDepthStencilState = &depth_info;
    pipeline_info.pMultisampleState = &msaa_info;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stage_info;

    VkResult status = vkCreateGraphicsPipelines(device, 0, 1, &pipeline_info, 0, pipeline);

    free(stage_info);

    return status == VK_SUCCESS;
}

VkPipeline GFX_find_pipeline(struct FbGfxRenderProgram *prog, struct FbGpu *gpu, u64 state)
{
    u32 sz = ARRAY_size(prog->cached_pipelines);
    for (u32 i = 0; i < sz; ++i) {
        if (prog->pipeline_bits[i] == state) {
            return prog->cached_pipelines[i];
        }
    }

    VkPipeline pipeline = 0;
    bool result = VKGFX_create_pipeline(gpu->device, prog->pipeline_layout, prog->layout, prog->vsh, prog->psh, state, &pipeline);

    ARRAY_push(prog->cached_pipelines, pipeline);
    ARRAY_push(prog->pipeline_bits, state);

    return pipeline;
}

static enum FbErrorCode GFX_create_pipeline_layout(struct FbGpu *gpu, struct BalSpirv *vsh, struct BalSpirv *psh, VkPipelineLayout *pipeline_layout, VkDescriptorSetLayout *descriptor_layout)
{
    VkDescriptorSetLayoutBinding *bindings = 0;

    struct BalBuffer *vsh_layout_buf = BAL_PTR(vsh->layout);
    for (u32 i = 0; i < vsh_layout_buf->size; ++i) {
        VkDescriptorSetLayoutBinding bind = ((VkDescriptorSetLayoutBinding *)vsh_layout_buf->data)[i];
        bind.stageFlags = get_vk_stage(vsh->stage);

        ARRAY_push(bindings, bind);
    }

    struct BalBuffer *psh_layout_buf = BAL_PTR(psh->layout);
    for (u32 i = 0; i < psh_layout_buf->size; ++i) {
        VkDescriptorSetLayoutBinding bind = ((VkDescriptorSetLayoutBinding *)psh_layout_buf->data)[i];
        bind.stageFlags = get_vk_stage(psh->stage);

        ARRAY_push(bindings, bind);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_info = { 0 };
    descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_info.bindingCount = ARRAY_size(bindings);
    descriptor_info.pBindings = bindings;

    VkDescriptorSetLayout layout = 0;
    vkCreateDescriptorSetLayout(gpu->device, &descriptor_info, 0, &layout);

    VkPipelineLayoutCreateInfo layout_info = { 0 };
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &layout;
    vkCreatePipelineLayout(gpu->device, &layout_info, 0, pipeline_layout);
    *descriptor_layout = layout;

    return FB_ERR_NONE;
}

enum FbErrorCode GFX_create_render_program(struct FbGpu *gpu, struct BalDescriptorTable *table, struct FbGfxRenderProgramDesc desc, struct FbGfxRenderProgram *program)
{
    struct BalSpirv *vsh = 0;
    if (!GFX_find_shader(gpu, table, desc.vsh_name, BAL_SHADER_VERTEX, &vsh)) {

    }

    struct BalSpirv *psh = 0;
    if (desc.psh_name && !GFX_find_shader(gpu, table, desc.psh_name, BAL_SHADER_PIXEL, &psh)) {

    }

    VkPipelineLayout pipeline_layout = 0;
    VkDescriptorSetLayout descriptor_layout = 0;
    GFX_create_pipeline_layout(gpu, vsh, psh, &pipeline_layout, &descriptor_layout);

    *program = (struct FbGfxRenderProgram) {
        .vsh = vsh,
        .psh = psh,
        .descriptor_layout = descriptor_layout,
        .pipeline_layout = pipeline_layout,
        .layout = desc.layout,
        .cached_pipelines = 0,
        .pipeline_bits = 0
    };
    GFX_find_pipeline(program, gpu, 0);
    return FB_ERR_NONE;
}

u32 GFXVK_find_memory_type(struct FbGpu *gpu, u32 type_bits, enum FbGfxMemoryUsage usage)
{
    VkPhysicalDeviceMemoryProperties prop = gpu->memory_properties;

    VkMemoryPropertyFlags required = 0;
    VkMemoryPropertyFlags preferred = 0;

    switch (usage) {
        case FB_GFX_MEMORY_USAGE_GPU_ONLY:
            preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case FB_GFX_MEMORY_USAGE_CPU_ONLY:
            required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case FB_GFX_MEMORY_USAGE_CPU_TO_GPU:
            required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case FB_GFX_MEMORY_USAGE_GPU_TO_CPU:
            required |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            break;
        default:
            // TODO(fkaa): err
            break;
    }

    for (u32 i = 0; i < prop.memoryTypeCount; ++i) {
        if (((type_bits >> i) & 1) == 0) {
            continue;
        }

        VkMemoryPropertyFlags properties = prop.memoryTypes[i].propertyFlags;
        if ((properties & required) != required) {
            continue;
        }

        if ((properties & preferred) != preferred) {
            continue;
        }

        return i;
    }

    for (u32 i = 0; i < prop.memoryTypeCount; ++i) {
        if (((type_bits >> i) & 1) == 0) {
            continue;
        }

        VkMemoryPropertyFlags properties = prop.memoryTypes[i].propertyFlags;
        if ((properties & required) != required) {
            continue;
        }

        return i;
    }

    return 0xffffffff;
}

enum FbErrorCode GFX_create_staging_pool(struct FbGpu *gpu, u32 buffer_count, u64 size, struct FbGfxStagingPool *pool)
{
    VkBufferCreateInfo buffer_info = { 0 };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    struct FbGfxStagingBuffer *buffers = calloc(buffer_count, sizeof(*buffers));
    for (u32 i = 0; i < buffer_count; ++i) {
        buffers[i].offset = 0;

        vkCreateBuffer(gpu->device, &buffer_info, NULL, &buffers[i].buffer);
    }

    VkMemoryRequirements requirements = { 0 };

    for (u32 i = 0; i < buffer_count; ++i) {
        vkGetBufferMemoryRequirements(gpu->device, buffers[i].buffer, &requirements);
    }

    VkDeviceSize align_mod = requirements.size % requirements.alignment;
    VkDeviceSize align_size = (align_mod == 0) ? requirements.size : (requirements.size + requirements.alignment - align_mod);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = align_size * buffer_count;
    alloc_info.memoryTypeIndex = GFXVK_find_memory_type(gpu, requirements.memoryTypeBits, FB_GFX_MEMORY_USAGE_CPU_TO_GPU);

    VkDeviceMemory memory = 0;
    vkAllocateMemory(gpu->device, &alloc_info, NULL, &memory);

    for (u32 i = 0; i < buffer_count; ++i) {
        vkBindBufferMemory(gpu->device, buffers[i].buffer, memory, i * align_size);
    }

    u8 *mapped_data = 0;
    vkMapMemory(gpu->device, memory, 0, align_size * buffer_count, 0, &mapped_data);

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = 0;

    VkCommandPool command_pool = 0;
    vkCreateCommandPool(gpu->device, &pool_info, NULL, &command_pool);

    VkCommandBufferAllocateInfo cmd_buffer_info = {0};
    cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_info.commandPool = command_pool;
    cmd_buffer_info.commandBufferCount = 1;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    for (u32 i = 0; i < buffer_count; ++i) {
        vkAllocateCommandBuffers(gpu->device, &cmd_buffer_info, &buffers[i].command_buffer);
        vkCreateFence(gpu->device, &fence_info, NULL, &buffers[i].fence);
        vkBeginCommandBuffer(buffers[i].command_buffer, &begin_info);

        buffers[i].data = (u8 *)mapped_data + (i * align_size);
    }


    *pool = (struct FbGfxStagingPool) {
        .staging_memory = memory,
        .command_pool = command_pool,

        .mapped_data = mapped_data,

        .max_size = size,
        .current_buffer = 0,
        .buffer_count = buffer_count,

        .buffers = buffers
    };

    return FB_ERR_NONE;
}

void GFX_wait_staging_buffer(struct FbGfxStagingBuffer *buffer, struct FbGpu *gpu)
{
    if (!buffer->submitted) {
        return;
    }

    vkWaitForFences(gpu->device, 1, &buffer->fence, VK_TRUE, ULLONG_MAX);
    vkResetFences(gpu->device, 1, &buffer->fence);

    buffer->offset = 0;
    buffer->submitted = false;

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(buffer->command_buffer, &begin_info);
}

void GFX_staging_buffer(struct FbGfxStagingPool *pool, struct FbGpu *gpu, u32 size, u32 alignment, VkCommandBuffer *command_buffer, VkBuffer *buffer, VkDeviceSize *buffer_offset, u8 **buffer_data)
{
    if (size > pool->max_size) {
        // TOOD(fkaa): err
    }

    struct FbGfxStagingBuffer *staging_buffer = &pool->buffers[pool->current_buffer];
    u32 align_mod = staging_buffer->offset % alignment;
    staging_buffer->offset = ((align_mod == 0) ? staging_buffer->offset : (staging_buffer->offset + alignment - align_mod));

    if ((staging_buffer->offset + size) >= pool->max_size && !staging_buffer->submitted) {
        GFX_flush_staging_pool(pool, gpu);
    }

    staging_buffer = &pool->buffers[pool->current_buffer];
    if (staging_buffer->submitted) {
        GFX_wait_staging_buffer(staging_buffer, gpu);
    }

    *command_buffer = staging_buffer->command_buffer;
    *buffer = staging_buffer->buffer;
    *buffer_offset = staging_buffer->offset;
    u8 *data = staging_buffer->data + staging_buffer->offset;
    staging_buffer->offset += size;
    *buffer_data = data;
}

void GFX_flush_staging_pool(struct FbGfxStagingPool *pool, struct FbGpu *gpu)
{
    struct FbGfxStagingBuffer *buffer = &pool->buffers[pool->current_buffer];

    VkMemoryBarrier barrier = { 0 };
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
    vkCmdPipelineBarrier(
        buffer->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
        1, &barrier,
        0, NULL,
        0, NULL
    );
    vkEndCommandBuffer(buffer->command_buffer);

    VkMappedMemoryRange range = { 0 };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = pool->staging_memory;
    range.size = VK_WHOLE_SIZE;

    vkFlushMappedMemoryRanges(gpu->device, 1, &range);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer->command_buffer;

    vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, buffer->fence);
    buffer->submitted = true;

    pool->current_buffer = (pool->current_buffer + 1) % pool->buffer_count;
}

void GFX_update_buffer(struct FbGpu *gpu, struct FbGfxStagingPool *pool, struct FbGfxBuffer *buffer, const void *data, u32 size, u32 offset)
{
    VkBuffer stage_buffer = 0;
    VkCommandBuffer stage_cmd_buffer = 0;
    VkDeviceSize stage_offset = 0;
    u8 *stage_data = 0;

    GFX_staging_buffer(pool, gpu, size, 1, &stage_cmd_buffer, &stage_buffer, &stage_offset, &stage_data);

    memcpy(stage_data, data, size);

    VkBufferCopy copy = {0};
    copy.srcOffset = stage_offset;
    copy.dstOffset = buffer->offset + offset;
    copy.size = size;

    vkCmdCopyBuffer(stage_cmd_buffer, stage_buffer, buffer->buffer, 1, &copy);
}

enum FbErrorCode GFX_create_vertex_buffer(struct FbGpu *gpu, u32 size, struct FbGfxStagingPool *pool, const void *data, struct FbGfxBuffer *buffer)
{
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo create_info = {0};
    create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer vk_buffer = 0;
    VmaAllocation vma_alloc = 0;
    VmaAllocationInfo vma_info = { 0 };
    
    vmaCreateBuffer(gpu->allocator, &buffer_info, &create_info, &vk_buffer, &vma_alloc, &vma_info);

    *buffer = (struct FbGfxBuffer) {
        .size = size,
        .offset = 0,
        .buffer = vk_buffer,
        .vma_alloc = vma_alloc,
        .vma_info = vma_info
    };

    if (data) {
        GFX_update_buffer(gpu, pool, buffer, data, size, 0);
    }
    
    return FB_ERR_NONE;
}

enum FbErrorCode GFX_create_index_buffer(struct FbGpu *gpu, u32 size, struct FbGfxStagingPool *pool, const void *data, struct FbGfxBuffer *buffer)
{
    VkBufferCreateInfo buffer_info = { 0 };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo create_info = { 0 };
    create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer vk_buffer = 0;
    VmaAllocation vma_alloc = 0;
    VmaAllocationInfo vma_info = { 0 };

    vmaCreateBuffer(gpu->allocator, &buffer_info, &create_info, &vk_buffer, &vma_alloc, &vma_info);

    *buffer = (struct FbGfxBuffer) {
        .size = size,
            .offset = 0,
            .buffer = vk_buffer,
            .vma_alloc = vma_alloc,
            .vma_info = vma_info
    };

    if (data) {
        GFX_update_buffer(gpu, pool, buffer, data, size, 0);
    }

    return FB_ERR_NONE;
}

enum FbErrorCode GFX_create_sprite_batch(struct FbGpu *gpu, struct FbGfxStagingPool *pool, struct FbGfxRenderProgram *program, u32 buffer_count, u32 size, struct FbGfxSpriteBatch *batch)
{
    struct FbGfxBuffer *vertex_buffers = calloc(buffer_count, sizeof(*vertex_buffers));
    struct FbGfxBuffer *index_buffers = calloc(buffer_count, sizeof(*index_buffers));

    for (u32 i = 0; i < buffer_count; ++i) {
        GFX_create_vertex_buffer(gpu, size * sizeof(*batch->vertices), pool, NULL, &vertex_buffers[i]);
        GFX_create_index_buffer(gpu, size * sizeof(*batch->indices), pool, NULL, &index_buffers[i]);
    }

    *batch = (struct FbGfxSpriteBatch) {
        .vertex_buffers = vertex_buffers,
        .index_buffers = index_buffers,
        .buffer_count = buffer_count,
        .buffer_size = size,
        .current_buffer = 0,

        .program = program,

        .vertices = malloc(size * sizeof(*batch->vertices)),
        .indices = malloc(size * sizeof(*batch->indices)),

        .vertex_cursor = 0,
        .index_cursor = 0,

        .current_element = 0
    };

    return FB_ERR_NONE;
}

void GFX_sprite_batch_upload(struct FbGpu *gpu, struct FbGfxSpriteBatch *batch, struct FbGfxStagingPool *pool)
{
    GFX_update_buffer(gpu, pool, &batch->vertex_buffers[batch->current_buffer], batch->vertices, batch->vertex_cursor * sizeof(*batch->vertices), 0);
    GFX_update_buffer(gpu, pool, &batch->index_buffers[batch->current_buffer], batch->indices, batch->index_cursor * sizeof(*batch->indices), 0);
    
    // TODO(fkaa): move somewhere else..
    GFX_flush_staging_pool(pool, gpu);
}

void GFX_sprite_batch_draw(struct FbGpu *gpu, struct FbGfxSpriteBatch *batch, struct FbGfxStagingPool *pool, VkCommandBuffer buf)
{
    GFX_sprite_batch_upload(gpu, batch, pool);

    VkBuffer buffer;
    VkDeviceSize offset = 0;

    u32 current_buffer = batch->current_buffer;

    u64 state = 0;
    state |= (FB_STATE_SRCBLEND_SRC_ALPHA << FB_STATE_SRCBLEND_BITS);
    state |= (FB_STATE_DSTBLEND_ONE_MINUS_SRC_ALPHA << FB_STATE_DSTBLEND_BITS);
    state |= FB_STATE_BLEND_ENABLE;

    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, GFX_find_pipeline(batch->program, gpu, state));
    vkCmdBindVertexBuffers(buf, 0, 1, &batch->vertex_buffers[current_buffer].buffer, &offset);
    vkCmdBindIndexBuffer(buf, batch->index_buffers[current_buffer].buffer, offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(buf, batch->index_cursor, 1, 0, 0, 0);

    batch->current_buffer = (batch->current_buffer + 1) % batch->buffer_count;
    batch->vertex_cursor = 0;
    batch->index_cursor = 0;
    batch->current_element = 0;
}

void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, struct FbGfxSpriteVertex *vertices, u64 vertex_count, u32 *indices, u64 index_count)
{
    if (batch->vertex_cursor + vertex_count > batch->buffer_size ||
        batch->index_cursor + index_count > batch->buffer_size)
    {
        // TODO(fkaa): batch is full
    }

    memcpy(batch->vertices + batch->vertex_cursor, vertices, vertex_count * sizeof(*batch->vertices));
    memcpy(batch->indices + batch->index_cursor, indices, index_count * sizeof(*batch->indices));

    batch->current_element += (u32)vertex_count;
    batch->vertex_cursor += (u32)vertex_count;
    batch->index_cursor += (u32)index_count;
}

struct FbGfxDebugText {
    r32 x, y, z;
    u32 color;
    char msg[128];
};

struct FbGfxDebugText *GFX_debug_messages = 0;

static struct FbVec4 GFX_transform_to_screen(struct FbMatrix4 cam, struct FbVec4 pos, bool *clip)
{
    struct FbVec4 p = vec4_transform(pos, cam);

    // hmm
    if (p.w == .0f) p.w = 1.f;

    p = vec4_muls(p, 1.f / p.w);
    p = vec4_adds(vec4_muls(p, .5f), .5f);

    if (clip && p.z > 1.f) {
        *clip = true;
    }

    p.y = 1.f - p.y;
    p.x *= 800.f;
    p.y *= 600.f;


    return p;
}

void GFX_debug_text(r32 x, r32 y, r32 z, u32 color, const char *fmt, ...)
{
    struct FbGfxDebugText text = {
        .x = x,
        .y = y,
        .z = z,
        .color = color,
        .msg = {0}
    };

    va_list args;
    va_start(args, fmt);
    vsnprintf(text.msg, sizeof(text.msg), fmt, args);
    va_end(args);

    ARRAY_push(GFX_debug_messages, text);
}

void GFX_debug_draw(struct FbGfxSpriteBatch *batch, struct FbFont *font, struct FbMatrix4 cam)
{
    for (u32 i = 0; i < ARRAY_size(GFX_debug_messages); ++i) {
        struct FbGfxDebugText *text = &GFX_debug_messages[i];
        bool clip = false;
        struct FbVec4 pos = GFX_transform_to_screen(cam, (struct FbVec4) { text->x, text->y, text->z, 1.f }, &clip);

        if (!clip) {
            FONT_draw_string(font, batch, text->msg, pos.x, pos.y, text->color);
        }
    }
    ARRAY_reset(GFX_debug_messages);
}
