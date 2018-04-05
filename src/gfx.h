#ifndef FB_GFX_H
#define FB_GFX_H

#include "shared/types.h"
#include "mathtypes.h"

// TODO(fkaa): yuck.. gfxtypes.h?
#include <vulkan/vulkan.h>

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

struct FbGpu {
    VkInstance instance;
    VkDevice device;
    VkQueue graphics_queue;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
};

struct FbGfxPipeline {
    enum FbGfxStateBits state_bits;
    VkPipeline pipeline;
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
    u32 buffer;
    u32 type;
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


struct FbGfxSpriteBatch {
    struct FbGfxShader *shader;
    struct FbGfxInputLayout *layout;
    struct FbGfxTextureBinding texture_bindings[8];
    u32 texture_length;

    struct FbGfxShader *new_shader;
    struct FbGfxInputLayout *new_layout;
    struct FbGfxTextureBinding new_texture_bindings[8];
    u32 new_texture_length;

    struct FbGfxBuffer vertex_buffer;
    struct FbGfxBuffer index_buffer;
    struct FbGfxBuffer camera_buffer;

    void *vertex_buffer_ptr;
    void *index_buffer_ptr;

    u32 vertex_buffer_size;
    u32 index_buffer_size;

    u32 vertex_offset;
    u32 index_offset;

    u32 vertex_cursor;
    u32 index_cursor;

    u32 current_element;
    bool dirty;
};

// debug
void GFX_debug_text(r32 x, r32 y, r32 z, u32 color, const char *fmt, ...);
void GFX_debug_draw(struct FbGfxSpriteBatch *batch, struct FbFont *font, struct FbMatrix4 cam);

enum FbErrorCode GFX_create_render_program(struct FbGpu *gpu, struct BalDescriptorTable *table, struct FbGfxRenderProgramDesc desc, struct FbGfxRenderProgram *program);

// sprite batch
enum FbErrorCode GFX_create_sprite_batch(u64 vertex_size, u64 index_size, struct FbGfxSpriteBatch *batch);
void GFX_sprite_batch_begin(struct FbGfxSpriteBatch *batch);
void GFX_sprite_batch_end(struct FbGfxSpriteBatch *batch);
void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, void *vertices, u64 vertex_count, u64 vertex_size, u32 *indices, u64 index_size); 
void GFX_sprite_batch_set_transform(struct FbGfxSpriteBatch *batch, struct FbMatrix4 transform); 
void GFX_sprite_batch_set_textures(struct FbGfxSpriteBatch *batch, struct FbGfxTextureBinding *bindings, u32 texture_length);
void GFX_sprite_batch_set_shader(struct FbGfxSpriteBatch *batch, struct FbGfxShader *shader);
void GFX_sprite_batch_set_layout(struct FbGfxSpriteBatch *batch, struct FbGfxInputLayout *layout);

// low level
enum FbErrorCode GFX_load_shader_files(struct FbGfxShaderFile *files, u32 count, struct FbGfxShader *shader);
enum FbErrorCode GFX_create_input_layout(struct FbGfxVertexEntry *entries, u32 count, struct FbGfxInputLayout *layout);
void GFX_create_buffer(struct FbGfxBufferDesc *desc, struct FbGfxBuffer *buffer);
void GFX_update_buffer(struct FbGfxBuffer *buffer, u64 size, void *data);
void GFX_set_vertex_buffers(struct FbGfxShader *shader, struct FbGfxBuffer *buffers, u32 buffer_count, struct FbGfxInputLayout *layout);
void GFX_set_uniform_buffers(struct FbGfxShader *shader, struct FbGfxBufferBinding *buffers, u32 buffer_count);
void GFX_set_textures(struct FbGfxShader *shader, struct FbGfxTextureBinding *textures, u32 texture_count);
void GFX_draw(u32 vertices);

#endif /* FB_GFX_H */
