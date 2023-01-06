#include "aducpal/ftw.h"

#ifdef ADUCPAL_USE_PAL

// TODO(JeffMill): [PAL] Check license

#    if !defined(PATH_MAX)
#        define PATH_MAX 260
#    endif

// Modified version of this code:

/*
 * nftw_utf8.c v1.0.0 <z64.me>
 *
 * a win32 solution for nftw
 *
 * _wnftw() uses wchar_t regardless of _UNICODE status (on Windows)
 * _tnftw() uses _wnftw if Windows _UNICODE is defined, nftw otherwise
 * nftw_utf8() works like _tnftw internally, but inputs/outputs utf8
 *
 * based on the documentation from the Linux programmer's manual:
 * https://man7.org/linux/man-pages/man3/ftw.3.html
 *
 * win32-specific code adapted from this example on Rosetta Code:
 * https://rosettacode.org/wiki/Walk_a_directory/Recursively#Windows
 *
 * the musl implementation was a valuable reference:
 * http://git.musl-libc.org/cgit/musl/tree/src/misc/nftw.c
 *
 * known limitations:
 *  - symlinks and shortcuts don't work on Windows
 *  - does not support FTW_ACTIONRETVAL
 *
 */

// #    include <errno.h>
// #    include <stdio.h>
// #    include <stdlib.h>
// #    include <wchar.h>

#    include <malloc.h> // malloc
#    include <sys/stat.h> // dev_t, ino_t
#    include <wchar.h> // wmemcpy

#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>

struct history
{
    struct history* chain;
    dev_t dev;
    ino_t ino;
    int level;
    int base;
};

// Convert a string in place, changing \ to /
static void make_forward_slash(wchar_t* path)
{
    while (*path != L'\0')
    {
        if (*path == L'\\')
        {
            *path = L'/';
        }
        ++path;
    }
}

static char* wstr_to_utf8(const wchar_t* wstr)
{
    if (!wstr)
    {
        return 0;
    }

    const wchar_t* src = wstr;

    const int src_length = (int)wcslen(src);
    const int length = WideCharToMultiByte(CP_UTF8, 0, src, src_length, 0, 0, NULL, NULL);

    char* out = malloc((length + 1) * sizeof(char));
    if (out != NULL)
    {
        WideCharToMultiByte(CP_UTF8, 0, src, src_length, out, length, NULL, NULL);
        out[length] = '\0';
    }
    return out;
}

static wchar_t* utf8_to_wstr(const char* str)
{
    if (!str)
    {
        return 0;
    }

    const char* src = str;
    const int src_length = (int)strlen(src);

    const int length = MultiByteToWideChar(CP_UTF8, 0, src, src_length, 0, 0);
    wchar_t* out = malloc((length + 1) * sizeof(*out));
    if (out != NULL)
    {
        MultiByteToWideChar(CP_UTF8, 0, src, src_length, out, length);
        out[length] = L'\0';
    }
    return out;
}

static int
do_callback(NFTW_FUNC_T func, const wchar_t* wfpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf)
{
    int rval = 0;

    // Callback is expecting a utf8 string.
    char* fpath = wstr_to_utf8(wfpath);
    if (!fpath)
    {
        return -1;
    }

    rval = func(fpath, sb, typeflag, ftwbuf);

    free(fpath);
    return rval;
}

/* construct typeflag as described in the ftw manual */
static int construct_typeflag(const DWORD attrib, int flags)
{
    int typeflag = 0;

    if (attrib == INVALID_FILE_ATTRIBUTES)
    {
        return -1;
    }

    /* is a directory */
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        /* FTW_DEPTH was specified in flags */
        if (flags & FTW_DEPTH)
        {
            typeflag = FTW_DP;
        }
        else
        {
            typeflag = FTW_D;
        }

        /* is a directory which can't be read */
        if (attrib & FILE_ATTRIBUTE_READONLY)
        {
            typeflag = FTW_DNR;
        }
    }
    /* is a regular file */
    else
    {
        typeflag = FTW_F;
    }

    return typeflag;
};

/* this function was adapted from a public domain Rosetta Code example:
 * https://rosettacode.org/wiki/Walk_a_directory/Recursively#Windows
 */
