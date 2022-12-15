#ifndef ADUCPAL_SYS_STAT_H
#define ADUCPAL_SYS_STAT_H

#if defined(ADUCPAL_USE_PAL)

typedef unsigned int mode_t;

#    define __S_IREAD 0400 /* Read by owner.  */
#    define __S_IREAD 0400 /* Read by owner.  */
#    define __S_IWRITE 0200 /* Write by owner.  */
#    define __S_IEXEC 0100 /* Execute by owner.  */

#    define S_IRUSR __S_IREAD /* Read by owner.  */
#    define S_IWUSR __S_IWRITE /* Write by owner.  */
#    define S_IXUSR __S_IEXEC /* Execute by owner.  */
/* Read, write, and execute by owner.  */
#    define S_IRWXU (__S_IREAD | __S_IWRITE | __S_IEXEC)

int ADUCPAL_mkdir(const char* path, mode_t mode);

#else

#    include <sys/stat.h>

#    define ADUCPAL_mkdir(path, mode) mkdir(path, mode)

#endif // defined(ADUCPAL_USE_PAL)

#endif // ADUCPAL_SYS_STAT_H
