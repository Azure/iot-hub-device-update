#ifndef ADUCPAL_GRP_H
#define ADUCPAL_GRP_H

#ifdef ADUCPAL_USE_PAL

#    include "aducpal/sys_types.h" // gid_t

// This project only references gr_gid, gr_mem
struct group
{
    // char   *gr_name;       /* group name */
    // char   *gr_passwd;     /* group password */
    gid_t gr_gid; /* group ID */
    char** gr_mem; /* group members */
};

#    ifdef __cplusplus
extern "C"
{
#    endif

    struct group* ADUCPAL_getgrnam(const char* name);

#    ifdef __cplusplus
}
#    endif

#else

#    include <grp.h>

#    define ADUCPAL_getgrnam getgrnam

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_GRP_H
