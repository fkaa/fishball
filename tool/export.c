#include "export.h"

#include <stdio.h>
#include <windows.h>

void BAL_create_exporter(const char *path, const char *output, struct BalExporter **exporter)
{
    u8 *start = (char *)VirtualAlloc(0, 1024 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
    u8 *end = start;

    struct BalHeader *header = (struct BalHeader *)end;
    header->magic = BAL_MAGIC;
    BAL_SET_REF_PTR(header->descriptor_tables, NULL);
    end += sizeof(struct BalHeader);
    BAL_ALIGN(end);

    *exporter = malloc(sizeof(**exporter));
    **exporter = (struct BalExporter) {
        .data_start = start,
        .data_end = end,
        .path = path,
        .output = output
    };
}

void BAL_exporter_write(struct BalExporter *exporter)
{
    FILE *f = fopen(exporter->output, "wb+");
    if (!f) return;

    fwrite(exporter->data_start, exporter->data_end - exporter->data_start, 1, f);
    fclose(f);
}

struct BalBuffer *BAL_allocate_descriptor_table(struct BalExporter *exporter, u32 size)
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
