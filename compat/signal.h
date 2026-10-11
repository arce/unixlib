/* Shim opcional (-Icompat): en Windows, <signal.h> tambien aporta
 * sigaction, kill, SIGUSR1... */
#ifndef UNIXLIB_COMPAT_SIGNAL_H
#define UNIXLIB_COMPAT_SIGNAL_H

#include_next <signal.h>

#if defined(_WIN32) && !defined(UNIXLIB_FORK_H) && !defined(UNIXLIB_PIPES_H) \
    && !defined(UNIXLIB_SIGNALS_H) && !defined(UNIXLIB_UNIXLIB_H)
#  include <unixlib/unixlib.h>
#endif

#endif
