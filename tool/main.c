#include <stdlib.h>
#include <stdio.h>

#include "bal.h"

#include <windows.h>

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

    printf("hello\n");
    CONV_write_bal("fish.bal", output_buffer, output);
    return 0;
}

