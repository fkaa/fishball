#include "export.h"
#include "shared/error.h"

#include <stdio.h>
#include <stdlib.h>

enum FbErrorCode run()
{
    struct BalExporter *exporter = 0;
    enum FbErrorCode err = FB_ERR_NONE;
    
    if ((err = BAL_create_exporter("conv.toml", &exporter)) != FB_ERR_NONE)
        return err;

    BAL_walk_dirs(exporter);
    //BAL_export_font(exporter, "unifont.bdf");

    if ((err = BAL_exporter_write(exporter)) != FB_ERR_NONE)
        return err;

    return FB_ERR_NONE;
}

int main(int argc, char* argv[])
{
    if (run() != FB_ERR_NONE) {
        //fprintf(stderr, "%s\n", ERROR_buffer);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
