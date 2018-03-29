#include "voxel.h"
#include "array.h"
#include "gfx.h"

#include "shared\types.h"

#include <stdlib.h>
#include <stdio.h>

struct FbVoxelMemory {
    struct FbVoxelChunkPage *chunk_pages;
};

struct FbVoxelChunk {
    struct FbVoxelChunkPage slices[VIRTUAL_CHUNK_COUNT];
    struct FbGfxBuffer vertex_buffer;
    bool dirty;
};

struct FbVoxelWorld {
    struct FbVoxelMemory memory;

    unsigned int num_chunks;
    unsigned long long *chunk_keys;
    struct FbVoxelChunk *chunks;
};

struct FbGfxShader VXL_shader = {0};
struct FbGfxInputLayout VXL_layout = {0};

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

    // TODO(fkaa): move elsewhere
    struct FbGfxShaderFile files[] = {
        { .path = "asset/shader/chunk.glslv", .type = FB_GFX_VERTEX_SHADER },
        { .path = "asset/shader/chunk.glslf", .type = FB_GFX_PIXEL_SHADER },
    };
    GFX_load_shader_files(files, 2, &VXL_shader);

    struct FbGfxVertexEntry desc[] = {
        { .name = "PositionVS", .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 0 },
        { .name = "ColorVS",    .type = FB_GFX_UNSIGNED_BYTE, .count = 4, .normalized = true, .stride = 12 * sizeof(u8), .offset = 4 * sizeof(u8) },
        { .name = "NormalVS",   .type = FB_GFX_BYTE, .count = 4, .normalized = false, .stride = 12 * sizeof(u8), .offset = 8 * sizeof(u8) },
    };
    GFX_create_input_layout(desc, 3, &VXL_layout);

    w.num_chunks = 1;
    ARRAY_push(w.chunk_keys, VXL_encode_chunk_key(0, 0, 0));

    struct FbVoxelChunk chunk = { 0 };
    for (int i = 0; i < VIRTUAL_CHUNK_COUNT; i++) {
        chunk.slices[i] = (struct FbVoxelChunkPage) { 0 };// w.memory.chunk_pages[i];
    }
    ARRAY_push(w.chunks, chunk);

    *world = malloc(sizeof(**world));
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

