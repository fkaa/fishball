#include "voxel.h"
#include "array.h"

#include <stdlib.h>
#include <stdio.h>

struct FbVoxelMemory {
    struct FbVoxelChunkPage *chunk_pages;
};

struct FbVoxelWorld {
    struct FbVoxelMemory memory;

    unsigned int num_chunks;
    unsigned long long *chunk_keys;
    struct FbVoxelChunk *chunks;
};

static unsigned long long VXL_encode_chunk_key(int cx, int cy, int cz);

void VXL_request_chunk_page(struct FbVoxelMemory *memory, struct FbVoxelChunkPage *page)
{
    struct FbVoxelChunkPage new_page = (struct FbVoxelChunkPage) { .voxels = malloc(VIRTUAL_CHUNK_VOLUME) };

    ARRAY_push(memory->chunk_pages, new_page);

    *page = new_page;
}

void VXL_new_world(struct FbVoxelWorldConfig cfg, struct FbVoxelWorld **world)
{
    struct FbVoxelWorld w = {0};

    printf(
        "CHUNK_SIZE: %d\n"
        "CHUNK_VOLUME: %d\n"
        "VIRTUAL_CHUNK_SIZE: %d\n"
        "VIRTUAL_CHUNK_VOLUME: %d\n"
        "VIRTUAL_CHUNK_COUNT: %d\n",
        CHUNK_SIZE,
        CHUNK_VOLUME,
        VIRTUAL_CHUNK_SIZE,
        VIRTUAL_CHUNK_VOLUME,
        VIRTUAL_CHUNK_COUNT
    );

    /*for (int i = 0; i < VIRTUAL_CHUNK_COUNT; i++) {
        struct FbVoxelChunkPage page = (struct FbVoxelChunkPage) { .voxels = malloc(VIRTUAL_CHUNK_VOLUME) };
        ARRAY_push(w.memory.chunk_pages, page);
    }
    for (int i = 0; i < VIRTUAL_CHUNK_COUNT; i++) {
        for (int j = 0; j < VIRTUAL_CHUNK_VOLUME; j++) {
            w.memory.chunk_pages[i].voxels[j] = (struct FbVoxel) { .type = (i * j) % 255 };
        }
    }*/

    w.num_chunks = 1;
    ARRAY_push(w.chunk_keys, VXL_encode_chunk_key(0, 0, 0));

    struct FbVoxelChunk chunk = { 0 };
    for (int i = 0; i < VIRTUAL_CHUNK_COUNT; i++) {
        chunk.slices[i] = (struct FbVoxelChunkPage) { 0 };// w.memory.chunk_pages[i];
    }
    ARRAY_push(w.chunks, chunk);

    *world = malloc(sizeof(*world));
    **world = w;
}

static unsigned long long expand(int x)
{
    x &= 0x000003ff;
    x = (x ^ (x << 16)) & 0xff0000ff;
    x = (x ^ (x <<  8)) & 0x0300f00f;
    x = (x ^ (x <<  4)) & 0x030c30c3;
    x = (x ^ (x <<  2)) & 0x09249249;
    return x;
}

static unsigned long long VXL_encode_chunk_key(int cx, int cy, int cz)
{
    return expand(cx) + (expand(cy) << 1) + (expand(cz) << 2);
}

void VXL_find_chunk(struct FbVoxelWorld *world, int x, int y, int z, struct FbVoxelChunk **chunk)
{
    int cx = x >> CHUNK_SIZE_EXPONENT;
    int cy = y >> CHUNK_SIZE_EXPONENT;
    int cz = z >> CHUNK_SIZE_EXPONENT;
    unsigned long long key = VXL_encode_chunk_key(cx, cy, cz);

    unsigned int n = world->num_chunks;
    unsigned int i = key % n;
    unsigned long long *keys = world->chunk_keys;
    while (keys[i] != key && keys[i] != 0xffffffffffffffffULL) {
        i = (i + 1) % n;
    }

    if (keys[i] != 0xffffffffffffffffULL) {
        *chunk = &world->chunks[i];
    } else {
        *chunk = 0;
    }
}

