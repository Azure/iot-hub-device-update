#include "aducpal/stdio.h"

#include <ctype.h> // toupper
#include <direct.h> // _rmdir
#include <io.h> // _unlink
#include <stdbool.h>
#include <stdio.h> // popen, pclose
#include <string.h>

#include <errno.h> // ENOTDIR, _get_errno

static const char* get_executable_extension_pointer(const char* command)
{
    if (command == NULL)
    {
        return NULL;
    }

    const char* argStart;
    const char* argEnd;

    if (*command == '"')
    {
        argStart = command + 1;
        argEnd = strchr(argStart, '"');
    }
    else
    {
        argStart = command;
        argEnd = command + 1;
        while (*argEnd != ' ' && *argEnd != '\0')
        {
            ++argEnd;
        }
        if (*argEnd == '\0')
        {
            --argEnd;
        }
    }

    if (argEnd != NULL)
    {
        // Extension is the last '.'
        const char* dot;
        for (dot = argEnd; dot >= argStart; --dot)
        {
            if (*dot == '/' || *dot == '\\')
            {
                // Don't go past a path separator.
                break;
            }

            if (*dot == '.')
            {
                // Found the dot.
                return dot;
            }
        }
    }

    return NULL;
}

static bool is_extension(const char* ext, char c1, char c2, char c3)
{
    if (ext == NULL)
    {
        return false;
    }
    return (
        (ext[0] == '.') && (toupper(ext[1]) == toupper(c1)) && (toupper(ext[2]) == toupper(c2))
        && (toupper(ext[3]) == toupper(c3)) && (ext[4] != ' ' || ext[4] != '"' || ext[4] != '\0'));
}

FILE* ADUCPAL_popen(const char* command, const char* type)
{
    char launch_command[2048];
    const char* ext = get_executable_extension_pointer(command);
    if (ext == NULL)
    {
        // Can't find an extension.
        _set_errno(EACCES);
        return NULL;
    }

    if (is_extension(ext, 'P', 'S', '1'))
    {
        // Need to prepend "powershell.exe"
        if (strcpy_s(launch_command, sizeof(launch_command), "powershell.exe -NonInteractive -NoProfile -NoLogo ")
            != 0)
        {
            _set_errno(E2BIG);
            return NULL;
        }

        if (strcat_s(launch_command, sizeof(launch_command), command) != 0)
        {
            _set_errno(E2BIG);
            return NULL;
        }

        command = launch_command;
    }
    else if (!is_extension(ext, 'E', 'X', 'E'))
    {
        _set_errno(EACCES);
        return NULL;
    }

    return _popen(command, type);
}

int ADUCPAL_pclose(FILE* stream)
{
    return _pclose(stream);
}

int ADUCPAL_remove(const char* pathname)
{
    int ret = 0;

    // Unlike Windows implementation, remove() calls unlink(2) for files, and rmdir(2) for directories.
    // However, remove() generally is used to remove files and rmdir() is used to remove directories.

    if (_rmdir(pathname) == 0)
    {
        ret = 0;
        goto done;
    }

    errno_t errno;
    _get_errno(&errno);
    if (errno != ENOTDIR)
    {
        // Failed deleting directory
        ret = -1;
        goto done;
    }

    // Assume it's a file.

    if (_unlink(pathname) == 0)
    {
        ret = 0;
        goto done;
    }

    // Failed
    ret = -1;

done:
    return ret;
}

int ADUCPAL_rename(const char* old_f, const char* new_f)
{
    // If the link named by the new argument exists, it shall be removed and old renamed to new.
    (void)_unlink(new_f);

    return rename(old_f, new_f);
}
