#ifndef BAL_BDF_H
#define BAL_BDF_H

#include "shared/types.h"

struct BalFont;
struct BalExporter;

struct BalFont *BAL_export_font(struct BalExporter *exporter, const char *path);

#endif /* BAL_BDF_H */