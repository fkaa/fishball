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

/*
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

static VkPipelineShaderStageCreateInfo *GFX_create_pipeline_shaders(VkDevice device, struct BalSpirv *spirv, u32 shader_len)
{
    VkPipelineShaderStageCreateInfo *vk_shaders = malloc(shader_len * sizeof(*vk_shaders));

    for (u32 i = 0; i < shader_len; ++i) {
        struct BalBuffer *buf = BAL_PTR(spirv[i].buffer);

        VkShaderModuleCreateInfo module_info = {
            .sType = VK_STRUCTURE_SHADER_MODULE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .codeSize = buf->size,
            .pCode = (u32 *)buf->data
        };
        VkShaderModule *module = 0;
        vkCreateShaderModule(device, &module_info, 0, &module);

        vk_shaders[i] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .stage = get_vk_stage(spirv->type),
            .module = module,
            .pName = "main",
            .pSpecializationInfo = 0
        };
    }

    return vk_shaders;
}

enum FbErrorCode GFX_create_graphics_pipeline(struct FbGpu *gpu,
                                              struct BalSpirv *shaders,
                                              u32 shader_len,
                                              VkVertexInputBindingDescription *vertex_bindings,
                                              u32 bind_count,
                                              VkVertexInputAttributeDescription *vertex_attributes,
                                              u32 attr_count,
                                              VkViewport viewport,
                                              VkRect2D scissor,
                                              VkPipelineColorBlendAttachmentState *blend_attachments,
                                              u32 blend_count,
                                              struct FbGfxTargetDescription *targets,
                                              u32 target_count,
                                              struct FbGfxPipeline *pipeline)
{
    VkPipelineShaderStageCreateInfo *shader_info = GFX_create_pipeline_shaders(gpu->device, shaders, shader_len);
    VkPipelineVertexInputStateCreateInfo vertex_info = {
        .sType = VK_STRUCTURE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .vertexBindingDescriptionCount = bind_count,
        .pVertexBindingDescriptions = vertex_bindings,
        .vertexAttributeDescriptionCount = attr_count,
        .pVertexAttributeDescriptions = vertex_attributes
    };

    VkPipelineInputAssemblyStateCreateInfo ia_info = {
        .sType = VK_STRUCTURE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = 0
    };

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType = VK_STRUCTURE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo raster_info = {
        .sType = VK_STRUCTURE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthClampEnable = 1,
        .rasterizerDiscardEnable = 1,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = 0,
        .depthBiasConstantFactor = .0f,
        .depthBiasClamp = .0f,
        .depthBiasSlopeFactor = .0f,
        .lineWidth = 0.f,
    };

    VkPipelineMultisampleStateCreateInfo msaa_info = {
        .sType = VK_STRUCTURE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .rasterizationSamples = 1,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = .0f,
        .pSampleMask = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
        .sType = VK_STRUCTURE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = 0,
        .back = 0,
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.f
    };

    VkPipelineColorBlendStateCreateInfo blend_info = {
        .sType = VK_STRUCTURE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_FALSE,
        .attachmentCount = blend_count,
        .pAttachments = blend_attachments,
        .blendConstants = { 1.f, 1.f, 1.f, 1.f }
    };


    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .stageCount = shader_len,
        .pStages = shader_info,
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &ia_info,
        .pTesselationState = 0,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_info,
        .pMultisampleState = &msaa_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &blend_info,
        .pDynamicState = 0,

        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0
    };

    free(shader_info);
}*/

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


enum FbErrorCode GFX_create_sprite_batch(u64 vertex_size, u64 index_size, struct FbGfxSpriteBatch *batch)
{
    u32 buffers[2] = { 0, 0 };
    glGenBuffers(2, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    struct FbGfxBuffer camera_buffer = {0};
    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = 0,
            .length = sizeof(struct FbMatrix4),
            .type = FB_GFX_UNIFORM_BUFFER,
            .usage = FB_GFX_USAGE_STREAM_WRITE
        };
        GFX_create_buffer(&buffer_desc, &camera_buffer);
    }

    *batch = (struct FbGfxSpriteBatch) {
        .shader = 0,
        .layout = 0,
        .texture_bindings = { 0 },
        .texture_length = 0,

        .new_shader = 0,
        .new_layout = 0,
        .new_texture_bindings = { 0 },
        .new_texture_length = 0,

        .vertex_buffer = { buffers[0], GL_ARRAY_BUFFER },
        .index_buffer = { buffers[1], GL_ELEMENT_ARRAY_BUFFER },
        .camera_buffer = camera_buffer,

        .vertex_buffer_ptr = 0,
        .index_buffer_ptr = 0,

        .vertex_buffer_size = (u32)vertex_size,
        .index_buffer_size = (u32)index_size,

        .vertex_offset = 0,
        .index_offset = 0,

        .vertex_cursor = 0,
        .index_cursor = 0,

        .current_element = 0,
        .dirty = false,
    };

    return FB_ERR_NONE;
}

