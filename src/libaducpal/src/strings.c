#include "aducpal/strings.h"

#ifdef ADUCPAL_USE_PAL

#    include <string.h> // _stricmp

int ADUCPAL_strcasecmp(const char* s1, const char* s2)
{
    return _stricmp(s1, s2);
}

#endif // #ifdef ADUCPAL_USE_PAL
