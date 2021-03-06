#include "shared/types.h"
#include "bal.h"

typedef struct toml_table_t toml_table_t;

struct BalExporter {
    u8 *data_start;
    u8 *data_end;

    toml_table_t *toml;

    struct BalFont **fonts;
    struct BalSpirv **shaders;
};

#define BAL_ALIGN(pointer) \
    ((char *)pointer = (char *)(((uintptr_t)pointer + 7) & ~7))
#define BAL_ALLOC(pointer, size) \
    ((char *)pointer += (size), (char *)pointer - (size))
#define BAL_ALLOC_VARIABLE_SIZE_TYPE(pointer, type, field, count) \
    (BAL_ALIGN(pointer), (type *)BAL_ALLOC(pointer, BAL_GET_SIZE(type, field, count)))

enum FbErrorCode;

enum FbErrorCode BAL_create_exporter(const char *path, struct BalExporter **exporter);
enum FbErrorCode BAL_walk_dirs(struct BalExporter *exporter);
enum FbErrorCode BAL_exporter_write(struct BalExporter *exporter);
struct BalDescriptorTable *BAL_allocate_descriptor_table(struct BalExporter *exporter, u32 size);
struct BalFont *BAL_allocate_font(struct BalExporter *exporter, u32 size);
struct BalBuffer *BAL_allocate_buffer(struct BalExporter *exporter, u32 size);

struct BalSpirv *BAL_allocate_spirv(struct BalExporter *exporter, const char *name, u8 *data, u32 size);
