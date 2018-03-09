#include "window.h"
#include "fbgl.h"
#include "voxel.h"
#include "array.h"
#include "gfx.h"

#include <stdlib.h>  

int main() {
    struct FbWindow *wnd = 0;
    window_new((struct FbWindowConfig) { .width = 800, .height = 600, .title = "Test Window" }, &wnd);
    window_cxt(wnd);

    struct FbVoxelWorldConfig cfg = {
        .chunk_count_x = 1,
        .chunk_count_y = 1,
        .chunk_count_z = 1
    };
    struct FbVoxelWorld *world;
    VXL_new_world(cfg, &world);

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int k = 0; k < 32; k++) {
                VXL_set_voxel(world, i, j, k, (struct FbVoxel) { .type = 1 });
                //struct FbVoxel voxel = VXL_find_voxel(world, i, j, k);
                //printf("%d, ", voxel.type);
            }
        }
    }
    struct FbVoxelChunk *chunk = 0;
    VXL_find_chunk(world, 0, 0, 0, &chunk);
    struct FbVoxelVertex *vertices = 0;
    VXL_create_geometry2(chunk, &vertices);
    FBGL_load_procs();

    struct FbGfxShader *shader = 0;
    struct FbGfxShaderFile files[] = {
        { .path = "asset/chunk.glslv", .type = FB_GFX_VERTEX_SHADER },
        { .path = "asset/chunk.glslf", .type = FB_GFX_PIXEL_SHADER },
    };
    GFX_load_shader_files(files, 2, &shader);

    while (window_open(wnd)) {
        //glEnable(GL_BLEND);
        window_swap(wnd);
        window_poll(wnd);
    }
    return 0;
}
