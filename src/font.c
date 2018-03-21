#include "font.h"
#include "fbgl.h"
#include "bal.h"
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
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RED, 1024, 1024, 4, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

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

void FONT_stuff(struct FbFontStore *store, struct FbFont *font)
{
    u32 size = 1024 / 16;
    for (u32 x = 0; x < size; ++x) {
        for (u32 y = 0; y < size; ++y) {
            u16 codepoint = x + size * y;
            struct BalGlyph glyph = font->bal_font->glyphs[codepoint];
            struct BalBuffer *buffer = (struct BalBuffer *)BAL_PTR(font->bal_font->buffer);

            u32 width = glyph.width;
            u32 height = glyph.height;
            u8 *ptr = buffer->data + glyph.data_offset;

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glBindTexture(GL_TEXTURE_2D_ARRAY, store->texture_array);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x * 16, y * 16, 0, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, ptr);
        }
    }
}
