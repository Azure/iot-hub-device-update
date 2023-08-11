#include "aducpal/dirent.h"

/*
 * Dirent interface for Microsoft Visual Studio
 *
 * Copyright (C) 1998-2019 Toni Ronkko
 * This file is part of dirent.  Dirent may be freely distributed
 * under the MIT license.  For all details and documentation, see
 * https://github.com/tronkko/dirent
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <malloc.h> // malloc
#include <stdlib.h> // mbstowcs_s
#include <sys/stat.h> // S_*

struct _wdirent
{
    /* Always zero */
    long d_ino;

    /* Position of next file in a directory stream */
    long d_off;

    /* Structure size */
    unsigned short d_reclen;

    /* Length of name without \0 */
    size_t d_namlen;

    /* File type */
    int d_type;

    /* File name */
    wchar_t d_name[PATH_MAX + 1];
};

typedef struct _wdirent _wdirent;

struct _WDIR
{
    /* Current directory entry */
    struct _wdirent ent;

    /* Private file data */
    WIN32_FIND_DATAW data;

    /* True if data is valid */
    int cached;

    /* True if next entry is invalid */
    int invalid;

    /* Win32 search handle */
    HANDLE handle;

    /* Initial directory name */
    wchar_t* patt;
};
typedef struct _WDIR _WDIR;

#define dirent_set_errno _set_errno

/* Get first directory entry */
static WIN32_FIND_DATAW* dirent_first(_WDIR* dirp)
{
    /* Open directory and retrieve the first entry */
    dirp->handle = FindFirstFileExW(dirp->patt, FindExInfoStandard, &dirp->data, FindExSearchNameMatch, NULL, 0);
    if (dirp->handle == INVALID_HANDLE_VALUE)
        goto error;

    /* A directory entry is now waiting in memory */
    dirp->cached = 1;
    return &dirp->data;

error:
    /* Failed to open directory: no directory entry in memory */
    dirp->cached = 0;
    dirp->invalid = 1;

    /* Set error code */
    DWORD errorcode = GetLastError();
    switch (errorcode)
    {
    case ERROR_ACCESS_DENIED:
        /* No read access to directory */
        dirent_set_errno(EACCES);
        break;

    case ERROR_DIRECTORY:
        /* Directory name is invalid */
        dirent_set_errno(ENOTDIR);
        break;

    case ERROR_PATH_NOT_FOUND:
    default:
        /* Cannot find the file */
        dirent_set_errno(ENOENT);
    }
    return NULL;
}

/*
 * Close directory stream opened by opendir() function.  This invalidates the
 * DIR structure as well as any directory entry read previously by
 * _wreaddir().
 */
static int _wclosedir(_WDIR* dirp)
{
    if (!dirp)
    {
        dirent_set_errno(EBADF);
        return /*failure*/ -1;
    }

    /*
	 * Release search handle if we have one.  Being able to handle
	 * partially initialized _WDIR structure allows us to use this
	 * function to handle errors occuring within _wopendir.
	 */
    if (dirp->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(dirp->handle);
    }

    /*
	 * Release search pattern.  Note that we don't need to care if
	 * dirp->patt is NULL or not: function free is guaranteed to act
	 * appropriately.
	 */
    free(dirp->patt);

    /* Release directory structure */
    free(dirp);
    return /*success*/ 0;
}

/*
 * Open directory stream DIRNAME for read and return a pointer to the
 * internal working area that is used to retrieve individual directory
 * entries.
 */
static _WDIR* _wopendir(const wchar_t* dirname)
{
    wchar_t* p;

    /* Must have directory name */
    if (dirname == NULL || dirname[0] == '\0')
    {
        dirent_set_errno(ENOENT);
        return NULL;
    }

    /* Allocate new _WDIR structure */
    _WDIR* dirp = (_WDIR*)malloc(sizeof(struct _WDIR));
    if (!dirp)
        return NULL;

    /* Reset _WDIR structure */
    dirp->handle = INVALID_HANDLE_VALUE;
    dirp->patt = NULL;
    dirp->cached = 0;
    dirp->invalid = 0;

    /*
	 * Compute the length of full path plus zero terminator
	 */
    DWORD n = GetFullPathNameW(dirname, 0, NULL, NULL);

    /* Allocate room for absolute directory name and search pattern */
    dirp->patt = (wchar_t*)malloc(sizeof(wchar_t) * n + 16);
    if (dirp->patt == NULL)
    {
        goto exit_closedir;
    }

    /*
	 * Convert relative directory name to an absolute one.  This
	 * allows rewinddir() to function correctly even when current
	 * working directory is changed between opendir() and rewinddir().
	 */
    n = GetFullPathNameW(dirname, n, dirp->patt, NULL);
    if (n <= 0)
    {
        goto exit_closedir;
    }

    /* Append search pattern \* to the directory name */
    p = dirp->patt + n;
    switch (p[-1])
    {
    case '\\':
    case '/':
    case ':':
        /* Directory ends in path separator, e.g. c:\temp\ */
        /*NOP*/;
        break;

    default:
        /* Directory name doesn't end in path separator */
        *p++ = '\\';
    }
    *p++ = '*';
    *p = '\0';

    /* Open directory stream and retrieve the first entry */
    if (!dirent_first(dirp))
    {
        goto exit_closedir;
    }

    /* Success */
    return dirp;

    /* Failure */
exit_closedir:
    _wclosedir(dirp);
    return NULL;
}

