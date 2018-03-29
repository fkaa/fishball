#include "gfx.h"
#include "fbgl.h"
#include "file.h"
#include "array.h"
#include "mathimpl.h"
#include "font.h"

#include "shared/error.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct FbGfxDebugText {
    r32 x, y, z;
    u32 color;
    char msg[128];
};

struct FbGfxDebugText *GFX_debug_messages = 0;

static struct FbVec4 GFX_transform_to_screen(struct FbMatrix4 cam, struct FbVec4 pos, bool *clip)
{
    struct FbVec4 p = vec4_transform(pos, cam);

    // hmm
    if (p.w == .0f) p.w = 1.f;

    p = vec4_muls(p, 1.f / p.w);
    p = vec4_adds(vec4_muls(p, .5f), .5f);

    if (clip && p.z > 1.f) {
        *clip = true;
    }

    p.y = 1.f - p.y;
    p.x *= 800.f;
    p.y *= 600.f;


    return p;
}

void GFX_debug_text(r32 x, r32 y, r32 z, u32 color, const char *fmt, ...)
{
    struct FbGfxDebugText text = {
        .x = x,
        .y = y,
        .z = z,
        .color = color,
        .msg = {0}
    };

    va_list args;
    va_start(args, fmt);
    vsnprintf(text.msg, sizeof(text.msg), fmt, args);
    va_end(args);

    ARRAY_push(GFX_debug_messages, text);
}

void GFX_debug_draw(struct FbGfxSpriteBatch *batch, struct FbFont *font, struct FbMatrix4 cam)
{
    for (u32 i = 0; i < ARRAY_size(GFX_debug_messages); ++i) {
        struct FbGfxDebugText *text = &GFX_debug_messages[i];
        bool clip = false;
        struct FbVec4 pos = GFX_transform_to_screen(cam, (struct FbVec4) { text->x, text->y, text->z, 1.f }, &clip);

        if (!clip) {
            FONT_draw_string(font, batch, text->msg, pos.x, pos.y, text->color);
        }
    }
    ARRAY_reset(GFX_debug_messages);
}


enum FbErrorCode GFX_create_sprite_batch(u64 vertex_size, u64 index_size, struct FbGfxSpriteBatch *batch)
{
    u32 buffers[2] = { 0, 0 };
    glGenBuffers(2, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, 0, FB_GFX_USAGE_DYNAMIC_WRITE);

    struct FbGfxBuffer camera_buffer = {0};
    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = 0,
            .length = sizeof(struct FbMatrix4),
            .type = FB_GFX_UNIFORM_BUFFER,
            .usage = FB_GFX_USAGE_STREAM_WRITE
        };
        GFX_create_buffer(&buffer_desc, &camera_buffer);
    }

    *batch = (struct FbGfxSpriteBatch) {
        .shader = 0,
        .layout = 0,
        .texture_bindings = { 0 },
        .texture_length = 0,

        .new_shader = 0,
        .new_layout = 0,
        .new_texture_bindings = { 0 },
        .new_texture_length = 0,

        .vertex_buffer = { buffers[0], GL_ARRAY_BUFFER },
        .index_buffer = { buffers[1], GL_ELEMENT_ARRAY_BUFFER },
        .camera_buffer = camera_buffer,

        .vertex_buffer_ptr = 0,
        .index_buffer_ptr = 0,

        .vertex_buffer_size = (u32)vertex_size,
        .index_buffer_size = (u32)index_size,

        .vertex_offset = 0,
        .index_offset = 0,

        .vertex_cursor = 0,
        .index_cursor = 0,

        .current_element = 0,
        .dirty = false,
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
    if (batch->vertex_cursor != batch->vertex_offset) {
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, batch->vertex_offset, batch->vertex_cursor - batch->vertex_offset);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    batch->vertex_buffer_ptr = 0;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
    if (batch->index_cursor != batch->index_offset) {
        glFlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER, batch->index_offset, batch->index_cursor - batch->index_offset);
    }
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    batch->index_buffer_ptr = 0;
}

