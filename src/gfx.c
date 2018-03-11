#include "gfx.h"
#include "fbgl.h"
#include "file.h"

#include "shared/error.h"

#include <stdlib.h>
#include <string.h>

enum FbErrorCode GFX_load_shader_files(struct FbGfxShaderFile *files, unsigned int count, struct FbGfxShader *shader)
{
    GLuint program = glCreateProgram();
    for (unsigned int i = 0; i < count; ++i) {
        struct FbGfxShaderFile file = files[i];

        char *source = 0;
        u32 source_length = 0;
        // TODO(fkaa): err, free()
        FILE_read_whole(file.path, &source, &source_length);

        GLuint shader = glCreateShader(file.type);
        glShaderSource(shader, 1, &source, &source_length);
        glCompileShader(shader);

        GLint compile_status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status) {
            u32 len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            char *log = malloc(len + 1);
            glGetShaderInfoLog(shader, len, 0, log);

            printf("GFX: program compile error: %s\n", log);
            free(log);
        }

        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        u32 len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        char *log = malloc(len + 1);
        glGetProgramInfoLog(program, len, 0, log);

        printf("GFX: program link error: %s\n", log);
        free(log);
    }

    shader->program = program;

    return FB_ERR_NONE;
}

enum FbErrorCode GFX_create_input_layout(struct FbGfxVertexEntry *entries, u32 count, struct FbGfxInputLayout *layout)
{
    struct FbGfxInputLayout l = {0};
    glGenVertexArrays(1, &l.vao);
    l.desc = malloc(count * sizeof(*entries));
    l.count = count;
    memcpy_s(l.desc, count * sizeof(*entries), entries, count * sizeof(*entries));
    *layout = l;

    return FB_ERR_NONE;
}

void GFX_create_buffer(struct FbGfxBufferDesc *desc, struct FbGfxBuffer *buffer)
{
    glGenBuffers(1, &buffer->buffer);
    buffer->type = desc->type;
    glBindBuffer(desc->type, buffer->buffer);
    glBufferData(desc->type, desc->length, desc->data, desc->usage);
}

void GFX_update_buffer(struct FbGfxBuffer *buffer, u64 size, void *data)
{
    glBindBuffer(buffer->type, buffer->buffer);
    glBufferSubData(buffer->type, 0, size, data);
}

void GFX_set_vertex_buffers(struct FbGfxShader *shader, struct FbGfxBuffer *buffers, u32 buffer_count, struct FbGfxInputLayout *layout)
{
    glBindVertexArray(layout->vao);
    for (u32 i = 0; i < buffer_count; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[i].buffer);
        for (u32 j = 0; j < layout->count; ++j) {
            struct FbGfxVertexEntry entry = layout->desc[j];
            s32 attr = glGetAttribLocation(shader->program, entry.name);
            glEnableVertexAttribArray(attr);
            glVertexAttribPointer(attr, entry.count, entry.type, entry.normalized, entry.stride, (void*)entry.offset);
        }
    }
}

void GFX_set_uniform_buffers(struct FbGfxShader *shader, struct FbGfxBufferBinding *buffers, u32 buffer_count)
{
    for (u32 i = 0; i < buffer_count; ++i) {
        struct FbGfxBufferBinding binding = buffers[i];
        u32 idx = glGetUniformBlockIndex(shader->program, binding.name);
        glBindBufferRange(GL_UNIFORM_BUFFER, idx, binding.buffer->buffer, binding.offset, binding.length);
    }
}

void GFX_draw(u32 vertices)
{
    glDrawArrays(GL_TRIANGLES, 0, vertices);
}

