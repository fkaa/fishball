#include <stdlib.h>
#include <stdio.h>

#include "array.h"
#include "export.h"

#include <windows.h>

struct BalFont *CONV_fonts = NULL;

struct BalDescriptorTable *CONV_create_descriptor_table()
{

    return 0;
}

void CONV_write_bal(const char *path, char *start, char *end)
{

    FILE *f = fopen(path, "wb+");
    if (!f) return;

    fwrite(start, end - start, 1, f);
    fclose(f);
}

int main()
{
    char *output_buffer = (char *)VirtualAlloc(0, 1024 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
    char *output = output_buffer;

    struct BalHeader *header = (struct BalHeader *)output;
    header->magic = BAL_MAGIC;
    BAL_SET_REF_PTR(header->descriptor_tables, NULL);

    struct BalDescriptorTable *descriptor_table = CONV_create_descriptor_table();
    BAL_SET_REF_PTR(header->descriptor_tables, descriptor_table);

    printf("hello\n");
    CONV_write_bal("fish.bal", output_buffer, output);
    return 0;
}

