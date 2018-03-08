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
#define ARRAY_push(array, item) \
    ARRAY_full(array) ? array = ARRAY_grow(array, sizeof(*array)) : 0, \
    array[FB_ARRAY_HEADER(array)->size++] = item

#include <stdio.h>
// TODO(fkaa): move into .c
static void *ARRAY_grow(void *arr, int size)
{
    int twice_capacity = arr ? 2 * ARRAY_capacity(arr) : 0;
    int min = ARRAY_size(arr) + 1;
    int new_size = twice_capacity > min ? twice_capacity : min;
    struct FbArrayHeader *header = realloc(arr ? FB_ARRAY_HEADER(arr) : 0, size * new_size + sizeof(struct FbArrayHeader));
    header->capacity = new_size;
    if (!arr)
        header->size = 0;

    return (void *)((char*)header + sizeof(struct FbArrayHeader));
}
