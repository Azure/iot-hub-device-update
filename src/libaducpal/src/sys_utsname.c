#include "aducpal/sys_utsname.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <malloc.h>
#include <stdbool.h>

static bool g_utsname_initialized = false;
static char g_utsname_sysname[128] = { '\0' };
static char g_utsname_release[128] = { '\0' };
static char g_utsname_machine[128] = { '\0' };
static struct utsname g_utsname;

bool RegQueryStringValue(HKEY hkey, const char* subkey, char* buffer, DWORD buffer_size)
{
    DWORD type;
    if (RegQueryValueEx(hkey, subkey, NULL, &type, (BYTE*)buffer, &buffer_size) != 0)
    {
        return false;
    }

    if (type != REG_SZ)
    {
        return false;
    }

    return true;
}

int ADUCPAL_uname(struct utsname* buf)
{
    if (!g_utsname_initialized)
    {
        g_utsname.sysname = g_utsname_sysname;
        g_utsname.release = g_utsname_release;
        g_utsname.machine = g_utsname_machine;

        HKEY hk;
        if (RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hk) != ERROR_SUCCESS)
        {
            return -1;
        }

        const bool success =
            (RegQueryStringValue(hk, "ProductName", g_utsname_sysname, sizeof(g_utsname_sysname))
             && RegQueryStringValue(hk, "CurrentVersion", g_utsname_release, sizeof(g_utsname_release))
             && RegQueryStringValue(hk, "CurrentType", g_utsname_machine, sizeof(g_utsname_machine)));

        RegCloseKey(hk);

        if (!success)
        {
            return -1;
        }

        g_utsname_initialized = true;
    }

    *buf = g_utsname;
    return 0;
}
