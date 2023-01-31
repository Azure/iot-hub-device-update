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
    int ADUCPAL_rename(const char* old_f, const char* new_f);

#    ifdef __cplusplus
}
#    endif

#else

#    include <stdio.h>

#    define ADUCPAL_popen popen
#    define ADUCPAL_pclose pclose
#    define ADUCPAL_rename rename

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_STDIO_H
