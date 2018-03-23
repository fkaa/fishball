#include <windows.h>

void HELPER_open_browser_url(const char *url)
{
    ShellExecuteA(0, 0, url, 0, 0, SW_SHOW);
}
