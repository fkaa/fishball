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
    glBindBuffer(buffer->buffer, desc->type);
    glBufferData(desc->type, desc->length, desc->data, desc->usage);
}
