/* Shim opcional (-Icompat): <sys/ipc.h> en Windows (System V IPC). */
#ifndef UNIXLIB_COMPAT_SYS_IPC_H
#define UNIXLIB_COMPAT_SYS_IPC_H

#ifdef _WIN32
#  include <unixlib/ipc.h>
#else
#  include_next <sys/ipc.h>
#endif

#endif
