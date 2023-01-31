#include "aducpal/dlfcn.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif

static char s_dlfcn_errorbuf[256] = { 0 };
static DWORD s_dlfcn_lasterror = 0;

void* ADUCPAL_dlopen(const char* filename, int flag)
{
    UNREFERENCED_PARAMETER(flag);

    s_dlfcn_lasterror = 0;

    HANDLE hlib = LoadLibrary(filename);
    if (hlib == NULL)
    {
        s_dlfcn_lasterror = GetLastError();
    }

    return hlib;
}

char* ADUCPAL_dlerror()
{
    s_dlfcn_errorbuf[0] = '\0';

    if (s_dlfcn_lasterror != 0)
    {
        const DWORD ret = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            s_dlfcn_lasterror,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            s_dlfcn_errorbuf,
            sizeof(s_dlfcn_errorbuf) / sizeof(s_dlfcn_errorbuf[0]),
            NULL);
        if (ret != 0)
        {
            // Number of chars excluding the terminating null character.
            char* curr = s_dlfcn_errorbuf + ret - 1;
            while (*curr == '\n' || *curr == '\r')
            {
                *curr = '\0';
                --curr;
            }
        }
        else
        {
            // FormatMessage failed.
            s_dlfcn_errorbuf[0] = '\0';
        }
    }

    return s_dlfcn_errorbuf;
}

void* ADUCPAL_dlsym(void* handle, const char* symbol)
{
    s_dlfcn_lasterror = 0;

    FARPROC proc = GetProcAddress(handle, symbol);
    if (proc == NULL)
    {
        s_dlfcn_lasterror = GetLastError();
    }

    return (void*)proc;
}

int ADUCPAL_dlclose(void* handle)
{
    s_dlfcn_lasterror = 0;

    if (!FreeLibrary(handle))
    {
        s_dlfcn_lasterror = GetLastError();
    }

    // Returns 0 on success.
    return (s_dlfcn_lasterror == 0) ? 0 : 1;
}
