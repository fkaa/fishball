#include "gfx.h"
#include "fbgl.h"
#include "file.h"

#include "shared/error.h"

#include <stdlib.h>
#include <string.h>

enum FbErrorCode GFX_create_sprite_batch(u64 vertex_size, u64 index_size, struct FbGfxSpriteBatch *batch)
{
    u32 buffers[2] = { 0, 0 };
    glGenBuffers(2, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    *batch = (struct FbGfxSpriteBatch) {
        .shader = 0,
        .layout = 0,

        .vertex_buffer = { buffers[0], GL_ARRAY_BUFFER },
        .index_buffer = { buffers[1], GL_ELEMENT_ARRAY_BUFFER },

        .vertex_buffer_ptr = 0,
        .index_buffer_ptr = 0,

        .vertex_buffer_size = vertex_size,
        .index_buffer_size = index_size,

        .vertex_offset = 0,
        .index_offset = 0,

        .vertex_cursor = 0,
        .index_cursor = 0,
    };

    return FB_ERR_NONE;
}

void GFX_sprite_batch_map_buffers(struct FbGfxSpriteBatch *batch)
{
    if (batch->vertex_buffer_ptr || batch->index_buffer_ptr) {
        printf("trying to map buffers that are already mapped!\n");
        return;
    }
    
    batch->vertex_offset = batch->vertex_cursor;
    batch->index_offset = batch->index_cursor;

    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.buffer);
    batch->vertex_buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, batch->vertex_cursor, batch->vertex_buffer_size - batch->vertex_cursor, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
    batch->index_buffer_ptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, batch->index_cursor, batch->index_buffer_size - batch->index_cursor, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
}

void GFX_sprite_batch_unmap_buffers(struct FbGfxSpriteBatch *batch)
{
    if (!batch->vertex_buffer_ptr || !batch->index_buffer_ptr) {
        printf("trying to unmap buffers that are not mapped!\n");
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.buffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    if (batch->vertex_cursor != batch->vertex_offset) {
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, batch->vertex_offset, batch->vertex_cursor - batch->vertex_offset);
    }
    batch->vertex_buffer_ptr = 0;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    if (batch->index_cursor != batch->index_offset) {
        glFlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER, batch->index_offset, batch->index_cursor - batch->index_offset);
    }
    batch->index_buffer_ptr = 0;
}

void GFX_sprite_batch_draw(struct FbGfxSpriteBatch *batch)
{
    u32 index_offset = batch->index_offset;
    u32 index_len = batch->index_cursor - index_offset;

    if (index_len > 0) {
        GFX_set_vertex_buffers(batch->shader, &batch->vertex_buffer, 1, batch->layout);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
        glDrawElements(GL_TRIANGLES, index_len, GL_UNSIGNED_SHORT, (void *)index_offset);
    }
}

void GFX_sprite_batch_begin(struct FbGfxSpriteBatch *batch)
{
    GFX_sprite_batch_map_buffers(batch);
}

void GFX_sprite_batch_end(struct FbGfxSpriteBatch *batch)
{
    GFX_sprite_batch_unmap_buffers(batch);
    GFX_sprite_batch_draw(batch);

    batch->vertex_cursor = 0;
    batch->index_cursor = 0;

    batch->vertex_offset = 0;
    batch->index_offset = 0;
}



void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, struct FbGfxShader *shader, struct FbGfxInputLayout *layout, void *vertices, u64 vertex_size, u16 *indices, u64 index_size)
{
    if (batch->shader->program != shader->program || batch->layout != layout) {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);
        
        batch->vertex_offset = batch->vertex_cursor;
        batch->index_offset = batch->index_cursor;

        batch->shader = shader;
        batch->layout = layout;
    }

    if (batch->vertex_cursor + vertex_size > batch->vertex_buffer_size ||
        batch->index_cursor + index_size * 2 > batch->index_buffer_size)
    {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);

        
        batch->vertex_cursor = 0;
        batch->index_cursor = 0;

        batch->vertex_offset = 0;
        batch->index_offset = 0;
    }

    memcpy((char *)batch->vertex_buffer_ptr + batch->vertex_offset, vertices, vertex_size);
    memcpy((char *)batch->index_buffer_ptr + batch->index_offset, indices, index_size * 2);

    batch->vertex_cursor += vertex_size;
    batch->index_cursor += index_size * 2;
}

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

