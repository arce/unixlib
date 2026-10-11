/* Optional shim (-Icompat): <sys/wait.h> on Windows. */
#ifndef UNIXLIB_COMPAT_SYS_WAIT_H
#define UNIXLIB_COMPAT_SYS_WAIT_H

#ifdef _WIN32
#  if !defined(UNIXLIB_FORK_H) && !defined(UNIXLIB_PIPES_H) \
      && !defined(UNIXLIB_SIGNALS_H) && !defined(UNIXLIB_UNIXLIB_H)
#    include <unixlib/unixlib.h>
#  endif
#else
#  include_next <sys/wait.h>
#endif

#endif
