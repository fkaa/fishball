#ifndef FB_FONT_H
#define FB_FONT_H

#include "shared/types.h"

struct BalFont;
struct FbFontStore;
struct FbFont;
struct FbGfxSpriteBatch;


struct FbGlyph {
    u16 x, y, w, h;
    u16 offset_x, offset_y;
    r32 advance_x, advance_y;
};

struct FbGlyphVertex {
    r32 x, y, z; // 12
    u32 color;   // 16
    r32 u, v, w; // 22
};

enum FbErrorCode;

enum FbErrorCode FONT_create_font_store(struct FbFontStore **store);
enum FbErrorCode FONT_load_font(struct BalDescriptorTable *table, const char *id, struct FbFont **font);

enum FbErrorCode FONT_enable_drawing();
enum FbErrorCode FONT_disable_drawing();

u32 FONT_string_length(struct FbFont *font, const char *string);
void FONT_draw_string(struct FbFont *font, struct FbGfxSpriteBatch *batch, const char *string, r32 x, r32 y, u32 color);
void FONT_stuff(struct FbFontStore *store, struct FbFont *font);


#endif /* FB_FONT_H */
