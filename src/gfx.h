#ifndef FB_GFX_H
#define FB_GFX_H

#include "shared/types.h"

enum FbGfxShaderType {
    FB_GFX_VERTEX_SHADER,
    FB_GFX_PIXEL_SHADER,
};

struct FbGfxShaderFile {
    const char *path;
    enum FbGfxShaderType type;
};

enum FbGfxDataType {
    FB_GFX_FLOAT = 0,
    FB_GFX_INT32,
    FB_GFX_BYTE,
};

struct FbGfxVertexEntry {
    u32 index;
    enum FbGfxDataType type;
    u32 count;
    u32 stride;
    u32 offset;
};


struct FbGfxInputLayout {
    u32 vao;
    u32 count;
    struct FbGfxVertexArrayEntry *desc;
};

struct FbGfxShader {
    u32 program;
};

struct FbGfxBufferDesc {
    u8 *data;
    u32 length;
};

struct FbGfxBuffer {
    u32 buffer;
};

enum FbErrorCode;

enum FbErrorCode GFX_load_shader_files(struct FbGfxShaderFile *files, u32 count, struct FbGfxShader *shader);
enum FbErrorCode GFX_create_input_layout(struct FbGfxVertexEntry *entries, u32 count, struct FbGfxInputLayout *layout);
void GFX_create_buffer(struct FbGfxBufferDesc *desc, struct FbGfxBuffer *buffer);

#endif /* FB_GFX_H */
