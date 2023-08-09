#include "aducpal/strings.h"

#include <string.h> // _stricmp

int ADUCPAL_strcasecmp(const char* s1, const char* s2)
{
    return _stricmp(s1, s2);
}
