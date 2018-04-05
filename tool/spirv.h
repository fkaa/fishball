#ifndef BAL_SPIRV_H
#define BAL_SPIRV_H

#include "shared/types.h"

struct BalSpirv;
typedef struct toml_table_t toml_table_t;

struct BalSpirv *BAL_compile_glsl(struct BalExporter *exporter, const char *name, const char *outfile, const char *path, toml_table_t *options);

#endif /* BAL_SPIRV_H */


