#include "window.h"
#include "fbgl.h"
#include "voxel.h"
#include "array.h"
#include "gfx.h"
#include "math.h"
#include "font.h"
#include "bal.h"
#include "mem.h"
#include "import.h"
#include "helper.h"
#include "shared/error.h"

#include "Remotery.h"

#include <stdlib.h>  

enum FbErrorCode run()
{
    Remotery *rmt;
    if (RMT_ERROR_NONE != rmt_CreateGlobalInstance(&rmt)) {
        return ERR_fmt(FB_ERR_REMOTERY_INIT, "Could not initialize remotery");
    }

    u32 width = 800, height = 600;
    struct FbWindow *wnd = 0;
    window_new((struct FbWindowConfig) { .width = 800, .height = 600, .title = "Test Window" }, &wnd);
    window_cxt(wnd);
    FBGL_load_procs();

    struct FbVoxelWorldConfig cfg = {
        .chunk_count_x = 1,
        .chunk_count_y = 1,
        .chunk_count_z = 1
    };
    struct FbVoxelWorld *world;
    VXL_new_world(cfg, &world);

    struct BalHeader *bal_header = 0;
    BAL_import("asset/fish.bal", &bal_header);
    struct BalDescriptorTable *table = BAL_PTR(bal_header->descriptor_tables);
    printf("Loading 'fish.bal':\n");
    printf("\tdescriptor_count: %d\n", table->descriptor_count);

    struct FbGfxSpriteBatch batch;
    GFX_create_sprite_batch(KiB(2048), KiB(512), &batch);

    struct FbFont *font;

    FONT_load_font(table, "unifont", &font);
    FONT_enable_drawing();

    for (int i = 0; i < 32; i++) {
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
    VXL_create_geometry2(chunk, &vertices);

    struct FbGfxShader shader = {0};
    struct FbGfxShaderFile files[] = {
        { .path = "asset/shader/chunk.glslv", .type = FB_GFX_VERTEX_SHADER },
        { .path = "asset/shader/chunk.glslf", .type = FB_GFX_PIXEL_SHADER },
    };
    GFX_load_shader_files(files, 2, &shader);

    struct FbGfxVertexEntry desc[] = {
        { .name = "PositionVS", .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 0 },
        { .name = "ColorVS",    .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = true, .stride = 12 * sizeof(u8), .offset = 4 * sizeof(u8) },
        { .name = "NormalVS",   .type = FB_GFX_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 8 * sizeof(u8) },
    };
    struct FbGfxInputLayout layout = {0};
    GFX_create_input_layout(desc, 3, &layout);

    struct FbGfxBuffer buffer = {0};
    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = (u8*)vertices,
            .length = sizeof(*vertices) * ARRAY_size(vertices),
            .type = FB_GFX_VERTEX_BUFFER,
            .usage = FB_GFX_USAGE_IMMUTABLE_READ
        };
        GFX_create_buffer(&buffer_desc, &buffer);
    }

    struct FbGfxBuffer camera_buffer = {0};
    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = (u8*)vertices,
            .length = sizeof(*vertices) * ARRAY_size(vertices),
            .type = FB_GFX_VERTEX_BUFFER,
            .usage = FB_GFX_USAGE_IMMUTABLE_READ
        };
        GFX_create_buffer(&buffer_desc, &camera_buffer);
    }

    struct FbGfxBufferBinding bindings[] = {
        { .name = "Camera", .buffer = &camera_buffer, .offset = 0, .length = 256 }
    };

    struct FbMatrix4 proj = mat4_perspective_RH(60.f * 3.14f/180.f, 800.f / 600.f, .01f, 100.f);
    struct FbMatrix4 view = mat4_look_at_RH((struct FbVec3){10, 10, 10}, (struct FbVec3){0, 0, 0}, (struct FbVec3){0, 1, 0});
    struct FbMatrix4 ortho = mat4_ortho_RH(0, height, 0, width, 0.f, 1.f);

    GFX_sprite_batch_set_transform(&batch, ortho);

    printf("%d\n", ARRAY_size(vertices));
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = 0.f;
    while (window_open(wnd)) {
        rmt_BeginCPUSample(frame, 0); 
        view = mat4_look_at_RH((struct FbVec3){sinf(t)*50, sin(t*0.2)*70, cosf(t)*50}, (struct FbVec3){0, 0, 0}, (struct FbVec3){0, 1, 0});
        struct FbMatrix4 mat = mat4_mul(view, proj);
        GFX_update_buffer(&camera_buffer, sizeof(mat), &mat);

        glClearColor(.2f, .22f, .4f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        /*glUseProgram(shader.program);
        GFX_set_vertex_buffers(&shader, &buffer, 1, &layout);
        GFX_set_uniform_buffers(&shader, bindings, 1);
        GFX_draw(ARRAY_size(vertices));*/

        glDisable(GL_DEPTH_TEST);
        GFX_sprite_batch_begin(&batch);
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "t: %.3f", t);
        FONT_draw_string(font, &batch, time_str, 10, 10, 0xffffffff);
        FONT_draw_string(font, &batch, u8"\u300c\u792a\u79aa\u7948\u300d - a\u263bb\u263ac\u2602", 10, 26, 0xff22ff44);
        GFX_sprite_batch_end(&batch);

        window_swap(wnd);
        window_poll(wnd);

        t += 0.0001f;
        rmt_EndCPUSample();
    }

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
