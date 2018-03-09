#include "file.h"

#include "shared/error.h"

#include <stdlib.h>
#include <stdio.h>

enum FbErrorCode FILE_read_whole(const char *path, char **string)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "rb");
    if (!f || error != 0)
        return FB_ERR_FILE_NOT_FOUND;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return FB_ERR_NONE;
}