static int do_wnftw(wchar_t* wpath, NFTW_FUNC_T func, int fd_limit, int flags, struct history* h)
{
    struct stack
    {
        wchar_t* path;
        size_t pathlen;
        size_t slashlen;
        HANDLE ffh;
        WIN32_FIND_DATAW ffd;
        struct stack* next;
    } *dir, dir0, *ndir;
    size_t patternlen;
    wchar_t *buf, c, *pattern;
    int rval = 0;

    dir0.path = wpath;
    dir0.pathlen = wcslen(dir0.path);
    pattern = L"*";
    patternlen = wcslen(pattern);

    //
    // Must put backslash between path and pattern, unless
    // last character of path is slash or colon.
    //
    //   'dir' => 'dir\*'
    //   'dir\' => 'dir\*'
    //   'dir/' => 'dir/*'
    //   'c:' => 'c:*'
    //
    // 'c:*' and 'c:\*' are different files!
    //
    c = dir0.path[dir0.pathlen - 1];
    if (c == ':' || c == '/' || c == '\\')
    {
        dir0.slashlen = dir0.pathlen;
    }
    else
    {
        dir0.slashlen = dir0.pathlen + 1;
    }

    /* Allocate space for path + backslash + pattern + \0. */
    buf = calloc(dir0.slashlen + patternlen + 1, sizeof buf[0]);
    if (buf == NULL)
    {
        return -1;
    }

    dir0.path = wmemcpy(buf, dir0.path, dir0.pathlen + 1);
    dir0.ffh = INVALID_HANDLE_VALUE;
    dir0.next = NULL;

    dir = &dir0;

    /* Loop for each directory in linked list. */
loop:
    while (dir)
    {
        /*
		 * At first visit to directory:
		 *   Print the matching files. Then, begin to find
		 *   subdirectories.
		 *
		 * At later visit:
		 *   dir->ffh is the handle to find subdirectories.
		 *   Continue to find them.
		 */
        if (dir->ffh == INVALID_HANDLE_VALUE)
        {
            /* Append backslash + pattern + \0 to path. */
            dir->path[dir->pathlen] = '\\';
            wmemcpy(dir->path + dir->slashlen, pattern, patternlen + 1);

            /* Find all files to match pattern. */
            dir->ffh = FindFirstFileW(dir->path, &dir->ffd);
            if (dir->ffh == INVALID_HANDLE_VALUE)
            {
                /* Check if no files match pattern. */
                if (GetLastError() == ERROR_FILE_NOT_FOUND)
                    goto subdirs;

                /* Bail out from other errors. */
                dir->path[dir->pathlen] = '\0';
                // oops(dir->path);
                rval = 1;
                goto popdir;
            }

            /* Remove pattern from path; keep backslash. */
            dir->path[dir->slashlen] = '\0';

            /* Print all files to match pattern. */
            do
            {
                const DWORD attr = dir->ffd.dwFileAttributes;
                int typeflag = construct_typeflag(attr, flags);

                struct FTW lev = { 0 };

                struct _stat st64i32;
                _wstat(wpath, &st64i32);

                // TODO: Convert to stat. (Only st_dev and st_ino are used, however)
                struct stat st;
                memset(&st, sizeof(st), 0);
                st.st_dev = st64i32.st_dev;
                st.st_ino = st64i32.st_ino;
                st.st_mode = st64i32.st_mode;
                st.st_mode = st64i32.st_mode;
                st.st_nlink = st64i32.st_nlink;
                st.st_uid = st64i32.st_uid;
                st.st_gid = st64i32.st_gid;
                st.st_rdev = st64i32.st_rdev;
                st.st_size = st64i32.st_size;
                st.st_atime = st64i32.st_atime;
                st.st_mtime = st64i32.st_mtime;
                st.st_ctime = st64i32.st_ctime;

                struct history new;
                new.chain = h;
                new.dev = st.st_dev;
                new.ino = st.st_ino;
                new.level = h ? h->level + 1 : 0;

                /* TODO not recursive, so refactor this to make lev be
				 * function-scope and increment/decerment lev.level as
				 * directories are entered/exited; it will also know to
				 * exit when fd_limit is exceeded
				 */
                (void)fd_limit;
                lev.level = new.level;
                lev.base = (int)wcslen(dir->path);

                const wchar_t* thisname = dir->ffd.cFileName;

                /* skip directories named '.' and '..' */
                if (thisname[0] == L'.' && (!thisname[1] || (thisname[1] == L'.' && !thisname[2])))
                {
                    continue;
                }

                wchar_t full[MAX_PATH];
                wcscpy_s(full, sizeof(full) / sizeof(full[0]), dir->path);
                wcscat_s(full, sizeof(full) / sizeof(full[0]), thisname);

                make_forward_slash(full);

                // An invocation of fn() returns a nonzero value, in which case nftw() returns that value.
                int r = do_callback(func, full, &st, typeflag, &lev);
                if (r != 0)
                {
                    return r;
                }

            } while (FindNextFileW(dir->ffh, &dir->ffd) != 0);
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                dir->path[dir->pathlen] = '\0';
                // oops(dir->path);
                rval = 1;
            }
            FindClose(dir->ffh);

        subdirs:
            /* Append * + \0 to path. */
            dir->path[dir->slashlen] = '*';
            dir->path[dir->slashlen + 1] = '\0';

            /* Find first possible subdirectory. */
            dir->ffh =
                FindFirstFileExW(dir->path, FindExInfoStandard, &dir->ffd, FindExSearchLimitToDirectories, NULL, 0);
            if (dir->ffh == INVALID_HANDLE_VALUE)
            {
                dir->path[dir->pathlen] = '\0';
                // oops(dir->path);
                rval = 1;
                goto popdir;
            }
        }
        else
        {
            /* Find next possible subdirectory. */
            if (FindNextFileW(dir->ffh, &dir->ffd) == 0)
                goto closeffh;
        }

        /* Enter subdirectories. */
        do
        {
            const wchar_t* fn = dir->ffd.cFileName;
            const DWORD attr = dir->ffd.dwFileAttributes;
            size_t buflen, fnlen;

            /*
			 * Skip '.' and '..', because they are links to
			 * the current and parent directories, so they
			 * are not subdirectories.
			 *
			 * Skip any file that is not a directory.
			 *
			 * Skip all reparse points, because they might
			 * be symbolic links. They might form a cycle,
			 * with a directory inside itself.
			 */
            if (wcscmp(fn, L".") == 0 || wcscmp(fn, L"..") == 0 || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0
                || (attr & FILE_ATTRIBUTE_REPARSE_POINT))
                continue;

            ndir = malloc(sizeof *ndir);
            if (ndir == NULL)
            {
                return -1;
            }

            /*
			 * Allocate space for path + backslash +
			 *     fn + backslash + pattern + \0.
			 */
            fnlen = wcslen(fn);
            buflen = dir->slashlen + fnlen + patternlen + 2;
            buf = calloc(buflen, sizeof buf[0]);
            if (buf == NULL)
            {
                return -1;
            }

            /* Copy path + backslash + fn + \0. */
            wmemcpy(buf, dir->path, dir->slashlen);
            wmemcpy(buf + dir->slashlen, fn, fnlen + 1);

            /* Push dir to list. Enter dir. */
            ndir->path = buf;
            ndir->pathlen = dir->slashlen + fnlen;
            ndir->slashlen = ndir->pathlen + 1;
            ndir->ffh = INVALID_HANDLE_VALUE;
            ndir->next = dir;
            dir = ndir;
            goto loop; /* Continue outer loop. */
        } while (FindNextFileW(dir->ffh, &dir->ffd) != 0);
    closeffh:
        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            dir->path[dir->pathlen] = '\0';
            // oops(dir->path);
            rval = 1;
        }
        FindClose(dir->ffh);

    popdir:
        /* Pop dir from list, free dir, but never free dir0. */
        free(dir->path);
        if ((ndir = dir->next))
            free(dir);
        dir = ndir;
    }

    return rval;
}

