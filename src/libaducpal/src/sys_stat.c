#include "aducpal/sys_stat.h"

#include <direct.h> // _mkdir
#include <errno.h> // errno

#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif

int ADUCPAL_chmod(const char* path, mode_t mode)
{
    // For Windows, do nothing with mode for now.
    UNREFERENCED_PARAMETER(path);
    UNREFERENCED_PARAMETER(mode);
    return 0;
}

int ADUCPAL_mkdir(const char* path, mode_t mode)
{
    // For Windows, do nothing with mode for now.
    UNREFERENCED_PARAMETER(mode);

    const int ret = _mkdir(path);
    if (ret == -1)
    {
        // Folder existing already isn't fatal.
        if (errno == EEXIST)
        {
            return 0;
        }
    }
    return ret;
}
