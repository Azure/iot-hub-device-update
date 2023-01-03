#include "aducpal/stdlib.h"

#ifdef ADUCPAL_USE_PAL

#    include <stdlib.h> // getenv
#    include <string.h>

int ADUCPAL_mkstemp(char* tmpl)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_setenv(const char* name, const char* value, int overwrite)
{
    char string0[1024];
    char* string;

    if (getenv(name) != NULL && overwrite == 0)
    {
        return 1;
    }
    strcpy(string0, name);
    strcat(string0, "=");
    strcat(string0, value);
    string = strdup(string0);
    return (putenv(string)) ? 1 : 0;
}

int ADUCPAL_unsetenv(const char* name)
{
    return (putenv(name)) ? 1 : 0;
}

#endif // #ifdef ADUCPAL_USE_PAL
