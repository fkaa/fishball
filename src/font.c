#include "font.h"
#include "fbgl.h"
#include "shared/error.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
// #include FT_ADVANCES_H
#include FT_LCD_FILTER_H

#include <stdlib.h>

// TODO(fkaa): binary search for codepoints rather than hashtable?
//
//

FT_Library FONT_ft_library;

struct FbFont {
    FT_Face font_face;
};

struct FbFontStore {
    u32 texture_array;
};

enum FbErrorCode FONT_init()
{
    if (FT_Init_FreeType(&FONT_ft_library)) {
        // TODO(fkaa): error
    }

    return FB_ERR_NONE;
}

enum FbErrorCode FONT_destroy()
{

    return FB_ERR_NONE;
}

enum FbErrorCode FONT_create_font_store(struct FbFontStore **store)
{
    u32 texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 1024, 1024, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    struct FbFontStore fstore;
    fstore.texture_array = texture;

    *store = malloc(sizeof(**store));
    **store = fstore;
    return FB_ERR_NONE;
}

enum FbErrorCode FONT_load_font(const char *path, struct FbFont **font)
{
    FT_Face face;

    s32 err = FT_New_Face(FONT_ft_library, path, 0, &face);
    if (err) {
        printf("%#x: Couldn't load font: %s\n", err, path);
        // TODO(fkaa): err
    }

    s32 size = 144;
    if (face->num_fixed_sizes > 0) {
        s32 best_match = 0;
        s32 diff = abs(size - face->available_sizes[0].width);
        for (s32 i = 1; i < face->num_fixed_sizes; ++i) {
            s32 ndiff =
                abs(size - face->available_sizes[i].width);
            if (ndiff < diff) {
                best_match = i;
                diff = ndiff;
            }
        }
        printf("selecting best match %d\n", best_match);
        FT_Select_Size(face, best_match);
    } else
        FT_Set_Pixel_Sizes(face, 0, size);

    struct FbFont f;
    f.font_face = face;

    *font = malloc(sizeof(**font));
    **font = f;
    return FB_ERR_NONE;
}

void FONT_stuff(struct FbFontStore *store, struct FbFont *font)
{
    s32 flags = FT_LOAD_DEFAULT;
    FT_Render_Mode mode = FT_RENDER_MODE_NORMAL;
    bool has_color = FT_HAS_COLOR(font->font_face);
    printf("has color: %d\n", has_color);
    u32 glyph_index = FT_Get_Char_Index(font->font_face, 'a');
    printf("glyph_index: %d\n", glyph_index);
    printf("load_glyph: %#x\n", FT_Load_Glyph(font->font_face, glyph_index, flags));
    printf("%d %d, %d\n", font->font_face->glyph->format, 'outl', 'bits');
    printf("render_glyph: %#x\n", FT_Render_Glyph(font->font_face->glyph, mode));
    printf("%d %d, %d\n", font->font_face->glyph->format, 'outl', 'bits');
    FT_GlyphSlot slot = font->font_face->glyph;
    u8 *data = slot->bitmap.buffer;
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
    } else {

    }

    printf("Pixel Mode: %d\n", slot->bitmap.pixel_mode);
    u32 height = slot->bitmap.rows;
    u32 width = slot->bitmap.width;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, store->texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, data);
}
