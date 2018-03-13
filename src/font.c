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
    if (FT_New_Face(FONT_ft_library, path, 0, &face)) {
        // TODO(fkaa): err
    }

    FT_Set_Pixel_Sizes(face, 0, 24);

    struct FbFont f;
    f.font_face = face;

    *font = malloc(sizeof(**font));
    **font = f;
    return FB_ERR_NONE;
}

void FONT_stuff(struct FbFontStore *store, struct FbFont *font)
{
    s32 flags = 0;
    FT_Render_Mode mode = FT_RENDER_MODE_NORMAL;

    u32 glyph_index = FT_Get_Char_Index(font->font_face, 0xffff);
    FT_Load_Glyph(font->font_face, glyph_index, flags);
    FT_Render_Glyph(font->font_face->glyph, mode);
    FT_GlyphSlot slot = font->font_face->glyph;
    u8 *data = slot->bitmap.buffer;
    u32 height = slot->bitmap.rows;
    u32 width = slot->bitmap.width;

    glBindTexture(GL_TEXTURE_2D_ARRAY, store->texture_array);
}
