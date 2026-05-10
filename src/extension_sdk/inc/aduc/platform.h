/**
 * @file platform.h
 * @brief Platform abstraction macros and helpers for Windows/Linux portability.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <stddef.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define ADU_SLEEP_MS(ms) Sleep(ms)
#define ADU_SLEEP_SEC(s) Sleep((s) * 1000)
#define ADU_PATH_SEP '\\'
#define ADU_PATH_SEP_STR "\\"
#define ADU_EXPORT __declspec(dllexport)
#define ADU_IMPORT __declspec(dllimport)
#define ADU_SHARED_LIB_EXT ".dll"

/* POSIX-like functions available under different names on Windows */
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define fileno _fileno

/* access() mode constants */
#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif

/* fsync is not available on Windows; use _commit instead */
#include <io.h>
#define fsync(fd) _commit(fd)

/* unlink -> _unlink on Windows */
#define unlink _unlink

/* getpid -> _getpid on Windows */
#include <process.h>
#define getpid _getpid

/* access -> _access on Windows */
#define access _access

/* mkdir on Windows takes only one argument (no mode) */
#include <direct.h>
#define adu_mkdir(path, mode) _mkdir(path)

/* mkstemp replacement for Windows */
#include <fcntl.h>
#include <sys/stat.h>
static inline int adu_mkstemp(char* tmpl)
{
    if (_mktemp_s(tmpl, strlen(tmpl) + 1) != 0)
    {
        return -1;
    }
    int fd;
    if (_sopen_s(&fd, tmpl, _O_CREAT | _O_EXCL | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0)
    {
        return -1;
    }
    return fd;
}

/* close() for file descriptors */
#define close _close

#else /* !_WIN32 */

#include <unistd.h>

#define ADU_SLEEP_MS(ms) usleep((ms) * 1000)
#define ADU_SLEEP_SEC(s) sleep(s)
#define ADU_PATH_SEP '/'
#define ADU_PATH_SEP_STR "/"
#define ADU_EXPORT __attribute__((visibility("default")))
#define ADU_IMPORT
#define ADU_SHARED_LIB_EXT ".so"

#define adu_mkdir(path, mode) mkdir(path, mode)
#define adu_mkstemp(tmpl) mkstemp(tmpl)

#endif /* _WIN32 */
