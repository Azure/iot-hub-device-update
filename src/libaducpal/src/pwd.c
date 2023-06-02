#include "aducpal/pwd.h"

#include <string.h>

struct passwd* ADUCPAL_getpwnam(const char* name)
{
    // From CMakeLists.txt
#define ADUC_FILE_USER "adu"
#define DO_FILE_USER "do"

    // For Windows, return success on "do" and "adu".
    static struct passwd g_p = { 0 /* pw_uid */ };

    if (strcmp(name, ADUC_FILE_USER) == 0 || strcmp(name, DO_FILE_USER) == 0)
    {
        return &g_p;
    }

    return NULL;
}
