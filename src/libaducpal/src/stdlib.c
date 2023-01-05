#include "aducpal/stdlib.h"

#ifdef ADUCPAL_USE_PAL

// clang-format off
#include <windows.h>
#include <corecrt_io.h> //_mktemp_s
#include <fcntl.h> // _O_*
#include <stdlib.h> // putenv
// clang-format on

#    include "aducpal/unistd.h" // open

#    include <stdlib.h> // getenv
#    include <string.h>

int ADUCPAL_mkstemp(char* tmpl)
{
    const errno_t ret = _mktemp_s(tmpl, strlen(tmpl) + 1);
    if (ret != 0)
    {
        errno = ret;
        return -1;
    }

    // TODO(JefFMill): [PAL] Pass , S_IRUSR | S_IWUSR.  _O_TEMPORARY requires admin access?
    return ADUCPAL_open(tmpl, _O_RDWR | _O_CREAT | _O_EXCL);
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

    return (_putenv_s(name, value) == 0) ? 1 : 0;
}

// Returns 1 on success.
int ADUCPAL_unsetenv(const char* name)
{
    return (_putenv(name)) ? 1 : 0;
}

#endif // #ifdef ADUCPAL_USE_PAL