void GFX_sprite_batch_draw(struct FbGfxSpriteBatch *batch)
{
    u32 index_offset = batch->index_offset;
    u32 index_len = (batch->index_cursor - index_offset) / sizeof(u32);

    if (index_len > 0) {
        struct FbGfxBufferBinding bindings[] = {
            { .name = "Camera", .buffer = &batch->camera_buffer, .offset = 0, .length = sizeof(struct FbMatrix4) }
        };

        GFX_set_vertex_buffers(batch->shader, &batch->vertex_buffer, 1, batch->layout);
        GFX_set_uniform_buffers(batch->shader, bindings, 1);
        GFX_set_textures(batch->shader, batch->texture_bindings, batch->texture_length);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer.buffer);
        glDrawElements(GL_TRIANGLES, index_len, GL_UNSIGNED_INT, (void *)(u64)index_offset);
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

    // TODO(fkaa): dont reset?
    batch->vertex_cursor = 0;
    batch->index_cursor = 0;

    batch->vertex_offset = 0;
    batch->index_offset = 0;
    batch->current_element = 0;
}


bool GFX_sprite_batch_needs_flush(struct FbGfxSpriteBatch *batch)
{
    if (batch->new_shader != batch->shader) {
        return true;
    }
    
    if (batch->new_layout != batch->layout) {
        return true;
    }

    if (batch->new_texture_length != batch->texture_length) {
        return true;
    }

    for (u32 i = 0; i < batch->texture_length; ++i) {
        struct FbGfxTextureBinding bind = batch->new_texture_bindings[i];
        struct FbGfxTextureBinding batch_bind = batch->texture_bindings[i];

        if (bind.texture == batch_bind.texture) continue;

        if (bind.texture->name != batch_bind.texture->name ||
            bind.texture->type != batch_bind.texture->type)
        {
            return true;
        }
    }

    return false;
}

void GFX_sprite_batch_append(struct FbGfxSpriteBatch *batch, void *vertices, u64 vertex_count, u64 vertex_size, u32 *indices, u64 index_size)
{
    if (GFX_sprite_batch_needs_flush(batch)) {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);
        
        batch->vertex_offset = batch->vertex_cursor;
        batch->index_offset = batch->index_cursor;

        batch->shader = batch->new_shader;
        batch->layout = batch->new_layout;
        batch->texture_length = batch->new_texture_length;
        memcpy(batch->texture_bindings, batch->new_texture_bindings, sizeof(struct FbGfxTextureBinding) * batch->texture_length);
    }

    if (batch->vertex_cursor + vertex_size > batch->vertex_buffer_size ||
        batch->index_cursor + index_size * sizeof(u32) > batch->index_buffer_size)
    {
        GFX_sprite_batch_unmap_buffers(batch);
        GFX_sprite_batch_draw(batch);
        GFX_sprite_batch_map_buffers(batch);

        
        batch->vertex_cursor = 0;
        batch->index_cursor = 0;

        batch->vertex_offset = 0;
        batch->index_offset = 0;

        batch->current_element = 0;
    }

    memcpy((char *)batch->vertex_buffer_ptr + batch->vertex_cursor, vertices, vertex_count * vertex_size);
    memcpy((char *)batch->index_buffer_ptr + batch->index_cursor, indices, index_size * sizeof(u32));

    batch->current_element += (u32)vertex_count;
    batch->vertex_cursor += (u32)vertex_count * (u32)vertex_size;
    batch->index_cursor += (u32)index_size * sizeof(u32);
}

void GFX_sprite_batch_set_transform(struct FbGfxSpriteBatch *batch, struct FbMatrix4 transform)
{
    GFX_update_buffer(&batch->camera_buffer, sizeof(transform), &transform);
}

void GFX_sprite_batch_set_textures(struct FbGfxSpriteBatch *batch, struct FbGfxTextureBinding *bindings, u32 texture_length)
{
    memcpy(batch->new_texture_bindings, bindings, sizeof(struct FbGfxTextureBinding) * texture_length);
    batch->new_texture_length = texture_length;
}

void GFX_sprite_batch_set_shader(struct FbGfxSpriteBatch *batch, struct FbGfxShader *shader)
{
    batch->new_shader = shader;
}

void GFX_sprite_batch_set_layout(struct FbGfxSpriteBatch *batch, struct FbGfxInputLayout *layout)
{
    batch->new_layout = layout;
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

void GFX_set_textures(struct FbGfxShader *shader, struct FbGfxTextureBinding *textures, u32 texture_count)
{
    for (u32 i = 0; i < texture_count; ++i) {
        struct FbGfxTextureBinding binding = textures[i];

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(binding.texture->type, binding.texture->name);

        u32 idx = glGetUniformLocation(shader->program, binding.name);
        glUseProgram(shader->program);
        glUniform1i(idx, i);
    }
}

void GFX_draw(u32 vertices)
{
    glDrawArrays(GL_TRIANGLES, 0, vertices);
}