static int _wnftw(const wchar_t* wpath, NFTW_FUNC_T func, int fd_limit, int flags)
{
    if (fd_limit <= 0)
    {
        return 0;
    }

    const size_t l = wcslen(wpath);
    if (l > PATH_MAX)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    wchar_t pathbuf[PATH_MAX + 1];
    wcscpy_s(pathbuf, sizeof(pathbuf) / sizeof(pathbuf[0]), wpath);

    make_forward_slash(pathbuf);

    return do_wnftw(pathbuf, func, fd_limit, flags, NULL);
}

int ADUCPAL_nftw(const char* path, NFTW_FUNC_T func, int nopenfd, int flags)
{
    if (nopenfd <= 0)
    {
        return 0;
    }

    // This code uses wchar_t internally for Windows APIs, but will convert back to UTF8 for the func callback.
    wchar_t* wpath = utf8_to_wstr(path);
    if (!wpath)
    {
        return -1;
    }

    const size_t l = wcslen(wpath);
    if (l > PATH_MAX)
    {
        free(wpath);
        errno = ENAMETOOLONG;
        return -1;
    }

    make_forward_slash(wpath);

    const int ret = do_wnftw(wpath, func, nopenfd, flags, NULL);

    free(wpath);
    return ret ? -1 : 0;
}

#endif // #ifdef ADUCPAL_USE_PAL
