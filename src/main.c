#include "window.h"
#include "fbgl.h"
#include "voxel.h"
#include "array.h"
#include "gfx.h"
#include "mathimpl.h"
#include "font.h"
#include "bal.h"
#include "mem.h"
#include "import.h"
#include "helper.h"
#include "shared/error.h"

#include "Remotery.h"

#include <stdlib.h>
#include <string.h>

struct BalSpirv *BAL_find_shader(struct BalDescriptorTable *table, const char *name, enum BalShaderStage stage)
{
    for (u32 i = 0; i < table->descriptor_count; ++i) {
        struct BalDescriptor *desc = &table->descriptors[i];

        if (desc->type == BAL_TYPE_SPRV) {
            struct BalSpirv *spirv = BAL_PTR(desc->ref);
            BalString shader_name = BAL_PTR(spirv->name);

            if (spirv->stage == stage && strcmp(name, shader_name) == 0) {
                return spirv;
            }
        }
    }

    return 0;
}

enum FbErrorCode run()
{
    Remotery *rmt;
    if (RMT_ERROR_NONE != rmt_CreateGlobalInstance(&rmt)) {
        return ERR_fmt(FB_ERR_REMOTERY_INIT, "Could not initialize remotery");
    }

    u32 width = 800, height = 600;
    struct FbWindowConfig wnd_cfg = {
        .width = 800,
        .height = 600,
        .title = "FB",
        .validation_layer = 1
    };

    struct FbGpu gpu = {0};
    struct FbWindow *wnd = 0;
    enum FbErrorCode err = window_new(wnd_cfg, &wnd, &gpu);
    if (err != FB_ERR_NONE) {
        return err;
    }


//    window_cxt(wnd);
//    FBGL_load_procs();

    struct FbVoxelWorldConfig cfg = {
        .chunk_count_x = 1,
        .chunk_count_y = 1,
        .chunk_count_z = 1
    };
    struct FbVoxelWorld *world;
    //VXL_new_world(cfg, &world);

    struct BalHeader *bal_header = 0;
    BAL_import("asset/fish.bal", &bal_header);
    struct BalDescriptorTable *table = BAL_PTR(bal_header->descriptor_tables);
    printf("fish.bal:\n");
    printf("  descriptor_count: %d\n", table->descriptor_count);
    for (u32 i = 0; i < table->descriptor_count; ++i) {
        struct BalDescriptor *desc = &table->descriptors[i];
        switch (desc->type) {
        case BAL_TYPE_SPRV:
            {
                struct BalSpirv *spirv = BAL_PTR(desc->ref);
                BalString shader_name = BAL_PTR(spirv->name);
                printf("  #%d(spirv): %s stage=%d\n", i, shader_name, spirv->stage);
            }
            break;
        case BAL_TYPE_FONT:
            {
                struct BalFont *font = BAL_PTR(desc->ref);
                printf("  #%d(font): %s glyphs=%d, size=%d\n", i, "temp", font->glyph_count, font->texture_size);
            }
            break;
        default:
            printf("  #%d: unknown descriptor: %04x\n", i, desc->type);
            break;
        }
    }




