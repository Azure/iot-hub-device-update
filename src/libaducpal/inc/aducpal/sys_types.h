#ifndef ADUCPAL_SYS_TYPES_H
#define ADUCPAL_SYS_TYPES_H

#ifdef ADUCPAL_USE_PAL

typedef int gid_t;
typedef int uid_t;
typedef long ssize_t;
typedef unsigned int mode_t;
typedef int pid_t;

#else

#    include <sys/types.h>

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_TYPES_H