void GFX_sprite_batch_map_buffers(struct FbGfxSpriteBatch *batch)
{
    if (batch->vertex_buffer_ptr || batch->index_buffer_ptr) {
        printf("trying to map buffers that are already mapped!\n");
        return;
    }
    
    batch->vertex_offset = batch->vertex_cursor;
    batch->index_offset = batch->index_cursor;

    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.buffer);
    batch->vertex_buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, batch->vertex_cursor, batch->vertex_buffer_size - batch->vertex_cursor, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
    batch->index_buffer_ptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, batch->index_cursor, batch->index_buffer_size - batch->index_cursor, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
}

void GFX_sprite_batch_unmap_buffers(struct FbGfxSpriteBatch *batch)
{
    if (!batch->vertex_buffer_ptr || !batch->index_buffer_ptr) {
        printf("trying to unmap buffers that are not mapped!\n");
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.buffer);
    if (batch->vertex_cursor != batch->vertex_offset) {
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, batch->vertex_offset, batch->vertex_cursor - batch->vertex_offset);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    batch->vertex_buffer_ptr = 0;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
    if (batch->index_cursor != batch->index_offset) {
        glFlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER, batch->index_offset, batch->index_cursor - batch->index_offset);
    }
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    batch->index_buffer_ptr = 0;
}

void GFX_sprite_batch_draw(struct FbGfxSpriteBatch *batch)
{
    u32 index_offset = batch->index_offset;
    u32 index_len = (batch->index_cursor - index_offset) / sizeof(u32);

    if (index_len > 0) {
        struct FbGfxBufferBinding bindings[] = {
            { .name = "Camera", .buffer = &batch->camera_buffer, .offset = 0, .length = sizeof(struct FbMatrix4) }
        };

        GFX_set_vertex_buffers(batch->shader, &batch->vertex_buffer, 1, batch->layout);
        GFX_set_uniform_buffers(batch->shader, bindings, 1);
        GFX_set_textures(batch->shader, batch->texture_bindings, batch->texture_length);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
        glDrawElements(GL_TRIANGLES, index_len, GL_UNSIGNED_INT, (void *)(u64)index_offset);
    }
}

void GFX_sprite_batch_begin(struct FbGfxSpriteBatch *batch)
{
    GFX_sprite_batch_map_buffers(batch);
}

void GFX_sprite_batch_end(struct FbGfxSpriteBatch *batch)
{
    GFX_sprite_batch_unmap_buffers(batch);
    GFX_sprite_batch_draw(batch);

    // TODO(fkaa): dont reset?
    batch->vertex_cursor = 0;
    batch->index_cursor = 0;

    batch->vertex_offset = 0;
    batch->index_offset = 0;
    batch->current_element = 0;
}


bool GFX_sprite_batch_needs_flush(struct FbGfxSpriteBatch *batch)
{
    if (batch->new_shader != batch->shader) {
        return true;
    }
    
    if (batch->new_layout != batch->layout) {
        return true;
    }

    if (batch->new_texture_length != batch->texture_length) {
        return true;
    }

    for (u32 i = 0; i < batch->texture_length; ++i) {
        struct FbGfxTextureBinding bind = batch->new_texture_bindings[i];
        struct FbGfxTextureBinding batch_bind = batch->texture_bindings[i];

        if (bind.texture == batch_bind.texture) continue;

        if (bind.texture->name != batch_bind.texture->name ||
            bind.texture->type != batch_bind.texture->type)
        {
            return true;
        }
    }

    return false;
}

void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, void *vertices, u64 vertex_count, u64 vertex_size, u32 *indices, u64 index_size)
{
    if (GFX_sprite_batch_needs_flush(batch)) {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);
        
        batch->vertex_offset = batch->vertex_cursor;
        batch->index_offset = batch->index_cursor;

        batch->shader = batch->new_shader;
        batch->layout = batch->new_layout;
        batch->texture_length = batch->new_texture_length;
        memcpy(batch->texture_bindings, batch->new_texture_bindings, sizeof(struct FbGfxTextureBinding) * batch->texture_length);
    }

    if (batch->vertex_cursor + vertex_size > batch->vertex_buffer_size ||
        batch->index_cursor + index_size * sizeof(u32) > batch->index_buffer_size)
    {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);

        
        batch->vertex_cursor = 0;
        batch->index_cursor = 0;

        batch->vertex_offset = 0;
        batch->index_offset = 0;

        batch->current_element = 0;
    }

    memcpy((char *)batch->vertex_buffer_ptr + batch->vertex_cursor, vertices, vertex_count * vertex_size);
    memcpy((char *)batch->index_buffer_ptr + batch->index_cursor, indices, index_size * sizeof(u32));

    batch->current_element += (u32)vertex_count;
    batch->vertex_cursor += (u32)vertex_count * (u32)vertex_size;
    batch->index_cursor += (u32)index_size * sizeof(u32);
}

