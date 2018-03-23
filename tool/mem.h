#include "shared/types.h"

#define KiB(n) ((n) * 1024)
#define MiB(n) (KiB(n) * 1024)
#define GiB(n) (MiB(n) * 1024)

void *MEM_virtual_alloc(u64 size);