void VXL_create_geometry2(struct FbVoxelChunk *chunk, struct FbVoxelVertex **vertices)
{
    struct FbVoxelVertex *local_vertex_buffer = *vertices;
    struct FbVoxelVertex local_vertices[8 * 6];
    for (int i = 0; i < VIRTUAL_CHUNK_COUNT; i++) {
        struct FbVoxelChunkPage virtual_chunk = chunk->slices[i];
        if (virtual_chunk.voxels) {
            for (u8 x = 0; x < VIRTUAL_CHUNK_SIZE; x++) {
                for (u8 y = 0; y < VIRTUAL_CHUNK_SIZE; y++) {
                    for (u8 z = 0; z < VIRTUAL_CHUNK_SIZE; z++) {
                        unsigned int center = VXL_encode_voxel_position(x, y, z);
                        unsigned int px = VXL_encode_voxel_position(x + 1, y, z);
                        unsigned int nx = VXL_encode_voxel_position(x - 1, y, z);
                        unsigned int py = VXL_encode_voxel_position(x, y + 1, z);
                        unsigned int ny = VXL_encode_voxel_position(x, y - 1, z);
                        unsigned int pz = VXL_encode_voxel_position(x, y, z + 1);
                        unsigned int nz = VXL_encode_voxel_position(x, y, z - 1);

                        struct FbVoxel current = virtual_chunk.voxels[center];
                        u8 type = current.type;
                        if (current.type == 0) continue;
                        unsigned int vertex_count = 0;
                        unsigned char r = type, g = type, b = type;

                        //if (virtual_chunk.voxels[ny].type == 0) {
                            struct FbVoxelVertex bot_sw = {
                                .x = x, .y = y, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = -1,.k =  0
                            };
                            struct FbVoxelVertex bot_nw = {
                                .x = x, .y = y, .z = z + 1, .r = r, .g = g, .b = b, .i = 0, .j = -1,.k =  0
                            };
                            struct FbVoxelVertex bot_ne = {
                                .x = x + 1, .y = y, .z = z + 1, .r = r, .g = g, .b = b, .i = 0, .j = -1,.k =  0
                            };
                            struct FbVoxelVertex bot_se = {
                                .x = x + 1, .y = y, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = -1,.k =  0
                            };

                            
                            local_vertices[vertex_count++] = bot_sw;
                            local_vertices[vertex_count++] = bot_se;
                            local_vertices[vertex_count++] = bot_ne;
                            local_vertices[vertex_count++] = bot_sw;
                            local_vertices[vertex_count++] = bot_ne;
                            local_vertices[vertex_count++] = bot_nw;
                        //}

                        //if (virtual_chunk.voxels[py].type == 0) {
                            struct FbVoxelVertex top_sw = {
                                .x = x, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 1, .k = 0
                            };
                            struct FbVoxelVertex top_nw = {
                                .x = x, .y = y+1, .z = z + 1, .r = r, .g = g, .b = b, .i = 0, .j = 1, .k = 0
                            };
                            struct FbVoxelVertex top_ne = {
                                .x = x + 1, .y = y+1, .z = z + 1, .r = r, .g = g, .b = b, .i = 0, .j = 1, .k = 0
                            };
                            struct FbVoxelVertex top_se = {
                                .x = x + 1, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 1, .k = 0
                            };

                            local_vertices[vertex_count++] = top_sw;
                            local_vertices[vertex_count++] = top_se;
                            local_vertices[vertex_count++] = top_ne;
                            local_vertices[vertex_count++] = top_sw;
                            local_vertices[vertex_count++] = top_ne;
                            local_vertices[vertex_count++] = top_nw;
                        //}

                        //if (virtual_chunk.voxels[nx].type == 0) {
                            struct FbVoxelVertex left_sw = {
                                .x = x, .y = y, .z = z, .r = r, .g = g, .b = b, .i = -1,.j =  0,.k =  0
                            };
                            struct FbVoxelVertex left_nw = {
                                .x = x, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = -1,.j =  0,.k =  0
                            };
                            struct FbVoxelVertex left_ne = {
                                .x = x, .y = y+1, .z = z + 1, .r = r, .g = g, .b = b, .i = -1,.j =  0,.k =  0
                            };
                            struct FbVoxelVertex left_se = {
                                .x = x, .y = y, .z = z + 1, .r = r, .g = g, .b = b, .i = -1,.j =  0,.k =  0
                            };

                            local_vertices[vertex_count++] = left_sw;
                            local_vertices[vertex_count++] = left_se;
                            local_vertices[vertex_count++] = left_ne;
                            local_vertices[vertex_count++] = left_sw;
                            local_vertices[vertex_count++] = left_ne;
                            local_vertices[vertex_count++] = left_nw;
                        //}

                        //if (virtual_chunk.voxels[px].type == 0) {
                            struct FbVoxelVertex right_sw = {
                                .x = x + 1, .y = y, .z = z, .r = r, .g = g, .b = b, .i = 1, .j = 0, .k = 0
                            };
                            struct FbVoxelVertex right_nw = {
                                .x = x + 1, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = 1, .j = 0, .k = 0
                            };
                            struct FbVoxelVertex right_ne = {
                                .x = x + 1, .y = y+1, .z = z+1, .r = r, .g = g, .b = b, .i = 1, .j = 0, .k = 0
                            };
                            struct FbVoxelVertex right_se = {
                                .x = x + 1, .y = y, .z = z+1, .r = r, .g = g, .b = b, .i = 1, .j = 0, .k = 0
                            };

                            local_vertices[vertex_count++] = right_sw;
                            local_vertices[vertex_count++] = right_se;
                            local_vertices[vertex_count++] = right_ne;
                            local_vertices[vertex_count++] = right_sw;
                            local_vertices[vertex_count++] = right_ne;
                            local_vertices[vertex_count++] = right_nw;
                        //}

                        //if (virtual_chunk.voxels[nz].type == 0) {
                            struct FbVoxelVertex front_sw = {
                                .x = x, .y = y, .z = z+1, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = 1
                            };
                            struct FbVoxelVertex front_nw = {
                                .x = x, .y = y+1, .z = z+1, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = 1
                            };
                            struct FbVoxelVertex front_ne = {
                                .x = x + 1, .y = y+1, .z = z+1, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = 1
                            };
                            struct FbVoxelVertex front_se = {
                                .x = x + 1, .y = y, .z = z+1, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = 1
                            };

                            local_vertices[vertex_count++] = front_sw;
                            local_vertices[vertex_count++] = front_se;
                            local_vertices[vertex_count++] = front_ne;
                            local_vertices[vertex_count++] = front_sw;
                            local_vertices[vertex_count++] = front_ne;
                            local_vertices[vertex_count++] = front_nw;
                        //}

                        //if (virtual_chunk.voxels[pz].type == 0) {
                            struct FbVoxelVertex back_sw = {
                                .x = x, .y = y, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = -1
                            };
                            struct FbVoxelVertex back_nw = {
                                .x = x, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = -1
                            };
                            struct FbVoxelVertex back_ne = {
                                .x = x + 1, .y = y+1, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = -1
                            };
                            struct FbVoxelVertex back_se = {
                                .x = x + 1, .y = y, .z = z, .r = r, .g = g, .b = b, .i = 0, .j = 0, .k = -1
                            };

                            local_vertices[vertex_count++] = back_sw;
                            local_vertices[vertex_count++] = back_se;
                            local_vertices[vertex_count++] = back_ne;
                            local_vertices[vertex_count++] = back_sw;
                            local_vertices[vertex_count++] = back_ne;
                            local_vertices[vertex_count++] = back_nw;
                        //}

                        if (vertex_count > 0) {
                            ARRAY_append(local_vertex_buffer, local_vertices, vertex_count);
                        }
                    }
                }
            }
        }
    }

    *vertices = local_vertex_buffer;
}

