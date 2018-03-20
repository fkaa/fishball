#include "shared/types.h"
#include "bal.h"

struct BalExporter {
    u8 *data_start;
    u8 *data_end;

    struct BalFont **fonts;

    const char *path;
    const char *output;
};

#define BAL_ALIGN(pointer) \
    ((char *)pointer = (char *)(((uintptr_t)pointer + 7) & ~7))
#define BAL_ALLOC(pointer, size) \
    ((char *)pointer += size, (char *)pointer - size)
#define BAL_ALLOC_VARIABLE_SIZE_TYPE(pointer, type, field, count) \
    (BAL_ALIGN(pointer), (type *)BAL_ALLOC(pointer, BAL_GET_SIZE(type, field, count)))

void BAL_create_exporter(const char *path, const char *output, struct BalExporter **exporter);
void BAL_exporter_write(struct BalExporter *exporter);
struct BalDescriptorTable *BAL_allocate_descriptor_table(struct BalExporter *exporter, u32 size);
struct BalFont *BAL_allocate_font(struct BalExporter *exporter, u32 size);
struct BalBuffer *BAL_allocate_buffer(struct BalExporter *exporter, u32 size);
