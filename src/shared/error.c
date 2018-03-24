#include <stdarg.h>
#include <stdio.h>

char ERR_buffer[256];

enum FbErrorCode ERR_fmt_(enum FbErrorCode err, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(ERR_buffer, sizeof(ERR_buffer), fmt, args);
    va_end(args);

    fprintf(stderr, "error: %s\n", ERR_buffer);

    return err;
}
