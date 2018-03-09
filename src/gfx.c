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

        // TODO(fkaa): give FbGfxShaderType same values as GL
        GLenum type;
        switch (file.type) {
            case FB_GFX_VERTEX_SHADER:
                type = GL_VERTEX_SHADER;
                break;
            case FB_GFX_PIXEL_SHADER:
                type = GL_FRAGMENT_SHADER;
                break;
        }

        char *source = 0;
        // TODO(fkaa): err, free()
        FILE_read_whole(file.path, &source);
        int source_count = 1;

        GLuint shader = glCreateShader(file.type);
        glShaderSource(shader, 1, &source, &source_count);
        glCompileShader(shader);
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

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