    VkVertexInputBindingDescription bindings = {
        .binding = 0,
        .stride = sizeof(struct FbGlyphVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription attributes[] = {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM,   .offset = offsetof(struct FbGfxSpriteVertex, color) },
        { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(struct FbGfxSpriteVertex, u) },
    };

    struct FbGfxRenderProgramDesc program_desc[] = {
        [0] = { "font_vertex", "font_pixel", { 1, &bindings, 3, attributes }},
        [1] = { "triangle_vertex", "triangle_pixel", { 0, NULL, 0, NULL }}
    };
    struct FbGfxRenderProgram program = { 0 };
    GFX_create_render_program(&gpu, table, program_desc[0], &program);

    struct FbGfxRenderProgram tri_program = { 0 };
    GFX_create_render_program(&gpu, table, program_desc[1], &tri_program);

    struct FbGfxStagingPool pool = { 0 };
    GFX_create_staging_pool(&gpu, wnd->buffer_count, MiB(16), &pool);

    struct FbGfxSpriteBatch batch;
    GFX_create_sprite_batch(&gpu, &pool, &program, wnd->buffer_count, 100000, &batch);

    struct FbFont *font;

    FONT_load_font(table, "unifont", &font);
//    FONT_enable_drawing();

    /*for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int k = 0; k < 32; k++) {
                if (rand() % 8 == 0)
                VXL_set_voxel(world, i, j, k, (struct FbVoxel) { .type = rand() });
            }
        }
    }
    struct FbVoxelChunk *chunk = 0;
    VXL_find_chunk(world, 0, 0, 0, &chunk);
    struct FbVoxelVertex *vertices = 0;
    VXL_create_geometry2(chunk, &vertices);*/

    struct FbGfxShader shader = {0};
    struct FbGfxShaderFile files[] = {
        { .path = "asset/shader/chunk.glslv", .type = FB_GFX_VERTEX_SHADER },
        { .path = "asset/shader/chunk.glslf", .type = FB_GFX_PIXEL_SHADER },
    };
//    GFX_load_shader_files(files, 2, &shader);

    struct FbGfxVertexEntry desc[] = {
        { .name = "PositionVS", .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 0 },
        { .name = "ColorVS",    .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = true, .stride = 12 * sizeof(u8), .offset = 4 * sizeof(u8) },
        { .name = "NormalVS",   .type = FB_GFX_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 8 * sizeof(u8) },
    };
    struct FbGfxInputLayout layout = {0};
//    GFX_create_input_layout(desc, 3, &layout);

    struct FbGfxBuffer buffer = {0};
    {
        /*struct FbGfxBufferDesc buffer_desc = {
            .data = (u8*)vertices,
            .length = sizeof(*vertices) * ARRAY_size(vertices),
            .type = FB_GFX_VERTEX_BUFFER,
            .usage = FB_GFX_USAGE_IMMUTABLE_READ
        };*/
//        GFX_create_buffer(&buffer_desc, &buffer);
    }

    struct FbGfxBuffer camera_buffer = {0};
    {
        /*struct FbGfxBufferDesc buffer_desc = {
            .data = (u8*)vertices,
            .length = sizeof(*vertices) * ARRAY_size(vertices),
            .type = FB_GFX_VERTEX_BUFFER,
            .usage = FB_GFX_USAGE_IMMUTABLE_READ
        };*/
//        GFX_create_buffer(&buffer_desc, &camera_buffer);
    }

    /*struct FbGfxBufferBinding bindings[] = {
        { .name = "Camera", .buffer = &camera_buffer, .offset = 0, .length = 256 }
    };*/

    struct FbMatrix4 proj = mat4_perspective_RH(60.f * 3.14f/180.f, 800.f / 600.f, .01f, 100.f);
    struct FbMatrix4 view = mat4_look_at_RH((struct FbVec3){10, 10, 10}, (struct FbVec3){0, 0, 0}, (struct FbVec3){0, 1, 0});
    struct FbMatrix4 ortho = mat4_ortho_RH(0.f, (r32)height, 0.f, (r32)width, 0.f, 1.f);

    float t = 0.f;
    while (window_open(wnd)) {
        window_poll(wnd);

        u32 image_idx = window_acquire_image(&gpu, wnd);
        //printf("image idx=%d\n", image_idx);


        VkCommandBuffer buf = gpu.command_buffers[image_idx];
        VkCommandBufferBeginInfo begin_info = { 0 };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkClearValue clear = { 0.f, 99 / 255.f, 177 / 255.f, 1.f };
        VkRenderPassBeginInfo renderpass_info = { 0 };
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_info.framebuffer = wnd->swapchain_fbos[image_idx];
        renderpass_info.renderPass = wnd->swapchain_renderpass;
        renderpass_info.renderArea = (VkRect2D) { {0, 0}, wnd->swapchain_extent };
        renderpass_info.clearValueCount = 1;
        renderpass_info.pClearValues = &clear;

        vkBeginCommandBuffer(buf, &begin_info);
        vkCmdBeginRenderPass(buf, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, tri_program.cached_pipelines[0]);

        VkViewport viewport = {
            0.f, 0.f,
            wnd->swapchain_extent.width,
            wnd->swapchain_extent.height,
            0.f, 1.f
        };
        vkCmdSetScissor(buf, 0, 1, &renderpass_info.renderArea);
        vkCmdSetViewport(buf, 0, 1, &viewport);
        vkCmdDraw(buf, 3, 1, 0, 0);

        FONT_draw_string(font, &batch, u8"\u300c\u792a\u79aa\u7948\u300d - a\u263bb\u263ac\u2602", 10, 26, 0xff22ff44);
        GFX_sprite_batch_draw(&gpu, &batch, &pool, buf);

        vkCmdEndRenderPass(buf);
        vkEndCommandBuffer(buf);

        window_submit(wnd, &gpu, &buf, 1);
        window_present(&gpu, wnd);
        t += 0.0001f;
    }

    window_destroy(wnd);

    rmt_DestroyGlobalInstance(rmt);

    return FB_ERR_NONE;
}

int main(int argc, char* argv[])
{
    if (run() != FB_ERR_NONE) {
        //fprintf(stderr, "%s\n", ERROR_buffer);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
