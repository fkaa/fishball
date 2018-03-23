#include "mem.h"

#include <windows.h>

void *MEM_virtual_alloc(u64 size)
{
    return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
}
