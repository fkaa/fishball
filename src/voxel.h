#ifndef FB_VXL_H
#define FB_VXL_H

// 32^3
#define CHUNK_SIZE_EXPONENT 5
#define CHUNK_SIZE (1 << CHUNK_SIZE_EXPONENT)
#define CHUNK_MASK (CHUNK_SIZE - 1)
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

// 16^3
#define VIRTUAL_CHUNK_SIZE_EXPONENT 4
#define VIRTUAL_CHUNK_SIZE (1 << VIRTUAL_CHUNK_SIZE_EXPONENT)
#define VIRTUAL_CHUNK_MASK (VIRTUAL_CHUNK_SIZE - 1)
#define VIRTUAL_CHUNK_VOLUME (VIRTUAL_CHUNK_SIZE * VIRTUAL_CHUNK_SIZE * VIRTUAL_CHUNK_SIZE)
#define VIRTUAL_CHUNK_COUNT (CHUNK_VOLUME / VIRTUAL_CHUNK_VOLUME)

#include "mathtypes.h"

struct FbVoxelWorldConfig {
    int chunk_count_x;
    int chunk_count_y;
    int chunk_count_z;
};

struct FbVoxel {
    char type;
};

struct FbVoxelChunkPage {
    struct FbVoxel *voxels;
};

struct FbVoxelVertex {
    unsigned char x, y, z;
    unsigned char _padding0;
    unsigned char r, g, b;
    unsigned char _padding1;
    unsigned char i, j, k;
    unsigned char _padding2;
};

struct FbVoxelWorld;
struct FbVoxelChunk;

void VXL_new_world(struct FbVoxelWorldConfig cfg, struct FbVoxelWorld **world);
void VXL_set_voxel(struct FbVoxelWorld *world, int x, int y, int z, struct FbVoxel voxel);
struct FbVoxel VXL_find_voxel(struct FbVoxelWorld *world, int x, int y, int z);
void VXL_find_chunk(struct FbVoxelWorld *world, int x, int y, int z, struct FbVoxelChunk **chunk);

void VXL_draw(struct FbVoxelWorld *world, struct FbMatrix4 cam);

void VXL_create_geometry(struct FbVoxelChunk *chunk, struct FbVoxelVertex **vertices);
void VXL_create_geometry2(struct FbVoxelChunk *chunk, struct FbVoxelVertex **vertices);

#endif /* FB_VXL_H */
