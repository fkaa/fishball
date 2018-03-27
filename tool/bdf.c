#include "bdf.h"
#include "array.h"
#include "time.h"
#include "mem.h"
#include "export.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#include <string.h>
#include <stdio.h>

s32 FILE_readline(FILE *f, char **line)
{
    char *l = *line;
    ARRAY_reset(l);

    s32 c;
    while ((c = getc(f)) != EOF && c != '\n') {
        ARRAY_push(l, c);
    }
    ARRAY_push(l, '\0');

    *line = l;

    return ((c == EOF) ? EOF : ARRAY_size(l));
}

bool BDF_check_bitmap_empty(u8 *bitmap)
{
    for (u32 i = 0; i < ARRAY_size(bitmap); ++i) {
        if (bitmap[i] != 0) return false;
    }

    return true;
}

void BDF_parse_bdf(const char *path, u32 texture_size, struct BalGlyph **glyphs, u8 **glyph_data, u64 *data_len)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "rb");
    if (error || !f) return;

    u32 cursor_x = 0, cursor_y = 0, cursor_z = 0;

    struct BalGlyph *local_glyphs = 0;
    struct stbrp_rect *glyph_rects = 0;
    u8 *local_glyph_data = MEM_virtual_alloc(MiB(100));

    struct BalGlyph glyph;
    u8 *bitmap = 0;
    char *line = 0;
    bool parse_bitmap = false;
    u16 codepoint = 0;
    u8 width = 0, height = 0;

    // parse the file completely while rasterizing the glyph bitmaps
    while (FILE_readline(f, &line) != -1) {
        char *substring = 0;
        if ((substring = strstr(line, "BBX")) != NULL) {
            substring += sizeof("BBX");
            s32 width, height, xoff, yoff;
            sscanf_s(substring, "%d %d %d %d", &width, &height, &xoff, &yoff);
            glyph.width = width;
            glyph.height = height;
            glyph.xoff = xoff;
            glyph.yoff = yoff;
        }
        else if ((substring = strstr(line, "DWIDTH")) != NULL) {
            substring += sizeof("DWIDTH");
            s32 xadvance;
            sscanf_s(substring, "%d", &xadvance);
            glyph.xadvance = xadvance;
        }
        else if ((substring = strstr(line, "STARTCHAR U+")) != NULL) {
            substring += sizeof("STARTCHAR U+") - 1;
            s32 codepoint = strtol(substring, NULL, 16);
            glyph.codepoint = codepoint;
        }
        else if (strstr(line, "BITMAP") != NULL) {
            parse_bitmap = true;
        }
        else if (strcmp(line, "ENDCHAR") == 0) {
            if (cursor_x + glyph.width > texture_size) {
                cursor_x = 0;
                cursor_y += glyph.height;
            }
            if (cursor_y + glyph.height > texture_size) {
                cursor_x = 0;
                cursor_y = 0;
                cursor_z += 1;
            }

            glyph.layer = cursor_z;
            glyph.x = cursor_x;
            glyph.y = cursor_y;

            // trim empty space around the glyph
            u32 trim_l = 255;
            u32 trim_t = 255;
            u32 trim_r = 0;
            u32 trim_b = 0;
            for (u32 x = 0; x < glyph.width; ++x) {
                for (u32 y = 0; y < glyph.height; ++y) {
                    u8 bit = bitmap[x + y * glyph.width];

                    if (bit) {
                        trim_l = min(x , trim_l);
                        trim_r = max(x + 1, trim_r);

                        trim_t = min(y , trim_t);
                        trim_b = max(y  + 1, trim_b);
                    }
                }
            }

            u32 stride = glyph.width;
            if (trim_l == 255) trim_l = 0;
            if (trim_t == 255) trim_t = 0;

            glyph.width = trim_r - trim_l;
            glyph.height = trim_b - trim_t;
            glyph.xoff += trim_l;
            glyph.yoff += trim_t;

            struct stbrp_rect glyph_rect = {
                .id = ARRAY_size(local_glyphs),
                .w = glyph.width,
                .h = glyph.height,
                .x = 0,
                .y = 0,
                .was_packed = 0
            };

            ARRAY_push(local_glyphs, glyph);
            ARRAY_push(glyph_rects, glyph_rect);

            // naively write into big glyph map
            for (u32 x = 0; x < glyph.width; ++x) {
                for (u32 y = 0; y < glyph.height; ++y) {
                    u32 write_x = x + cursor_x;
                    u32 write_y = y + cursor_y;
                    local_glyph_data[write_x + write_y * texture_size + (cursor_z * texture_size * texture_size)] = bitmap[(trim_l + x) + (trim_t + y) * stride];
                }
            }
            cursor_x += glyph.width;

            parse_bitmap = false;

            ARRAY_reset(bitmap);
        }
        else if (parse_bitmap) {
            u32 bits = (u32)strlen(line) * 4;
            u16 row = (u16)strtol(line, NULL, 16);
            for (s32 i = bits - 1; i >= 0; --i) {
                if (row & (1 << i)) {
                    ARRAY_push(bitmap, 0xff);
                } else {
                    ARRAY_push(bitmap, 0x0);
                }
            }
        }
    }

    // pack glyphs
    u8 *final_atlas = MEM_virtual_alloc(MiB(50));

    s32 num_nodes = texture_size * 32;
    stbrp_node *nodes = malloc(sizeof(*nodes) * num_nodes);
    stbrp_context *packer_cxt = malloc(sizeof(*packer_cxt));

    u32 sz = 0;
    u32 texture_page = 0;
    while ((sz = ARRAY_size(glyph_rects)) > 0) {
        stbrp_init_target(packer_cxt, texture_size, texture_size, nodes, num_nodes);
        stbrp_setup_heuristic(packer_cxt, STBRP_HEURISTIC_Skyline_BL_sortHeight);
        stbrp_pack_rects(packer_cxt, glyph_rects, sz);

        u32 i = sz;
        while (i > 0) {
            stbrp_rect r = glyph_rects[i - 1];
            if (r.was_packed) {
                struct BalGlyph glyph = local_glyphs[r.id];
                for (u32 x = 0; x < r.w; ++x) {
                    for (u32 y = 0; y < r.h; ++y) {
                        final_atlas[(r.x + x) + (r.y + y) * texture_size + (texture_page * texture_size * texture_size)] = local_glyph_data[(x + glyph.x) + (y + glyph.y) * texture_size + (glyph.layer * texture_size * texture_size)];
                    }
                }
                local_glyphs[r.id].x = r.x;
                local_glyphs[r.id].y = r.y;
                local_glyphs[r.id].layer = texture_page;

                glyph_rects[i - 1] = glyph_rects[sz - 1];
                sz--;
            }
            i--;
        }

        ARRAY_set_len(glyph_rects, sz);
        texture_page++;
    }

    *glyphs = local_glyphs;
    *glyph_data = final_atlas;
    *data_len = (texture_page == 0 ? 1 : texture_page) * (texture_size * texture_size);

    free(nodes);
    fclose(f);
}

struct BalFont *BAL_export_font(struct BalExporter *exporter, const char *path)
{
    struct BalGlyph *glyphs = 0;
    u8 *data = 0;
    u64 data_len = 0;

    BDF_parse_bdf(path, 1024, &glyphs, &data, &data_len);

    struct BalBuffer *buf = BAL_allocate_buffer(exporter, (u32)data_len);
    buf->size = (u32)data_len;

    struct BalFont *font = BAL_allocate_font(exporter, ARRAY_size(glyphs));
    font->texture_size = 1024;
    font->glyph_count = ARRAY_size(glyphs);
    BAL_SET_REF_PTR(font->buffer, buf);

    memcpy_s(buf->data, buf->size, data, data_len);
    memcpy_s(font->glyphs, font->glyph_count * sizeof(struct BalGlyph), glyphs, ARRAY_size(glyphs) * sizeof(struct BalGlyph));

    return font;
}