unsigned int VXL_morton_LUT[48] = {
    0,1,8,9,64,65,72,73,512,513,520,521,576,577,584,585,
    0,2,16,18,128,130,144,146,1024,1026,1040,1042,1152,1154,1168,1170,
    0,4,32,36,256,260,288,292,2048,2052,2080,2084,2304,2308,2336,2340,
};

static unsigned int VXL_encode_voxel_position(char vx, char vy, char vz)
{
    unsigned int idx = 0;

    idx = VXL_morton_LUT[vz] << 2 |
          VXL_morton_LUT[vy] << 1 |
          VXL_morton_LUT[vx];

    return idx;
}

void VXL_chunk_set(struct FbVoxelWorld *world, struct FbVoxelChunk *chunk, int x, int y, int z, struct FbVoxel voxel)
{
    unsigned int virtual_chunk_index = ((x + 16 * (y + 16 * z)) >> 9); 
    unsigned int index = VXL_encode_voxel_position(x, y, z);

    struct FbVoxelChunkPage virtual_chunk = chunk->slices[virtual_chunk_index];

    if (!virtual_chunk.voxels) {
        VXL_request_chunk_page(&world->memory, &chunk->slices[virtual_chunk_index]);
        virtual_chunk = chunk->slices[virtual_chunk_index];
    }
    
    virtual_chunk.voxels[index] = voxel;
}

struct FbVoxel VXL_chunk_get(struct FbVoxelChunk *chunk, char x, char y, char z)
{
    struct FbVoxel voxel = { 0 };

    unsigned int virtual_chunk_index = ((x + 16 * (y + 16 * z)) >> 9); 
    unsigned int index = VXL_encode_voxel_position(x, y, z);

    struct FbVoxelChunkPage virtual_chunk = chunk->slices[virtual_chunk_index];
    if (virtual_chunk.voxels) {
        voxel = virtual_chunk.voxels[index];
    }
    else {
        // TODO(fkaa): check if virtual chunk is backed by existing memory and
        //             pull into memory if so
    }

    return voxel;
}

void VXL_set_voxel(struct FbVoxelWorld *world, int x, int y, int z, struct FbVoxel voxel)
{
    struct FbVoxelChunk *chunk = 0;
    VXL_find_chunk(world, x, y, z, &chunk);

    if (!chunk) {

    }

    VXL_chunk_set(world, chunk, x & VIRTUAL_CHUNK_MASK, y & VIRTUAL_CHUNK_MASK, z & VIRTUAL_CHUNK_MASK, voxel);
}

struct FbVoxel VXL_find_voxel(struct FbVoxelWorld *world, int x, int y, int z)
{
    struct FbVoxel voxel = {0};
    struct FbVoxelChunk *chunk = 0;
    VXL_find_chunk(world, x, y, z, &chunk);

    if (chunk) {
        voxel = VXL_chunk_get(chunk, x & VIRTUAL_CHUNK_MASK, y & VIRTUAL_CHUNK_MASK, z & VIRTUAL_CHUNK_MASK);
    }


    return voxel;
}

void VXL_create_geometry(struct FbVoxelChunk *chunk, struct FbVoxelVertex *vertices)
{
    for (char x = 0; x < CHUNK_SIZE; x++) {
        for (char y = 0; y < CHUNK_SIZE; y++) {
            for (char z = 0; z < CHUNK_SIZE; z++) {
                struct FbVoxel voxel = VXL_chunk_get(chunk, x, y, z);

                unsigned char r = 255, g = 128, b = 64;
                struct FbVoxelVertex bot_sw = {
                    x, y, z,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex bot_nw = {
                    x, y, z + 1,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex bot_ne = {
                    x + 1, y, z + 1,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex bot_se = {
                    x + 1, y, z,
                    r, g, b,
                    0, -1, 0
                };

                struct FbVoxelVertex top_sw = {
                    x, y, z,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex top_nw = {
                    x, y, z + 1,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex top_ne = {
                    x + 1, y, z + 1,
                    r, g, b,
                    0, -1, 0
                };
                struct FbVoxelVertex top_se = {
                    x + 1, y, z,
                    r, g, b,
                    0, -1, 0
                };
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);

                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);

                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);

                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);

                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);

                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
                ARRAY_push(vertices, bot_sw);
                ARRAY_push(vertices, bot_se);
                ARRAY_push(vertices, bot_ne);
            }
        }
    }
}
