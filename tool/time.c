#include "time.h"
#include <windows.h>

s64 TIME_current()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

r64 TIME_s(s64 time)
{
    LARGE_INTEGER t;
    t.QuadPart = time;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    return (r64)t.QuadPart / (r64)freq.QuadPart;
}

r64 TIME_ms(s64 time)
{
    return TIME_s(time) * 1000.f;
}
