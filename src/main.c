#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#include "window.h"
#include "fbgl.h"
#include "voxel.h"
#include <windows.h>
double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;
    if (!QueryPerformanceFrequency(&li))
        printf("QueryPerformanceFrequency failed!\n");

    PCFreq = (double)(li.QuadPart) / 1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (double)(li.QuadPart - CounterStart) / PCFreq;
}

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

    StartCounter();
/*    for (int l = 0; l < 10000; l++)
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int k = 0; k < 32; k++) {
                VXL_set_voxel(world, i, j, k, (struct FbVoxel) { .type = 1 });
                //struct FbVoxel voxel = VXL_find_voxel(world, i, j, k);
                //printf("%d, ", voxel.type);
            }
        }
    }*/
    struct FbVoxelChunk *chunk = 0;
    VXL_find_chunk(world, 0, 0, 0, &chunk);
    struct FbVoxelVertices *vertices = 0;
    VXL_create_geometry(chunk, vertices);
    printf("%fms\n", GetCounter());
    FBGL_load_procs();

    while (1) {
        glEnable(GL_BLEND);
        window_swap(wnd);
        window_poll(wnd);
    }
    return 0;
}
