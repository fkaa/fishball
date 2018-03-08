#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#include "window.h"
#include "fbgl.h"
#include "voxel.h"

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
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
                struct FbVoxel voxel = VXL_find_voxel(world, i, j, k);
                printf("%d, ", voxel.type);
            }
        }
    }
    FBGL_load_procs();

    while (1) {
        glEnable(GL_BLEND);
        window_swap(wnd);
        window_poll(wnd);
    }
    return 0;
}