/* Open directory stream using plain old C-string */
DIR* ADUCPAL_opendir(const char* dirname)
{
    /* Must have directory name */
    if (dirname == NULL || dirname[0] == '\0')
    {
        dirent_set_errno(ENOENT);
        return NULL;
    }

    /* Allocate memory for DIR structure */
    DIR* dirp = (DIR*)malloc(sizeof(DIR));
    if (!dirp)
    {
        return NULL;
    }

    /* Convert directory name to wide-character string */
    wchar_t wname[PATH_MAX + 1];
    size_t n;
    int error = mbstowcs_s(&n, wname, PATH_MAX + 1, dirname, PATH_MAX + 1);
    if (error)
    {
        goto exit_failure;
    }

    /* Open directory stream using wide-character name */
    dirp->wdirp = _wopendir(wname);
    if (!dirp->wdirp)
    {
        goto exit_failure;
    }

    /* Success */
    return dirp;

    /* Failure */
exit_failure:
    free(dirp);
    return NULL;
}

/* Get next directory entry */
static WIN32_FIND_DATAW* dirent_next(_WDIR* dirp)
{
    /* Return NULL if seek position was invalid */
    if (dirp->invalid)
    {
        return NULL;
    }

    /* Is the next directory entry already in cache? */
    if (dirp->cached)
    {
        /* Yes, a valid directory entry found in memory */
        dirp->cached = 0;
        return &dirp->data;
    }

    /* Read the next directory entry from stream */
    if (FindNextFileW(dirp->handle, &dirp->data) == FALSE)
    {
        /* End of directory stream */
        return NULL;
    }

    /* Success */
    return &dirp->data;
}

/*
 * Compute 31-bit hash of file name.
 *
 * See djb2 at http://www.cse.yorku.ca/~oz/hash.html
 */
static long dirent_hash(WIN32_FIND_DATAW* datap)
{
    unsigned long hash = 5381;
    unsigned long c;
    const wchar_t* p = datap->cFileName;
    const wchar_t* e = p + MAX_PATH;
    while (p != e && (c = *p++) != 0)
    {
        hash = (hash << 5) + hash + c;
    }

    return (long)(hash & ((~0UL) >> 1));
}

/*
 * Read next directory entry into called-allocated buffer.
 *
 * Returns zero on success.  If the end of directory stream is reached, then
 * sets result to NULL and returns zero.
 */
static int readdir_r(DIR* dirp, struct dirent* entry, struct dirent** result)
{
    /* Read next directory entry */
    WIN32_FIND_DATAW* datap = dirent_next(dirp->wdirp);
    if (!datap)
    {
        /* No more directory entries */
        *result = NULL;
        return /*OK*/ 0;
    }

    /* Attempt to convert file name to multi-byte string */
    size_t n;
    int error = wcstombs_s(&n, entry->d_name, PATH_MAX + 1, datap->cFileName, PATH_MAX + 1);

    /*
	 * If the file name cannot be represented by a multi-byte string, then
	 * attempt to use old 8+3 file name.  This allows the program to
	 * access files although file names may seem unfamiliar to the user.
	 *
	 * Be ware that the code below cannot come up with a short file name
	 * unless the file system provides one.  At least VirtualBox shared
	 * folders fail to do this.
	 */
    if (error && datap->cAlternateFileName[0] != '\0')
    {
        error = wcstombs_s(&n, entry->d_name, PATH_MAX + 1, datap->cAlternateFileName, PATH_MAX + 1);
    }

    if (!error)
    {
        /* Length of file name excluding zero terminator */
        entry->d_namlen = n - 1;

        /* Determine file type */
        DWORD attr = datap->dwFileAttributes;
        if ((attr & FILE_ATTRIBUTE_DEVICE) != 0)
        {
            entry->d_type = DT_CHR;
        }
        else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            entry->d_type = DT_DIR;
        }
        else
        {
            entry->d_type = DT_REG;
        }

        /* Get offset of next file */
        datap = dirent_next(dirp->wdirp);
        if (datap)
        {
            /* Compute 31-bit hash of the next directory entry */
            entry->d_off = dirent_hash(datap);

            /* Push the next directory entry back to cache */
            dirp->wdirp->cached = 1;
        }
        else
        {
            /* End of directory stream */
            entry->d_off = (long)((~0UL) >> 1);
        }

        /* Reset fields */
        entry->d_ino = 0;
        entry->d_reclen = sizeof(struct dirent);
    }
    else
    {
        /*
		 * Cannot convert file name to multi-byte string so construct
		 * an erroneous directory entry and return that.  Note that
		 * we cannot return NULL as that would stop the processing
		 * of directory entries completely.
		 */
        entry->d_name[0] = '?';
        entry->d_name[1] = '\0';
        entry->d_namlen = 1;
        entry->d_type = DT_UNKNOWN;
        entry->d_ino = 0;
        entry->d_off = -1;
        entry->d_reclen = 0;
    }

    /* Return pointer to directory entry */
    *result = entry;
    return /*OK*/ 0;
}

