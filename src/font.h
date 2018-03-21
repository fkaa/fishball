#ifndef FB_FONT_H
#define FB_FONT_H

#include "shared/types.h"

struct BalFont;

struct FbFontStore;

struct FbFont {
    struct BalFont *bal_font;
};

struct FbGlyph {
    u16 x, y, w, h;
    u16 offset_x, offset_y;
    r32 advance_x, advance_y;
};

struct FbGlyphVertex {
    r32 x, y, z; // 12
    u32 color;   // 16
    u16 u, v, w; // 22
    u16 layer;   // 24
};

enum FbErrorCode;

enum FbErrorCode FONT_create_font_store(struct FbFontStore **store);
enum FbErrorCode FONT_load_font(struct BalDescriptorTable *table, const char *id, struct FbFont *font);
void FONT_stuff(struct FbFontStore *store, struct FbFont *font);


#endif /* FB_FONT_H */
