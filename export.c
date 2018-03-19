#include "export.h"

#include <windows.h>

void BAL_create_exporter(const char *path, const char *output, struct BalExporter **exporter)
{
    u8 *start = (char *)VirtualAlloc(0, 1024 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
    u8 *end = output_buffer;

    struct BalHeader *header = (struct BalHeader *)output;
    header->magic = BAL_MAGIC;
    BAL_SET_REF_PTR(header->descriptor_tables, NULL);
    output += sizeof(struct BalHeader);
    BAL_ALIGN(output);

    *exporter = malloc(sizeof(**exporter));
    **exporter = (struct BalExporter) {
        .data_start = start,
        .data_end = end,
        .path = path,
        .output = output
    };
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
