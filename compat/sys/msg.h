/* Optional shim (-Icompat): <sys/msg.h> on Windows (System V IPC). */
#ifndef UNIXLIB_COMPAT_SYS_MSG_H
#define UNIXLIB_COMPAT_SYS_MSG_H

#ifdef _WIN32
#  include <unixlib/msg.h>
#else
#  include_next <sys/msg.h>
#endif

#endif
