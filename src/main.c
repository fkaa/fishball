#include "window.h"
#include "fbgl.h"
#include "voxel.h"

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

    FBGL_load_procs();

    while (1) {
        glEnable(GL_BLEND);
        window_swap(wnd);
        window_poll(wnd);
    }
    return 0;
}
