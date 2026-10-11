#ifndef UNIXLIB_SIGDEFS_H
#define UNIXLIB_SIGDEFS_H

/* Signal constants that MinGW does not define (Linux numbering).
 * They do not clash with the Windows CRT ones (2,4,8,11,15,21,22). */

#ifdef _WIN32

#include <signal.h>

#ifndef SIGHUP
#  define SIGHUP   1
#endif
#ifndef SIGQUIT
#  define SIGQUIT  3
#endif
#ifndef SIGKILL
#  define SIGKILL  9
#endif
#ifndef SIGUSR1
#  define SIGUSR1  10
#endif
#ifndef SIGUSR2
#  define SIGUSR2  12
#endif
#ifndef SIGALRM
#  define SIGALRM  14
#endif
#ifndef SIGCHLD
#  define SIGCHLD  17
#endif
#ifndef SIGCONT
#  define SIGCONT  18
#endif
#ifndef SIGSTOP
#  define SIGSTOP  19
#endif

#ifndef SIG_BLOCK
#  define SIG_BLOCK    0
#  define SIG_UNBLOCK  1
#  define SIG_SETMASK  2
#endif

/* sa_flags (own values; only SA_SIGINFO has any effect) */
#ifndef SA_SIGINFO
#  define SA_NOCLDSTOP 0x01
#  define SA_NOCLDWAIT 0x02
#  define SA_SIGINFO   0x04
#  define SA_RESTART   0x10
#  define SA_NODEFER   0x20
#  define SA_RESETHAND 0x40
#endif

#endif /* _WIN32 */

#endif