void VXL_create_geometry(struct FbVoxelChunk *chunk, struct FbVoxelVertex **vertices)
{
    struct FbVoxelVertex *local_vertices = *vertices;

    for (char x = 0; x < CHUNK_SIZE; x++) {
        for (char y = 0; y < CHUNK_SIZE; y++) {
            for (char z = 0; z < CHUNK_SIZE; z++) {
                struct FbVoxel voxel = VXL_chunk_get(chunk, x & VIRTUAL_CHUNK_MASK, y & VIRTUAL_CHUNK_MASK, z & VIRTUAL_CHUNK_MASK);

                unsigned char r = 255, g = 128, b = 64;
                struct FbVoxelVertex bot_sw = {
                    x, y, z, r, g, b, 0, -1, 0
                };
                struct FbVoxelVertex bot_nw = {
                    x, y, z + 1, r, g, b, 0, -1, 0
                };
                struct FbVoxelVertex bot_ne = {
                    x + 1, y, z + 1, r, g, b, 0, -1, 0
                };
                struct FbVoxelVertex bot_se = {
                    x + 1, y, z, r, g, b, 0, -1, 0
                };

                struct FbVoxelVertex top_sw = {
                    x, y+1, z, r, g, b, 0, 1, 0
                };
                struct FbVoxelVertex top_nw = {
                    x, y+1, z + 1, r, g, b, 0, 1, 0
                };
                struct FbVoxelVertex top_ne = {
                    x + 1, y+1, z + 1, r, g, b, 0, 1, 0
                };
                struct FbVoxelVertex top_se = {
                    x + 1, y+1, z, r, g, b, 0, 1, 0
                };

                struct FbVoxelVertex left_sw = {
                    x, y, z, r, g, b, -1, 0, 0
                };
                struct FbVoxelVertex left_nw = {
                    x, y+1, z, r, g, b, -1, 0, 0
                };
                struct FbVoxelVertex left_ne = {
                    x + 1, y+1, z, r, g, b, -1, 0, 0
                };
                struct FbVoxelVertex left_se = {
                    x + 1, y, z, r, g, b, -1, 0, 0
                };

                struct FbVoxelVertex right_sw = {
                    x, y, z+1, r, g, b, 1, 0, 0
                };
                struct FbVoxelVertex right_nw = {
                    x, y+1, z+1, r, g, b, 1, 0, 0
                };
                struct FbVoxelVertex right_ne = {
                    x + 1, y+1, z+1, r, g, b, 1, 0, 0
                };
                struct FbVoxelVertex right_se = {
                    x + 1, y, z+1, r, g, b, 1, 0, 0
                };

                struct FbVoxelVertex front_sw = {
                    x, y, z+1, r, g, b, 0, 0, 1
                };
                struct FbVoxelVertex front_nw = {
                    x, y+1, z+1, r, g, b, 0, 0, 1
                };
                struct FbVoxelVertex front_ne = {
                    x + 1, y+1, z+1, r, g, b, 0, 0, 1
                };
                struct FbVoxelVertex front_se = {
                    x + 1, y, z+1, r, g, b, 0, 0, 1
                };

                struct FbVoxelVertex back_sw = {
                    x, y, z, r, g, b, 0, 0, -1
                };
                struct FbVoxelVertex back_nw = {
                    x, y+1, z, r, g, b, 0, 0, -1
                };
                struct FbVoxelVertex back_ne = {
                    x + 1, y+1, z, r, g, b, 0, 0, -1
                };
                struct FbVoxelVertex back_se = {
                    x + 1, y, z, r, g, b, 0, 0, -1
                };

                ARRAY_push(local_vertices, bot_sw);
                ARRAY_push(local_vertices, bot_se);
                ARRAY_push(local_vertices, bot_ne);
                ARRAY_push(local_vertices, bot_sw);
                ARRAY_push(local_vertices, bot_se);
                ARRAY_push(local_vertices, bot_ne);

                ARRAY_push(local_vertices, top_sw);
                ARRAY_push(local_vertices, top_se);
                ARRAY_push(local_vertices, top_ne);
                ARRAY_push(local_vertices, top_sw);
                ARRAY_push(local_vertices, top_se);
                ARRAY_push(local_vertices, top_ne);

                ARRAY_push(local_vertices, left_sw);
                ARRAY_push(local_vertices, left_se);
                ARRAY_push(local_vertices, left_ne);
                ARRAY_push(local_vertices, left_sw);
                ARRAY_push(local_vertices, left_se);
                ARRAY_push(local_vertices, left_ne);

                ARRAY_push(local_vertices, right_sw);
                ARRAY_push(local_vertices, right_se);
                ARRAY_push(local_vertices, right_ne);
                ARRAY_push(local_vertices, right_sw);
                ARRAY_push(local_vertices, right_se);
                ARRAY_push(local_vertices, right_ne);

                ARRAY_push(local_vertices, front_sw);
                ARRAY_push(local_vertices, front_se);
                ARRAY_push(local_vertices, front_ne);
                ARRAY_push(local_vertices, front_sw);
                ARRAY_push(local_vertices, front_se);
                ARRAY_push(local_vertices, front_ne);

                ARRAY_push(local_vertices, back_sw);
                ARRAY_push(local_vertices, back_se);
                ARRAY_push(local_vertices, back_ne);
                ARRAY_push(local_vertices, back_sw);
                ARRAY_push(local_vertices, back_se);
                ARRAY_push(local_vertices, back_ne);
            }
        }
    }

    *vertices = local_vertices;
}