struct dirent* ADUCPAL_readdir(DIR* dirp)
{
    /*
	 * Read directory entry to buffer.  We can safely ignore the return
	 * value as entry will be set to NULL in case of error.
	 */
    struct dirent* entry;
    (void)readdir_r(dirp, &dirp->ent, &entry);

    /* Return pointer to statically allocated directory entry */
    return entry;
}

/* Close directory stream */
int ADUCPAL_closedir(DIR* dirp)
{
    int ok;

    if (!dirp)
    {
        goto exit_failure;
    }

    /* Close wide-character directory stream */
    ok = _wclosedir(dirp->wdirp);
    dirp->wdirp = NULL;

    /* Release multi-byte character version */
    free(dirp);
    return ok;

exit_failure:
    /* Invalid directory stream */
    dirent_set_errno(EBADF);
    return /*failure*/ -1;
}

/* Scan directory for entries */
int ADUCPAL_scandir(
    const char* dirname,
    struct dirent*** namelist,
    int (*filter)(const struct dirent*),
    int (*compare)(const struct dirent**, const struct dirent**))
{
    int result;

    /* Open directory stream */
    DIR* dir = ADUCPAL_opendir(dirname);
    if (!dir)
    {
        /* Cannot open directory */
        return /*Error*/ -1;
    }

    /* Read directory entries to memory */
    struct dirent* tmp = NULL;
    struct dirent** files = NULL;
    size_t size = 0;
    size_t allocated = 0;
    while (1)
    {
        /* Allocate room for a temporary directory entry */
        if (!tmp)
        {
            tmp = (struct dirent*)malloc(sizeof(struct dirent));
            if (!tmp)
            {
                goto exit_failure;
            }
        }

        /* Read directory entry to temporary area */
        struct dirent* entry;
        if (readdir_r(dir, tmp, &entry) != /*OK*/ 0)
        {
            goto exit_failure;
        }

        /* Stop if we already read the last directory entry */
        if (entry == NULL)
        {
            goto exit_success;
        }

        /* Determine whether to include the entry in results */
        if (filter && !filter(tmp))
        {
            continue;
        }

        /* Enlarge pointer table to make room for another pointer */
        if (size >= allocated)
        {
            /* Compute number of entries in the new table */
            size_t num_entries = size * 2 + 16;

            /* Allocate new pointer table or enlarge existing */
            void* p = realloc(files, sizeof(void*) * num_entries);
            if (!p)
            {
                goto exit_failure;
            }

            /* Got the memory */
            files = (dirent**)p;
            allocated = num_entries;
        }

        /* Store the temporary entry to ptr table */
        files[size++] = tmp;
        tmp = NULL;
    }

exit_failure:
    /* Release allocated entries */
    for (size_t i = 0; i < size; i++)
    {
        free(files[i]);
    }

    /* Release the pointer table */
    free(files);
    files = NULL;

    /* Exit with error code */
    result = /*error*/ -1;
    goto exit_status;

exit_success:
    /* Sort directory entries */
    qsort(files, size, sizeof(void*), (int (*)(const void*, const void*))compare);

    /* Pass pointer table to caller */
    if (namelist)
    {
        *namelist = files;
    }

    /* Return the number of directory entries read */
    result = (int)size;

exit_status:
    /* Release temporary directory entry, if we had one */
    free(tmp);

    /* Close directory stream */
    ADUCPAL_closedir(dir);
    return result;
}

/* Alphabetical sorting */
int ADUCPAL_alphasort(const struct dirent** a, const struct dirent** b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}
