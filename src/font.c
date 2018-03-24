#include "font.h"
#include "fbgl.h"
#include "bal.h"
#include "gfx.h"
#include "mem.h"
#include "shared/error.h"

#include <stdlib.h>

struct FbFontStore {
    u32 texture_array;
};

enum FbErrorCode FONT_create_font_store(struct FbFontStore **store)
{
    u32 texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RED, 1024, 1024, 16, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    struct FbFontStore fstore;
    fstore.texture_array = texture;

    *store = malloc(sizeof(**store));
    **store = fstore;
    return FB_ERR_NONE;
}

enum FbErrorCode FONT_load_font(struct BalDescriptorTable *table, const char *id, struct FbFont *font)
{
    // TODO(fkaa): actually search for `id`
    struct BalFont *bfont = (struct BalFont *)BAL_PTR(table->descriptors[0].ref);
    struct FbFont f;
    f.bal_font = bfont;

    *font = f;
    return FB_ERR_NONE;
}

struct FbGfxShader FONT_shader = {0};
struct FbGfxInputLayout FONT_layout = {0};
struct FbGfxBuffer FONT_vertex_buffer = {0};
struct FbGfxBuffer FONT_index_buffer = {0};

enum FbErrorCode FONT_enable_drawing()
{
    struct FbGfxShaderFile files[] = {
        { .path = "asset/shader/font.glslv", .type = FB_GFX_VERTEX_SHADER },
        { .path = "asset/shader/font.glslf", .type = FB_GFX_PIXEL_SHADER },
    };
    GFX_load_shader_files(files, 2, &FONT_shader);

    u64 stride = sizeof(struct FbGlyphVertex);
    struct FbGfxVertexEntry desc[] = {
        { .name = "PositionVS", .type = FB_GFX_FLOAT,          .count = 3, .normalized = false, .stride = stride, .offset = 0 },
        { .name = "ColorVS",    .type = FB_GFX_UNSIGNED_BYTE,  .count = 4, .normalized = true,  .stride = stride, .offset = 3 * sizeof(r32) },
        { .name = "TexCoordVS", .type = FB_GFX_UNSIGNED_SHORT, .count = 4, .normalized = false, .stride = stride, .offset = 3 * sizeof(r32) + sizeof(u32) },
    };
    GFX_create_input_layout(desc, 3, &FONT_layout);

    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = 0,
            .length = MiB(4),
            .type = FB_GFX_VERTEX_BUFFER,
            .usage = FB_GFX_USAGE_DYNAMIC_WRITE
        };
        GFX_create_buffer(&buffer_desc, &FONT_vertex_buffer);
    }

    {
        struct FbGfxBufferDesc buffer_desc = {
            .data = 0,
            .length = MiB(1),
            .type = FB_GFX_INDEX_BUFFER,
            .usage = FB_GFX_USAGE_DYNAMIC_WRITE
        };
        GFX_create_buffer(&buffer_desc, &FONT_index_buffer);
    }

    return FB_ERR_NONE;
}

enum FbErrorCode FONT_disable_drawing()
{
    return FB_ERR_NONE;
}

bool FONT_find_glyph(struct BalFont *font, u32 codepoint, struct BalGlyph *glyph)
{
    u32 start = codepoint;
    u32 end = font->glyph_count;
    while (start <= end) {
        u32 mid = (start + end) / 2;
        if (codepoint == font->glyphs[mid].codepoint) {
            *glyph = font->glyphs[mid];
            return true;
        } else if (codepoint < font->glyphs[mid].codepoint) {
            end = mid - 1;
        } else {
            start = mid + 1;
        }
    }

    return false;
}

void FONT_draw_string(struct FbFont *font, const char *string, r32 x, r32 y)
{
    r32 cursor_x = x;
    r32 cursor_y = y;
    struct BalGlyph glyph;

    for (u32 i = 0; string[i] != '\0'; ++i) {
        char c = string[i];
        u32 codepoint = c;
        
        if (FONT_find_glyph(font->bal_font, codepoint, &glyph)) {
            printf("glyph found (%d) for character (%c)\n", glyph.codepoint, c); 
            cursor_x += glyph.xadvance;
        }
    }
}

void FONT_stuff(struct FbFontStore *store, struct FbFont *font)
{
    struct BalBuffer *buffer = (struct BalBuffer *)BAL_PTR(font->bal_font->buffer);
    u32 layers = buffer->size / (font->bal_font->texture_size * font->bal_font->texture_size);
    u32 width = font->bal_font->texture_size;
    u32 height = font->bal_font->texture_size;

    printf(
            "layers: %d, width: %d, height: %d\n", layers, width, height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, store->texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, layers, GL_RED, GL_UNSIGNED_BYTE, buffer->data);
}
