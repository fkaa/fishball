#include "helper.h"
#include "file.h"

#include "shared/error.h"

#include <stdio.h>
#include <windows.h>

void HELPER_open_browser_url(const char *url)
{
    ShellExecuteA(0, 0, url, 0, 0, SW_SHOW);
}

u64 HELPER_get_hash(char *contents, u64 len)
{
    u64 hash = 0x3ad3203b8;

    for (u32 i = 0; i < len; ++i) {
        hash = ((hash << 5) + hash) + contents[i];
    }

    return hash;
}

void HELPER_write_file_hash(const char *path)
{
    char *contents = 0;
    u32 len = 0;
    if (FB_ERR_NONE == FILE_read_whole(path, &contents, &len)) {
        u64 hash = HELPER_get_hash(contents, len);
        free(contents);

        char hash_path[128];
        snprintf(hash_path, 128, "%s.hash", path);
        FILE *f = 0;
        errno_t error = fopen_s(&f, hash_path, "wb+");
        if (!f || error != 0)
            return;

        fwrite(&hash, sizeof(u64), 1, f);
        fclose(f);
    }
}

bool HELPER_check_file_hash(const char *path)
{
    char *contents = 0;
    u32 len = 0;
    if (FB_ERR_NONE == FILE_read_whole(path, &contents, &len)) {
        u64 file_hash = HELPER_get_hash(contents, len);
        free(contents);

        char hash_path[128];
        snprintf(hash_path, 128, "%s.hash", path);
        u64 *file_hash_ptr = 0;
        u32 file_hash_len = 0;

        if (FB_ERR_NONE == FILE_read_whole(hash_path, &file_hash_ptr, &file_hash_len)) {
            bool eq = false;
            if (file_hash_len == sizeof(u64)) {
                eq = file_hash == *file_hash_ptr;
            }

            free(file_hash_ptr);

            return eq;
        }

    }

    return false;
}
