#include "bdf.h"
#include "array.h"
#include "time.h"
#include "mem.h"

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
    u8 *local_glyph_data = MEM_virtual_alloc(MiB(100));

    struct BalGlyph glyph;
    u8 *bitmap = 0;
    char *line = 0;
    bool parse_bitmap = false;
    u16 codepoint = 0;
    u8 width = 0, height = 0;

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

            ARRAY_push(local_glyphs, glyph);
            
            for (u32 x = 0; x < glyph.width; ++x) {
                for (u32 y = 0; y < glyph.height; ++y) {
                    u32 write_x = x + cursor_x;
                    u32 write_y = y + cursor_y;
                    local_glyph_data[write_x + write_y * texture_size + (cursor_z * texture_size * texture_size)] = bitmap[x + y * glyph.width];
                }
            }
            cursor_x += glyph.width;

            parse_bitmap = false;

            ARRAY_reset(bitmap);
        }
        else if (parse_bitmap) {
            u32 bits = strlen(line) * 4;
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

    *glyphs = local_glyphs;
    *glyph_data = local_glyph_data;
    *data_len = (cursor_z == 0 ? 1 : cursor_z) * (texture_size * texture_size);

    fclose(f);
}

void BAL_export_font(struct BalExporter *exporter, const char *path)
{
    struct BalGlyph *glyphs = 0;
    u8 *data = 0;
    u64 data_len = 0;

    s64 start = TIME_current();
    BDF_parse_bdf(path, 1024, &glyphs, &data, &data_len);
    s64 time = TIME_current() - start;

    struct BalBuffer *buf = BAL_allocate_buffer(exporter, data_len);
    buf->size = data_len;

    struct BalFont *font = BAL_allocate_font(exporter, ARRAY_size(glyphs));
    font->texture_size = 1024;
    font->glyph_count = ARRAY_size(glyphs);
    BAL_SET_REF_PTR(font->buffer, buf);

    printf("BAL/BDF: %d glyphs, %d bytes bitmap.. %.1fms\n", ARRAY_size(glyphs), data_len, TIME_ms(time));

    memcpy_s(buf->data, buf->size, data, data_len);
    memcpy_s(font->glyphs, font->glyph_count * sizeof(struct BalGlyph), glyphs, ARRAY_size(glyphs) * sizeof(struct BalGlyph));

    ARRAY_push(exporter->fonts, font);
}
