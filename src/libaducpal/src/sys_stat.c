#include "aducpal/sys_stat.h"

#ifdef ADUCPAL_USE_PAL

#    include <direct.h> // _mkdir
#    include <errno.h> // errno
#    include <windows.h> // IsDebuggerPresent

// TODO(JeffMill): Implement
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

#endif // #ifdef ADUCPAL_USE_PAL
