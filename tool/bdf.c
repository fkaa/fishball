#include "bdf.h"
#include "array.h"
#include "time.h"

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

void BDF_parse_bdf(const char *path, struct BalGlyph **glyphs, u8 **glyph_data)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "rb");
    if (error || !f) return;

    struct BalGlyph *local_glyphs = 0;
    u8 *local_glyph_data = 0;

    struct BalGlyph glyph;
    u8 *bitmap = 0;
    char *line = 0;
    int i = 0;
    u32 data_offset = 0;
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
            // push glyph
            glyph.data_offset = data_offset;

            ARRAY_push(local_glyphs, glyph);
            ARRAY_append(local_glyph_data, bitmap, ARRAY_size(bitmap));

            parse_bitmap = false;
            data_offset += ARRAY_size(bitmap);

            ARRAY_reset(bitmap);
        }
        else if (parse_bitmap) {
            u16 row = (u16)strtol(line, NULL, 16);
            for (u32 i = 0; i < 16; ++i) {
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

    fclose(f);
}

void BAL_export_font(struct BalExporter *exporter, const char *path)
{
    struct BalGlyph *glyphs = 0;
    u8 *data = 0;

    s64 start = TIME_current();
    BDF_parse_bdf(path, &glyphs, &data);
    s64 time = TIME_current() - start;

    struct BalBuffer *buf = BAL_allocate_buffer(exporter, ARRAY_size(data));
    buf->size = ARRAY_size(data);

    struct BalFont *font = BAL_allocate_font(exporter, ARRAY_size(glyphs));
    font->glyph_count = ARRAY_size(glyphs);
    BAL_SET_REF_PTR(font->buffer, buf);

    printf("BAL/BDF: %d glyphs, %d bytes bitmap.. %.1fms\n", ARRAY_size(glyphs), ARRAY_size(data), TIME_ms(time));

    memcpy_s(buf->data, buf->size, data, ARRAY_size(data));
    memcpy_s(font->glyphs, font->glyph_count, glyphs, ARRAY_size(glyphs));

    ARRAY_push(exporter->fonts, font);
}
