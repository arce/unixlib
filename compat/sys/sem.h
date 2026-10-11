/* Optional shim (-Icompat): <sys/sem.h> on Windows (System V IPC). */
#ifndef UNIXLIB_COMPAT_SYS_SEM_H
#define UNIXLIB_COMPAT_SYS_SEM_H

#ifdef _WIN32
#  include <unixlib/sem.h>
#else
#  include_next <sys/sem.h>
#endif

#endif
