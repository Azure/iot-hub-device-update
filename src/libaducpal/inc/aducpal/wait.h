#ifndef ADUCPAL_WAIT_H
#define ADUCPAL_WAIT_H

#ifdef ADUCPAL_USE_PAL

#    include <aducpal/sys_types.h>

#    define WCOREDUMP(stat_val) 0
#    define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#    define WIFEXITED(stat_val) (((stat_val)&255) == 0)
#    define WIFSIGNALED(stat_val) 0
#    define WTERMSIG(stat_val) 0

#    ifdef __cplusplus
extern "C"
{
#    endif

    pid_t ADUCPAL_waitpid(pid_t pid, int* stat_loc, int options);

#    ifdef __cplusplus
}
#    endif

#else

#    include <wait.h>

#    define ADUCPAL_waitpid(pid, stat_loc, options) waitpid(pid, stat_loc, options)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_WAIT_H
