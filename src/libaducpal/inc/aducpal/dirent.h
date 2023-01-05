#ifndef ADUCPAL_DIRENT_H
#define ADUCPAL_DIRENT_H

#ifdef ADUCPAL_USE_PAL

/*
 * Dirent interface for Microsoft Visual Studio
 *
 * Copyright (C) 1998-2019 Toni Ronkko
 * This file is part of dirent.  Dirent may be freely distributed
 * under the MIT license.  For all details and documentation, see
 * https://github.com/tronkko/dirent
 */

/* Maximum length of file name */
#    if !defined(PATH_MAX)
#        define PATH_MAX 260
#    endif

/* File type flags for d_type */
#    define DT_REG S_IFREG

struct dirent
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
    char d_name[PATH_MAX + 1];
};
typedef struct dirent dirent;

struct _WDIR;

struct __dirstream
{
    struct dirent ent;
    struct _WDIR* wdirp;
};

typedef struct __dirstream DIR;

#    ifdef __cplusplus
extern "C"
{
#    endif

    DIR* ADUCPAL_opendir(const char* dirname);
    struct dirent* ADUCPAL_readdir(DIR* dirp);
    int ADUCPAL_closedir(DIR* dirp);

    int ADUCPAL_scandir(
        const char* dirname,
        struct dirent*** namelist,
        int (*filter)(const struct dirent*),
        int (*compare)(const struct dirent**, const struct dirent**));
    int ADUCPAL_alphasort(const struct dirent** a, const struct dirent** b);

#    ifdef __cplusplus
}
#    endif

#else

#    include <dirent.h>

#    define ADUCPAL_opendir(dirname) opendir(dirname)
#    define ADUCPAL_readdir(dirp) readdir(dirp)
#    define ADUCPAL_closedir(dirp) closedir(dirp)

#    define ADUCPAL_scandir(dirname, namelist, filter, compare) scandir(dirname, namelist, filter, compare)
#    define ADUCPAL_alphasort alphasort

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_DIRENT_H
