/* Optional shim (-Icompat): on Windows, <unistd.h> also provides fork,
 * wait, getppid, pipe, getuid, kill, pause, alarm... as on Unix. */
#ifndef UNIXLIB_COMPAT_UNISTD_H
#define UNIXLIB_COMPAT_UNISTD_H

#include_next <unistd.h>

#if defined(_WIN32) && !defined(UNIXLIB_FORK_H) && !defined(UNIXLIB_PIPES_H) \
    && !defined(UNIXLIB_SIGNALS_H) && !defined(UNIXLIB_UNIXLIB_H)
#  include <unixlib/unixlib.h>
#endif

#endif
