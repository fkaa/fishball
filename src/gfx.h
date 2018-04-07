#ifndef FB_GFX_H
#define FB_GFX_H

#include "shared/types.h"
#include "mathtypes.h"

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

enum FbErrorCode;

typedef u64 FbGfxStateBits;

#define FB_STATE_SRCBLEND_ONE                 (0ull << 0)
#define FB_STATE_SRCBLEND_ZERO                (1ull << 0)
#define FB_STATE_SRCBLEND_DST_COLOR           (2ull << 0)
#define FB_STATE_SRCBLEND_ONE_MINUS_DST_COLOR (3ull << 0)
#define FB_STATE_SRCBLEND_SRC_ALPHA           (4ull << 0)
#define FB_STATE_SRCBLEND_ONE_MINUS_SRC_ALPHA (5ull << 0)
#define FB_STATE_SRCBLEND_DST_ALPHA           (6ull << 0)
#define FB_STATE_SRCBLEND_ONE_MINUS_DST_ALPHA (7ull << 0)
#define FB_STATE_SRCBLEND_BITS                (7ull << 0)

#define FB_STATE_DSTBLEND_ONE                 (0ull << 3)
#define FB_STATE_DSTBLEND_ZERO                (1ull << 3)
#define FB_STATE_DSTBLEND_DST_COLOR           (2ull << 3)
#define FB_STATE_DSTBLEND_ONE_MINUS_DST_COLOR (3ull << 3)
#define FB_STATE_DSTBLEND_SRC_ALPHA           (4ull << 3)
#define FB_STATE_DSTBLEND_ONE_MINUS_SRC_ALPHA (5ull << 3)
#define FB_STATE_DSTBLEND_DST_ALPHA           (6ull << 3)
#define FB_STATE_DSTBLEND_ONE_MINUS_DST_ALPHA (7ull << 3)
#define FB_STATE_DSTBLEND_BITS                (7ull << 3)

#define FB_STATE_DEPTHFUNC_LESS               (0ull << 13)
#define FB_STATE_DEPTHFUNC_ALWAYS             (1ull << 13)
#define FB_STATE_DEPTHFUNC_GREATER            (2ull << 13)
#define FB_STATE_DEPTHFUNC_EQUAL              (3ull << 13)
#define FB_STATE_DEPTHFUNC_BITS               (3ull << 13)

#define FB_STATE_CULL_FRONT                   (0ull << 15)
#define FB_STATE_CULL_BACK                    (1ull << 15)
#define FB_STATE_CULL_NONE                    (2ull << 15)
#define FB_STATE_CULL_BITS                    (2ull << 15)

#define FB_STATE_CLOCKWISE                    (1ull << 58)
#define FB_STATE_BLEND_ENABLE                 (1ull << 59)
#define FB_STATE_DEPTH_TEST                   (1ull << 60)
#define FB_STATE_DEPTH_WRITE                  (1ull << 61)

enum FbGfxMemoryUsage {
    FB_GFX_MEMORY_USAGE_UNKNOWN,
    FB_GFX_MEMORY_USAGE_GPU_ONLY,
    FB_GFX_MEMORY_USAGE_CPU_ONLY,
    FB_GFX_MEMORY_USAGE_CPU_TO_GPU,
    FB_GFX_MEMORY_USAGE_GPU_TO_CPU,
};

struct FbGpu {
    VkInstance instance;
    VkDevice device;
    VkQueue graphics_queue;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VmaAllocator allocator;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

struct FbGfxVertexLayout {
    u32 binding_count;
    VkVertexInputBindingDescription *bindings;
    u32 attribute_count;
    VkVertexInputAttributeDescription *attributes;
};

struct FbGfxRenderProgramDesc {
    const char *vsh_name;
    const char *psh_name;
    struct FbGfxVertexLayout layout;
};

struct FbGfxRenderProgram {
    struct BalSpirv *vsh;
    struct BalSpirv *psh;
    struct FbGfxVertexLayout layout;
    VkDescriptorSetLayout descriptor_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline *cached_pipelines;
    u64 *pipeline_bits;
};

struct FbGfxStagingBuffer {
    VkCommandBuffer command_buffer;
    VkBuffer buffer;
    VkFence fence;
    VkDeviceSize offset;

    u8 *data;
    bool submitted;
};

struct FbGfxStagingPool {
    VkDeviceMemory staging_memory;
    VkCommandPool command_pool;

    u8 *mapped_data;

    u64 max_size;
    u32 current_buffer;
    u32 buffer_count;

