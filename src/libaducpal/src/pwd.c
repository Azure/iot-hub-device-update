#include "aducpal/pwd.h"

#ifdef ADUCPAL_USE_PAL

#    include <string.h>

struct passwd* ADUCPAL_getpwnam(const char* name)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, fake "do" and "adu".

    // From CMakeLists.txt
#    define ADUC_FILE_USER "adu"
#    define DO_FILE_USER "do"

    static struct passwd g_p = { 0 /* pw_uid */ };

    if (strcmp(name, ADUC_FILE_USER) == 0 || strcmp(name, DO_FILE_USER) == 0)
    {
        return &g_p;
    }

    return NULL;
}

#endif // #ifdef ADUCPAL_USE_PAL
