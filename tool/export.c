#include "export.h"
#include "array.h"
#include "mem.h"
#include "time.h"
#include "toml.h"
#include "bdf.h"
#include "shared/error.h"

#include <stdio.h>
#include <windows.h>

enum FbErrorCode BAL_create_exporter(const char *path, struct BalExporter **exporter)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "r");
    if (!f || error) {
        return ERR_fmt(FB_ERR_FILE_NOT_FOUND, "Failed to open root conv file '%s': %d", path, error);
    }

    char errbuf[256];
    toml_table_t *config = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);

    if (!config) {
        return ERR_fmt(FB_ERR_PARSE_TOML, "Failed to parse root conv file '%s': %s", path, errbuf);
    }

    u8 *start = (char *)MEM_virtual_alloc(GiB(1));
    u8 *end = start;

    struct BalHeader *header = (struct BalHeader *)end;
    header->magic = BAL_MAGIC;
    BAL_SET_REF_PTR(header->descriptor_tables, NULL);
    end += sizeof(struct BalHeader);
    BAL_ALIGN(end);
    
    struct BalExporter e = (struct BalExporter) {
        .data_start = start,
        .data_end = end,
        .fonts = NULL,
        .toml = config
    };

    *exporter = malloc(sizeof(**exporter));
    **exporter = e;

    return FB_ERR_NONE;
}

char* readable_bytes(double size/*in bytes*/, char *buf) {
    int i = 0;
    const char* units[] = {"B", "kiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    sprintf(buf, "%.*f %s", i, size, units[i]);
    return buf;
}

void BAL_process_conv(struct BalExporter *exporter, const char *path)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "r");
    if (!f || error) {
        // TODO(fkaa): error
        return;
    }

    char errbuf[256];
    toml_table_t *config = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);

    if (!config) {
        // TODO(fkaa): error
        return;
    }

    toml_array_t *bdfs = toml_array_in(config, "bdf");
    if (bdfs) {
        u32 idx = 0;
        toml_table_t *bdf = 0;
        while ((bdf = toml_table_at(bdfs, idx++)) != 0) {
            char *file = 0;
            const char *source = 0;
            if ((source = toml_raw_in(bdf, "source")) != 0) {
                toml_rtos(source, &file);
                char *file_path = malloc(strlen(file) + strlen(path) + 1);
                *file_path = '\0';
                strcat(file_path, path);
                file_path[strlen(file_path) - sizeof("conv.toml") + 1] = '\0';
                strcat(file_path, file);
                free(file);

                s64 start = TIME_current();
                struct BalFont *font = BAL_export_font(exporter, file_path);
                s64 time = TIME_current() - start;

                ARRAY_push(exporter->fonts, font);
    
                char texturesize[32];
                printf("%s: %d glyphs, %s textures.. %.1fms\n",
                        file_path,
                        font->glyph_count,
                        readable_bytes((r64)((struct BalBuffer *)BAL_PTR(font->buffer))->size, texturesize),
                        TIME_ms(time));
            }
        }
    }
}

void BAL_walk_dirs(struct BalExporter *exporter)
{
    toml_table_t *pkg = toml_table_in(exporter->toml, "package");
    if (!pkg)
        return;
    toml_array_t *directories = toml_array_in(pkg, "directories");
    if (directories) {
        char **directory_paths = 0;
        const char *directory = 0;

        u32 idx = 0;
        while ((directory = toml_raw_at(directories, idx++)) != 0) {
            char *str = 0;
            toml_rtos(directory, &str);
            char *directory_str = malloc(strlen(str) + sizeof("/conv.toml") + 1);
            *directory_str = '\0';
            strcat(directory_str, str);
            strcat(directory_str, "/conv.toml");
            free(str);

            ARRAY_push(directory_paths, directory_str);
        }

        u32 dir_len = ARRAY_size(directory_paths);
        for (u32 i = 0; i < dir_len; ++i) {
            //printf("[%d/%d] %s\n", i + 1, dir_len, directory_paths[i]);
            printf(">%s\n", directory_paths[i]);
            BAL_process_conv(exporter, directory_paths[i]);
        }
    }
    // for directory in conv.toml["package"]["directories]
    // {
    //     process(directory + "/conv.toml");
    //
    //     recursive?
    // }
}


enum FbErrorCode BAL_exporter_write(struct BalExporter *exporter)
{
    toml_table_t *pkg = 0;
    if (!(pkg = toml_table_in(exporter->toml, "package"))) {
        return ERR_fmt(FB_ERR_PARSE_TOML, "Could not find [package] table in root conv file");
    }

    char output[256];
    const char *name = 0;
    if (!(name = toml_raw_in(pkg, "name"))) {
        snprintf(output, sizeof(output), "fish.bal", name);
    }
    else {
        char *str = 0;
        toml_rtos(name, &str);
        snprintf(output, sizeof(output),"%s.bal", str);

        free(str);
    }

    FILE *f = 0;
    errno_t error = fopen_s(&f, output, "wb+");
    if (!f || error) {
        return ERR_fmt(FB_ERR_FILE_CREATE, "Failed to create BAL output file '%s': %d", output, error);
    }

    u32 size = 0;
    size += ARRAY_size(exporter->fonts);
    struct BalDescriptorTable *table = BAL_allocate_descriptor_table(exporter, size);
    table->descriptor_count = size;

    u32 table_index = 0;
    for (u32 i = 0; i < ARRAY_size(exporter->fonts); ++i) {
        struct BalDescriptor *desc = &table->descriptors[table_index];
        desc->type = BAL_TYPE_FONT;
        struct BalFont *font = exporter->fonts[i];
        BAL_SET_REF_PTR(desc->ref, font);
        table_index++;
    }

    struct BalHeader *header = (struct BalHeader *)exporter->data_start;
    BAL_SET_REF_PTR(header->descriptor_tables, table);

    char bytesize[32];
    printf(">writing to '%s' (%s)\n", output, readable_bytes((r64)(exporter->data_end - exporter->data_start), bytesize));

    fwrite(exporter->data_start, exporter->data_end - exporter->data_start, 1, f);
    fclose(f);

    return FB_ERR_NONE;
}

struct BalDescriptorTable *BAL_allocate_descriptor_table(struct BalExporter *exporter, u32 size)
{
    return BAL_ALLOC_VARIABLE_SIZE_TYPE(exporter->data_end, struct BalDescriptorTable, descriptors, size);
}

struct BalFont *BAL_allocate_font(struct BalExporter *exporter, u32 size)
{
    return BAL_ALLOC_VARIABLE_SIZE_TYPE(exporter->data_end, struct BalFont, glyphs, size);
}

struct BalBuffer *BAL_allocate_buffer(struct BalExporter *exporter, u32 size)
{
    return BAL_ALLOC_VARIABLE_SIZE_TYPE(exporter->data_end, struct BalBuffer, data, size);
}
