#include "aducpal/grp.h"

#include <string.h>

struct group* ADUCPAL_getgrnam(const char* name)
{
// TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return 0 (root) for "adu" and "do"

// From CMakeLists.txt
#define ADUC_FILE_GROUP "adu"
#define DO_FILE_GROUP "do"

    static char* s_gr_mem[] = { ADUC_FILE_GROUP, DO_FILE_GROUP, NULL };

    static struct group s_g = { 0 /* gr_gid */, s_gr_mem };

    if (strcmp(name, ADUC_FILE_GROUP) == 0 || strcmp(name, DO_FILE_GROUP) == 0)
    {
        return &s_g;
    }

    return NULL;
}
