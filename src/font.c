#include "font.h"
#include "fbgl.h"
#include "bal.h"
#include "gfx.h"
#include "mem.h"
#include "shared/error.h"

#include "utf8.h"

#include <stdlib.h>

struct FbFont {
    struct BalFont *bal_font;
    struct FbGfxTexture texture_array;
};

enum FbErrorCode FONT_create_font_store(struct FbFontStore **store)
{
    return FB_ERR_NONE;
}

enum FbErrorCode FONT_load_font(struct BalDescriptorTable *table, const char *id, struct FbFont **font)
{
    // TODO(fkaa): actually search for `id`

    struct BalFont *bfont = (struct BalFont *)BAL_PTR(table->descriptors[0].ref);
    struct FbFont f;
    f.bal_font = bfont;

    struct BalBuffer *buffer = (struct BalBuffer *)BAL_PTR(bfont->buffer);
    u32 layers = buffer->size / (bfont->texture_size * bfont->texture_size);
    u32 width = bfont->texture_size;
    u32 height = bfont->texture_size;

    printf("layers: %d, width: %d, height: %d\n", layers, width, height);

    u32 texture = 0;
 /*   glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RED, width, height, layers, 0, GL_RED, GL_UNSIGNED_BYTE, buffer->data);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
*/
    f.texture_array = (struct FbGfxTexture) { .type = GL_TEXTURE_2D_ARRAY, .name = texture };

    *font = malloc(sizeof(**font));
    **font = f;
    return FB_ERR_NONE;
}

struct FbGfxShader FONT_shader = {0};
struct FbGfxInputLayout FONT_layout = {0};
struct FbGfxBuffer FONT_vertex_buffer = {0};
struct FbGfxBuffer FONT_index_buffer = {0};

enum FbErrorCode FONT_enable_drawing()
{

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

u32 FONT_string_length(struct FbFont *font, const char *string)
{
    u32 cursor_x = 0;
    struct BalGlyph glyph;

    u32 codepoint = 0;
    while (*string != '\0' && (string = utf8codepoint(string, &codepoint)) != 0) {
        if (FONT_find_glyph(font->bal_font, codepoint, &glyph)) {
            cursor_x += glyph.xadvance;
        }
    }

    return cursor_x;
}

void FONT_draw_string(struct FbFont *font, struct FbGfxSpriteBatch *batch, const char *string, r32 x, r32 y, u32 color)
{
    struct FbGfxTextureBinding bindings[1] = {
        { "Texture", &font->texture_array }
    };

    //GFX_sprite_batch_set_shader(batch, &FONT_shader);
    //GFX_sprite_batch_set_layout(batch, &FONT_layout);
    //GFX_sprite_batch_set_textures(batch, bindings, 1);

    //u32 color = 0xff000088;
    r32 cursor_x = x;
    r32 cursor_y = y;
    struct BalGlyph glyph;

    u32 codepoint = 0;
    while (*string != '\0' && (string = utf8codepoint(string, &codepoint)) != 0) {
        if (FONT_find_glyph(font->bal_font, codepoint, &glyph)) {
            r32 glyph_pos_x = cursor_x + glyph.xoff;
            r32 glyph_pos_y = cursor_y + glyph.yoff;

            struct FbGfxSpriteVertex vertices[4] = {
                // top left
                { glyph_pos_x, glyph_pos_y, 0,
                  color,
                  (r32)glyph.x, (r32)glyph.y, (r32)glyph.layer
                },
                // top right
                { glyph_pos_x + (r32)glyph.width, glyph_pos_y, 0,
                  color,
                  (r32)(glyph.x + glyph.width), (r32)glyph.y, (r32)glyph.layer
                },
                // bottom left
                { glyph_pos_x, glyph_pos_y + glyph.height, 0,
                  color,
                  (r32)glyph.x, (r32)(glyph.y + glyph.height), (r32)glyph.layer 
                },
                // bottom right
                { glyph_pos_x + glyph.width, glyph_pos_y + glyph.height, 0,
                  color,
                  (r32)(glyph.x + glyph.width), (r32)(glyph.y + glyph.height), (r32)glyph.layer
                }
            };

            u32 offset = batch->current_element;

            u32 indices[6] = {
                offset + 2, offset + 1, offset + 0,
                offset + 2, offset + 3, offset + 1
            };

            GFX_sprite_batch_append(batch, vertices, 4, indices, 6);

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
    glBindTexture(GL_TEXTURE_2D_ARRAY, font->texture_array.name);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, layers, GL_RED, GL_UNSIGNED_BYTE, buffer->data);
}
