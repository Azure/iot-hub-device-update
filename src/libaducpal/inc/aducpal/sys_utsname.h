#ifndef ADUCPAL_SYS_UTSNAME_H
#define ADUCPAL_SYS_UTSNAME_H

#ifdef ADUCPAL_USE_PAL

// This project onl y references sysname, release, machine
struct utsname
{
    char* sysname; /* Operating system name (e.g., "Linux") */
    // char nodename[]; /* Name within "some implementation-defined network" */
    char* release; /* Operating system release (e.g., "2.6.28") */
    // char version[]; /* Operating system version */
    char* machine; /* Hardware identifier */
};

#    ifdef __cplusplus
extern "C"
{
#    endif
    int ADUCPAL_uname(struct utsname* buf);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/utsname.h>

#    define ADUCPAL_utsname(buf) utsname(buf)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_UTSNAME_H
