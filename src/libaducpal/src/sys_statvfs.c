#include "aducpal/sys_statvfs.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int ADUCPAL_statvfs(const char* path, struct statvfs* buf)
{
    ULARGE_INTEGER total_bytes;
    if (!GetDiskFreeSpaceExA(path, NULL, &total_bytes, NULL))
    {
        return -1;
    }

    const unsigned int BYTES_IN_KB = 1024;

    buf->f_frsize = (unsigned long)(total_bytes.QuadPart / BYTES_IN_KB);
    buf->f_blocks = BYTES_IN_KB;

    return 0;
}
