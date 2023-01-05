#include "aducpal/sys_stat.h"

#ifdef ADUCPAL_USE_PAL

#    include <direct.h> // _mkdir
#    include <errno.h> // errno
#    include <windows.h> // IsDebuggerPresent

int ADUCPAL_chmod(const char* path, mode_t mode)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_fchmod(int fd, mode_t mode)
{
    // TODO(JeffMill): [PAL] This is only used in a UT, and doesn't have any analog on Windows, so no-op.
    return 0;
}

int ADUCPAL_mkdir(const char* path, mode_t mode)
{
    const int ret = _mkdir(path);
    if (ret == -1)
    {
        // Folder existing already isn't fatal.
        if (errno != EEXIST)
        {
            if (IsDebuggerPresent())
            {
                __debugbreak();
            }
        }
    }
    return ret;
}

int ADUCPAL_mkfifo(const char* pathname, mode_t mode)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