    struct FbGfxStagingBuffer *buffers;
};

enum FbErrorCode GFX_create_staging_pool(struct FbGpu *gpu, u32 buffer_count, u64 size, struct FbGfxStagingPool *pool);
void GFX_staging_buffer(struct FbGfxStagingPool *pool, struct FbGpu *gpu, u32 size, u32 alignment, VkCommandBuffer *command_buffer, VkBuffer *buffer, VkDeviceSize *buffer_offset, u8 **buffer_data);
void GFX_flush_staging_pool(struct FbGfxStagingPool *pool, struct FbGpu *gpu);

struct FbGfxTargetDescription {
    VkAttachmentDescription desc;
    VkPipelineColorBlendAttachmentState blend;
};


struct FbGfxBufferDesc {
    u8 *data;
    u32 length;
    enum FbGfxBufferType type;
    enum FbGfxBufferUsage usage;
};

struct FbGfxBuffer {
    u32 size;
    u32 offset;
    VkBuffer buffer;
    VmaAllocation vma_alloc;
    VmaAllocationInfo vma_info;
};

struct FbGfxTexture {
    u32 name;
    u32 type;
};

enum FbGfxShaderType {
    FB_GFX_VERTEX_SHADER = 0x8B31,
    FB_GFX_PIXEL_SHADER = 0x8B30,
};

struct FbGfxShaderFile {
    const char *path;
    enum FbGfxShaderType type;
};

enum FbGfxDataType {
    FB_GFX_FLOAT = 0x1406,
    FB_GFX_INT32,
    FB_GFX_BYTE = 0x1400,
    FB_GFX_UNSIGNED_BYTE = 0x1401,
    FB_GFX_UNSIGNED_SHORT = 0x1403,
    FB_GFX_UNSIGNED_INT = 0x1405,
};

struct FbGfxVertexEntry {
    char *name;
    enum FbGfxDataType type;
    u32 count;
    u32 stride;
    u32 offset;
    bool normalized;
};

struct FbGfxInputLayout {
    u32 vao;
    u32 count;
    struct FbGfxVertexEntry *desc;
};

struct FbGfxBufferBinding {
    char *name;
    struct FbGfxBuffer *buffer;
    u32 offset;
    u32 length;
};

struct FbGfxTextureBinding {
    char *name;
    struct FbGfxTexture *texture;
};

struct FbGfxShader {
    u32 program;
};

enum FbGfxBufferType {
    FB_GFX_VERTEX_BUFFER = 0x8892,
    FB_GFX_INDEX_BUFFER = 0x8893,
    FB_GFX_UNIFORM_BUFFER = 0x8A11
};

enum FbGfxBufferUsage {
    FB_GFX_USAGE_STREAM_WRITE = 0x88E0,
    FB_GFX_USAGE_STREAM_READ = 0x88E1,
    FB_GFX_USAGE_STREAM_COPY = 0x88E2,
    FB_GFX_USAGE_IMMUTABLE_WRITE = 0x88E4,
    FB_GFX_USAGE_IMMUTABLE_READ = 0x88E5,
    FB_GFX_USAGE_IMMUTABLE_COPY = 0x88E6,
    FB_GFX_USAGE_DYNAMIC_WRITE = 0x88E8,
    FB_GFX_USAGE_DYNAMIC_READ = 0x88E9,
    FB_GFX_USAGE_DYNAMIC_COPY = 0x88EA,
};

struct FbGfxSpriteVertex {
    r32 x, y, z; // 12
    u32 color;   // 16
    r32 u, v, w; // 22
};

struct FbGfxSpriteBatch {
    struct FbGfxBuffer *vertex_buffers;
    struct FbGfxBuffer *index_buffers;
    u32 buffer_count;
    u32 buffer_size;
    u32 current_buffer;

    struct FbGfxBuffer camera_buffer;
    struct FbGfxRenderProgram *program;

    struct FbGfxSpriteVertex *vertices;
    u32 *indices;

    u32 vertex_cursor;
    u32 index_cursor;

    u32 current_element;
};

enum FbErrorCode GFX_create_sprite_batch(struct FbGpu *gpu, struct FbGfxStagingPool *pool, struct FbGfxRenderProgram *program, u32 buffer_count, u32 size, struct FbGfxSpriteBatch *batch);
void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, struct FbGfxSpriteVertex *vertices, u64 vertex_count, u32 *indices, u64 index_count);
void GFX_sprite_batch_draw(struct FbGpu *gpu, struct FbGfxSpriteBatch *batch, struct FbGfxStagingPool *pool, VkCommandBuffer buf);

// debug
void GFX_debug_text(r32 x, r32 y, r32 z, u32 color, const char *fmt, ...);
void GFX_debug_draw(struct FbGfxSpriteBatch *batch, struct FbFont *font, struct FbMatrix4 cam);

enum FbErrorCode GFX_create_render_program(struct FbGpu *gpu, struct BalDescriptorTable *table, struct FbGfxRenderProgramDesc desc, struct FbGfxRenderProgram *program);

#endif /* FB_GFX_H */
