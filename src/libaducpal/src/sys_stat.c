#include "aducpal/sys_stat.h"

#include <direct.h> // _mkdir
#include <errno.h> // errno

int ADUCPAL_chmod(const char* path, mode_t mode)
{
    // TODO(JeffMill): [PAL]
    // Pretty limited what we can do here, as Windows only really supports read-only on a file for owner (S_IRUSR).
    // Maybe set/reset read-only attribute on "path"?
    return 0;
}

int ADUCPAL_fchmod(int fd, mode_t mode)
{
    // TODO(JeffMill): [PAL]
    // Pretty limited what we can do here, as Windows only really supports read-only on a file for owner (S_IRUSR).
    // Furthermore, Windows doesn't have an implementation of fchmod, and no way to get path from fd for other
    // chmod APIs.
    return 0;
}

int ADUCPAL_mkdir(const char* path, mode_t mode)
{
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
