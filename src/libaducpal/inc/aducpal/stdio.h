#ifndef ADUCPAL_STDIO_H
#define ADUCPAL_STDIO_H

#ifdef ADUCPAL_USE_PAL

#    include <stdio.h> // FILE*

#    ifdef __cplusplus
extern "C"
{
#    endif
    FILE* ADUCPAL_popen(const char* command, const char* type);
    int ADUCPAL_pclose(FILE* stream);

#    ifdef __cplusplus
}
#    endif

#else

#    include <stdio.h>

#    define ADUCPAL_popen(command, type) popen(command, type)
#    define ADUCPAL_pclose(stream) pclose(stream)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_STDIO_H
