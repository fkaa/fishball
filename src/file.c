#include "file.h"

#include "shared/error.h"

#include <stdlib.h>
#include <stdio.h>

enum FbErrorCode FILE_read_whole(const char *path, char **string, u32 *len)
{
    FILE *f = 0;
    errno_t error = fopen_s(&f, path, "rb");
    if (!f || error != 0)
        return FB_ERR_FILE_NOT_FOUND;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *s = malloc(fsize + 1);
    fread(s, fsize, 1, f);
    fclose(f);

    s[fsize] = 0;

    *len = fsize;
    *string = s;

    return FB_ERR_NONE;
}
