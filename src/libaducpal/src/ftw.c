#include "aducpal/ftw.h"
#include "aducpal/dirent.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// Returns true if . or ..
static bool is_dots_dir(const struct dirent* entry)
{
    if ((entry->d_type == DT_DIR) && (entry->d_name[0] == '.'))
    {
        if ((entry->d_name[1] == '\0') || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))
        {
            return true;
        }
    }
    return false;
}

static void wstat_to_stat(struct stat* st, struct _stat* st64i32)
{
    memset(st, 0, sizeof(*st));
    st->st_dev = st64i32->st_dev;
    st->st_ino = st64i32->st_ino;
    st->st_mode = st64i32->st_mode;
    st->st_mode = st64i32->st_mode;
    st->st_nlink = st64i32->st_nlink;
    st->st_uid = st64i32->st_uid;
    st->st_gid = st64i32->st_gid;
    st->st_rdev = st64i32->st_rdev;
    st->st_size = st64i32->st_size;
    st->st_atime = st64i32->st_atime;
    st->st_mtime = st64i32->st_mtime;
    st->st_ctime = st64i32->st_ctime;
}

static int typeflag_from_stat(struct stat* st, int flags)
{
    // Note: FTW_SL and FTW_SLN not currently implemented.

    int typeflag = 0;

    if (st->st_mode & _S_IFDIR)
    {
        if (flags & FTW_DEPTH)
        {
            typeflag = FTW_DP;
        }
        else
        {
            typeflag = FTW_D;
        }

        if (!(st->st_mode & _S_IREAD))
        {
            typeflag = FTW_DNR;
        }
    }
    else
    {
        typeflag = FTW_F;
    }

    return typeflag;
};

static int do_callback(NFTW_FUNC_T func, const char* fpath, int flags, struct FTW* ftwbuf)
{
    struct _stat st64i32;
    if (_stat(fpath, &st64i32) != 0)
    {
        return -1;
    }

    struct stat st;
    wstat_to_stat(&st, &st64i32);

    const int typeflag = typeflag_from_stat(&st, flags);

    // If func() returns nonzero, then the tree walk is terminated and the value returned by fn()
    // is returned as the result of nftw().
    return func(fpath, &st, typeflag, ftwbuf);
}

// Return 0 on success, -1 if an error occurs.
static int do_nftw(const char* path, NFTW_FUNC_T func, int nopenfd, int flags, int level)
{
    int ret = 0;

    DIR* dp = ADUCPAL_opendir(path);
    if (dp == NULL)
    {
        return -1;
    }

    struct FTW ftwbuf = { 0 };
    // level is the depth of fpath in the directory tree, relative to the root of the tree (dirpath, which has depth 0).
    ftwbuf.level = level;
    // base is the offset of the filename (i.e. basename component) in the pathname given in fpath.
    ftwbuf.base = (int)strlen(path);

    if (!(flags & FTW_DEPTH) && level == 0)
    {
        // Do an initial callback for the folder that was initially passed in
        ret = do_callback(func, path, flags, &ftwbuf);
    }

    struct dirent* entry;
    while ((entry = ADUCPAL_readdir(dp)) != NULL)
    {
        if (is_dots_dir(entry))
        {
            continue;
        }

        if (entry->d_type == DT_DIR)
        {
            if (flags & FTW_MOUNT)
            {
                WIN32_FILE_ATTRIBUTE_DATA wfd;
                if (!GetFileAttributesEx(path, GetFileExInfoStandard, &wfd))
                {
                    continue;
                }

                // If set, stay within the same filesystem (i.e., do not cross mount points).
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                {
                    continue;
                }
            }

            ret = 0;

            // Future optimization: remove subdir and filepath allocations; pass mutable buffer around
            const size_t buffer_len = ftwbuf.base + 1 + entry->d_namlen + 1;
            char* subdir = malloc(buffer_len);
            sprintf_s(subdir, buffer_len, "%s/%s", path, entry->d_name);

            // FTW_DEPTH
            // If set, do a post-order traversal, that is, call fn() for the directory itself after handling the contents of the
            //  directory and its subdirectories. (By default, each directory is handled before its contents.)

            if (flags & FTW_DEPTH)
            {
                ret = do_nftw(subdir, func, nopenfd, flags, level + 1);
                if (ret != 0)
                {
                    return ret;
                }
            }

            if (ret == 0)
            {
                ret = do_callback(func, subdir, flags, &ftwbuf);
            }

            if (ret == 0)
            {
                if (!(flags & FTW_DEPTH))
                {
                    ret = do_nftw(subdir, func, nopenfd, flags, level + 1);
                }
            }

            free(subdir);

            if (ret != 0)
            {
                return ret;
            }
        }
        else if (entry->d_type == DT_REG)
        {
            const size_t buffer_len = ftwbuf.base + 1 + entry->d_namlen + 1;
            char* filepath = malloc(buffer_len);
            sprintf_s(filepath, buffer_len, "%s/%s", path, entry->d_name);

            ret = do_callback(func, filepath, flags, &ftwbuf);

            free(filepath);

            if (ret != 0)
            {
                return ret;
            }
        }
    }

    if ((flags & FTW_DEPTH) && level == 0)
    {
        // Do a final callback for the folder that was initially passed in
        ret = do_callback(func, path, flags, &ftwbuf);
    }

    if (dp != NULL)
    {
        ADUCPAL_closedir(dp);
    }

    return ret;
}

// Returns 0 on success, and -1 if an error occurs.
int ADUCPAL_nftw(const char* path, NFTW_FUNC_T func, int nopenfd, int flags)
{
    // returns -1 if an error occurs.

    // Note: Ignoring nopenfd not currently implemented.

    // Note: FTW_PHYS not currently implemetented (code references, so not failing on use)

    if (flags & FTW_CHDIR)
    {
        // Note: FTW_CHDIR not currently implemented.
        _set_errno(ENOSYS);
        return -1;
    }

    return do_nftw(path, func, nopenfd, flags, 0 /*level*/);
}
