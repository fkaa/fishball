#include <stdlib.h>

struct FbArrayHeader {
    unsigned int size;
    unsigned int capacity;
};

#define FB_ARRAY_HEADER(array) \
    ((struct FbArrayHeader *)((char *)(array) - sizeof(struct FbArrayHeader)))

#define ARRAY_size(array) ((array) ? FB_ARRAY_HEADER(array)->size : 0)
#define ARRAY_capacity(array) ((array) ? FB_ARRAY_HEADER(array)->capacity : 0)
#define ARRAY_full(array) (ARRAY_size(array) >= ARRAY_capacity(array))
#define ARRAY_reset(array) ((array) ? FB_ARRAY_HEADER(array)->size = 0 : 0)
#define ARRAY_append(array, items, n) \
    ARRAY_size(array) + (n) >= ARRAY_capacity(array) ? array = ARRAY_grow(array, sizeof(*array), (n)) : 0; \
    for (unsigned int i = 0, sz = FB_ARRAY_HEADER(array)->size; i < (n); ++i) array[sz + i] = (items)[i]; FB_ARRAY_HEADER(array)->size += (n) 
    
#define ARRAY_push(array, item) \
    ARRAY_full(array) ? array = ARRAY_grow(array, sizeof(*array), 1) : 0, \
    array[FB_ARRAY_HEADER(array)->size++] = item

#define ARRAY_set_len(array, len) ((array) ? FB_ARRAY_HEADER(array)->size = (len) : 0)

#include <stdio.h>
// TODO(fkaa): move into .c
static void *ARRAY_grow(void *arr, int size, int n)
{
    int twice_capacity = arr ? 2 * ARRAY_capacity(arr) : 0;
    int min = ARRAY_size(arr) + n;
    int new_size = twice_capacity > min ? twice_capacity : min;
    struct FbArrayHeader *header = realloc(arr ? FB_ARRAY_HEADER(arr) : 0, size * new_size + sizeof(struct FbArrayHeader));
    header->capacity = new_size;
    if (!arr)
        header->size = 0;

    return (void *)((char*)header + sizeof(struct FbArrayHeader));
}
