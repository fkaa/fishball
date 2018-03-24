#include <stdarg.h>
#include <stdio.h>

char ERROR_buffer[256];

enum FbErrorCode ERROR_fmt_(enum FbErrorCode err, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(ERROR_buffer, sizeof(ERROR_buffer), fmt, args);
    va_end(args);

    fprintf(stderr, "%s\n", ERROR_buffer);

    return err;
}