void GFX_sprite_batch_set_transform(struct FbGfxSpriteBatch *batch, struct FbMatrix4 transform)
{
    GFX_update_buffer(&batch->camera_buffer, sizeof(transform), &transform);
}

void GFX_sprite_batch_set_textures(struct FbGfxSpriteBatch *batch, struct FbGfxTextureBinding *bindings, u32 texture_length)
{
    memcpy(batch->new_texture_bindings, bindings, sizeof(struct FbGfxTextureBinding) * texture_length);
    batch->new_texture_length = texture_length;
}

void GFX_sprite_batch_set_shader(struct FbGfxSpriteBatch *batch, struct FbGfxShader *shader)
{
    batch->new_shader = shader;
}

void GFX_sprite_batch_set_layout(struct FbGfxSpriteBatch *batch, struct FbGfxInputLayout *layout)
{
    batch->new_layout = layout;
}

enum FbErrorCode GFX_load_shader_files(struct FbGfxShaderFile *files, unsigned int count, struct FbGfxShader *shader)
{
    GLuint program = glCreateProgram();
    for (unsigned int i = 0; i < count; ++i) {
        struct FbGfxShaderFile file = files[i];

        char *source = 0;
        u32 source_length = 0;
        // TODO(fkaa): err, free()
        FILE_read_whole(file.path, &source, &source_length);

        GLuint shader = glCreateShader(file.type);
        glShaderSource(shader, 1, &source, &source_length);
        glCompileShader(shader);

        GLint compile_status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status) {
            u32 len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            char *log = malloc(len + 1);
            glGetShaderInfoLog(shader, len, 0, log);

            printf("GFX: program compile error: %s\n", log);
            free(log);
        }

        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        u32 len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        char *log = malloc(len + 1);
        glGetProgramInfoLog(program, len, 0, log);

        printf("GFX: program link error: %s\n", log);
        free(log);
    }

    shader->program = program;

    return FB_ERR_NONE;
}

enum FbErrorCode GFX_create_input_layout(struct FbGfxVertexEntry *entries, u32 count, struct FbGfxInputLayout *layout)
{
    struct FbGfxInputLayout l = {0};
    glGenVertexArrays(1, &l.vao);
    l.desc = malloc(count * sizeof(*entries));
    l.count = count;
    memcpy_s(l.desc, count * sizeof(*entries), entries, count * sizeof(*entries));
    *layout = l;

    return FB_ERR_NONE;
}

void GFX_create_buffer(struct FbGfxBufferDesc *desc, struct FbGfxBuffer *buffer)
{
    glGenBuffers(1, &buffer->buffer);
    buffer->type = desc->type;
    glBindBuffer(desc->type, buffer->buffer);
    glBufferData(desc->type, desc->length, desc->data, desc->usage);
}

void GFX_update_buffer(struct FbGfxBuffer *buffer, u64 size, void *data)
{
    glBindBuffer(buffer->type, buffer->buffer);
    glBufferSubData(buffer->type, 0, size, data);
}

void GFX_set_vertex_buffers(struct FbGfxShader *shader, struct FbGfxBuffer *buffers, u32 buffer_count, struct FbGfxInputLayout *layout)
{
    glBindVertexArray(layout->vao);
    for (u32 i = 0; i < buffer_count; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[i].buffer);
        for (u32 j = 0; j < layout->count; ++j) {
            struct FbGfxVertexEntry entry = layout->desc[j];
            s32 attr = glGetAttribLocation(shader->program, entry.name);
            glEnableVertexAttribArray(attr);
            glVertexAttribPointer(attr, entry.count, entry.type, entry.normalized, entry.stride, (void*)entry.offset);
        }
    }
}

void GFX_set_uniform_buffers(struct FbGfxShader *shader, struct FbGfxBufferBinding *buffers, u32 buffer_count)
{
    for (u32 i = 0; i < buffer_count; ++i) {
        struct FbGfxBufferBinding binding = buffers[i];
        u32 idx = glGetUniformBlockIndex(shader->program, binding.name);
        glBindBufferRange(GL_UNIFORM_BUFFER, idx, binding.buffer->buffer, binding.offset, binding.length);
    }
}

void GFX_set_textures(struct FbGfxShader *shader, struct FbGfxTextureBinding *textures, u32 texture_count)
{
    for (u32 i = 0; i < texture_count; ++i) {
        struct FbGfxTextureBinding binding = textures[i];

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(binding.texture->type, binding.texture->name);

        u32 idx = glGetUniformLocation(shader->program, binding.name);
        glUseProgram(shader->program);
        glUniform1i(idx, i);
    }
}

void GFX_draw(u32 vertices)
{
    glDrawArrays(GL_TRIANGLES, 0, vertices);
}

