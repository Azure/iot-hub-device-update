#include "aducpal/sys_sysinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int ADUCPAL_sysinfo(struct sysinfo* info)
{
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
    {
        return -1;
    }

    const unsigned int BYTES_IN_KB = 1024;

    info->totalram = (unsigned long)(ms.ullTotalPhys / BYTES_IN_KB);
    info->mem_unit = BYTES_IN_KB;
    return 0;
}
