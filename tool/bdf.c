#include "bdf.h"
#include "array.h"

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
        }
        else if (strstr(line, "BITMAP") != NULL) {
            parse_bitmap = true;
        }
        else if (strcmp(line, "ENDCHAR") == 0) {
            // push glyph
            glyph.codepoint = codepoint;
            glyph.width = width;
            glyph.height = height;
            glyph.data_offset = data_offset;

            ARRAY_push(local_glyphs, glyph);
            ARRAY_append(local_glyph_data, bitmap, ARRAY_size(bitmap));
            
            parse_bitmap = false;
            data_offset += ARRAY_size(bitmap);
            ARRAY_reset(bitmap);
        }
        else if (parse_bitmap) {
            s32 row = (s32)strtol(line, NULL, 16);
            u8 *bytes = (u8*)&row;
            ARRAY_append(bitmap, bytes, 4);
            continue;
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

    BDF_parse_bdf(path, &glyphs, &data);

    struct BalBuffer *buf = BAL_allocate_buffer(exporter, ARRAY_size(data));
    buf->size = ARRAY_size(data);

    struct BalFont *font = BAL_allocate_font(exporter, ARRAY_size(glyphs));
    font->glyph_count = ARRAY_size(glyphs);
    BAL_SET_REF_PTR(font->buffer, buf);

    memcpy_s(buf->data, buf->size, data, ARRAY_size(data));
    memcpy_s(font->glyphs, font->glyph_count, glyphs, ARRAY_size(glyphs));
}
