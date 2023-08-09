#ifndef ADUCPAL_PWD_H
#define ADUCPAL_PWD_H

#ifdef ADUCPAL_USE_PAL

#    include "aducpal/sys_types.h" // uid_t

// // This project only references pw_uid
struct passwd
{
    // char   *pw_name;       /* username */
    // char   *pw_passwd;     /* user password */
    uid_t pw_uid; /* user ID */
    // gid_t   pw_gid;        /* group ID */
    // char   *pw_gecos;      /* user information */
    // char   *pw_dir;        /* home directory */
    // char   *pw_shell;      /* shell program */
};

#    ifdef __cplusplus
extern "C"
{
#    endif

    struct passwd* ADUCPAL_getpwnam(const char* name);

#    ifdef __cplusplus
}
#    endif

#else

#    include <pwd.h>

#    define ADUCPAL_getpwnam getpwnam

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_PWD_H
