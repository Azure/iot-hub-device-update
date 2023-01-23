#include "aducpal/stdlib.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <corecrt_io.h> //_mktemp_s
#include <fcntl.h> // _O_*
#include <stdlib.h> // getenv, putenv
#include <string.h>

#include "aducpal/unistd.h" // open

char* ADUCPAL_mktemp(char* tmpl)
{
    const errno_t ret = _mktemp_s(tmpl, strlen(tmpl) + 1);
    // The mktemp() function always returns template.
    // If a unique name could not be created,
    // template is made an empty string, and errno is set to indicate the error.
    _set_errno(ret);
    if (ret != 0)
    {
        tmpl[0] = '\0';
    }
    return tmpl;
}

// Returns 1 on success.
int ADUCPAL_setenv(const char* name, const char* value, int overwrite)
{
    errno_t ret;
    char* buffer;

    ret = _dupenv_s(&buffer, NULL, name);
    if (ret == 0)
    {
        // If the variable isn't found, then buffer is set to NULL, numberOfElements is set to 0,
        // and the return value is 0 because this situation isn't considered to be an error condition.
        if (buffer != NULL)
        {
            free(buffer);

            if (!overwrite)
            {
                // Variable exists, but don't overwrite it.
                return 1;
            }
        }
    }

    // On Windows, _putenv_s fails if value is "", so assume a unsetenv is desired in that case.
    if (*value == '\0')
    {
        return ADUCPAL_unsetenv(name);
    }

    return (_putenv_s(name, value) == 0) ? 1 : 0;
}

// Returns 1 on success.
int ADUCPAL_unsetenv(const char* name)
{
    return (_putenv(name)) ? 1 : 0;
}
