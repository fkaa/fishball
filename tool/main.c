#include <stdlib.h>
#include <stdio.h>

#include "array.h"
#include "export.h"

#include <windows.h>

int main()
{
    struct BalExporter *exporter = 0;
    BAL_create_exporter("conv.toml", &exporter);

    BAL_walk_dirs(exporter);
    //BAL_export_font(exporter, "unifont.bdf");

    BAL_exporter_write(exporter);
    return 0;
}

