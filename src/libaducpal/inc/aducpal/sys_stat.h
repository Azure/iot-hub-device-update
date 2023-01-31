#ifndef ADUCPAL_SYS_STAT_H
#define ADUCPAL_SYS_STAT_H

#ifdef ADUCPAL_USE_PAL

#    include "aducpal/sys_types.h" // mode_t

/* Encoding of the file mode.  These are the standard Unix values,
   but POSIX.1 does not specify what values should be used.  */

#    define __S_IFMT 0170000 /* These bits determine file type.  */

#    define __S_ISVTX 01000 /* Save swapped text after use (sticky).  */

/* File types.  */

#    define __S_IFDIR 0040000 /* Directory.  */
#    define __S_IFCHR 0020000 /* Character device.  */
#    define __S_IFREG 0100000 /* Regular file.  */
#    define __S_IFLNK 0120000 /* Symbolic link.  */

/* Test macros for file types.	*/

#    define __S_ISTYPE(mode, mask) (((mode)&__S_IFMT) == (mask))

#    define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#    define S_ISCHR(mode) __S_ISTYPE((mode), __S_IFCHR)
#    define S_ISLNK(mode) __S_ISTYPE((mode), __S_IFLNK)
#    define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

/* Protection bits.  */

/* Save swapped text after use (sticky bit).  This is pretty well obsolete.  */
#    define S_ISVTX __S_ISVTX

#    define __S_ISUID 04000 /* Set user ID on execution.  */

#    define __S_IREAD 0400 /* Read by owner.  */
#    define __S_IWRITE 0200 /* Write by owner.  */
#    define __S_IEXEC 0100 /* Execute by owner.  */

#    define S_ISUID __S_ISUID /* Set user ID on execution.  */

#    define S_IRUSR __S_IREAD /* Read by owner.  */
#    define S_IWUSR __S_IWRITE /* Write by owner.  */
#    define S_IXUSR __S_IEXEC /* Execute by owner.  */
/* Read, write, and execute by owner.  */
#    define S_IRWXU (__S_IREAD | __S_IWRITE | __S_IEXEC)

#    define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#    define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#    define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */
/* Read, write, and execute by group.  */
#    define S_IRWXG (S_IRWXU >> 3)

#    define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#    define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#    define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
#    define S_IRWXO (S_IRWXG >> 3)

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_chmod(const char* path, mode_t mode);

    int ADUCPAL_mkdir(const char* path, mode_t mode);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/stat.h>

#    define ADUCPAL_chmod chmod
#    define ADUCPAL_mkdir mkdir

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_STAT_H
