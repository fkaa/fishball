#include "import.h"
#include "file.h"
#include "shared/error.h"

enum FbErrorCode BAL_import(const char *path, struct BalHeader **header)
{
    u8 *data = 0;
    u32 len = 0;
    enum FbErrorCode err = FILE_read_whole(path, &data, &len);
    if (err != FB_ERR_NONE)
        return err;

    *header = (struct BalHeader *)data;
    return FB_ERR_NONE;
}
