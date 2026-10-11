/* Shim opcional (-Icompat): <sys/shm.h> en Windows (System V IPC). */
#ifndef UNIXLIB_COMPAT_SYS_SHM_H
#define UNIXLIB_COMPAT_SYS_SHM_H

#ifdef _WIN32
#  include <unixlib/shm.h>
#else
#  include_next <sys/shm.h>
#endif

#endif
