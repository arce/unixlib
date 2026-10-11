# unixlib

`fork`, `wait/waitpid`, `getppid`, `getuid`, `pipe`, signals (`signal`, `sigaction`,
`kill`, `raise`, `pause`, `alarm`, `sigprocmask`...) and **System V IPC** (shared memory,
semaphores and message queues) for Windows with **TDM-GCC / MinGW-w64** (>= 9.2, Windows 7+). **Header-only.** On Unix the headers forward to POSIX.

## Usage

```
gcc -Iinclude -Icompat prog.c -o prog.exe      # TDM-GCC
mingw32-make test                              # builds and runs the tests
```

- `-Iinclude`: `#include <unixlib/unixlib.h>` (or `fork.h`, `pipes.h`, `signals.h`, `shm.h`, `sem.h`, `msg.h`).
- `-Icompat` (optional): makes Unix code compile **unmodified**:
  `<unistd.h>`, `<signal.h>`, `<sys/wait.h>`, `<sys/shm.h>`, `<sys/sem.h>`,
  `<sys/msg.h>` and `<sys/ipc.h>` also provide `fork`, `pipe`, `wait`,
  `sigaction`, `kill`, `pause`, `alarm`... (`tests/tutorial/` checks this).
- POSIX names are macros; `-DUNIXLIB_NO_POSIX_NAMES` leaves only `ufork_*`,
  `upipe_*`, `usig_*` (and `ushm_*`, `usem_*`, `umsg_*`).
- The internal state uses `__declspec(selectany)`: a single instance even if you include the
  headers from several `.c` files (`tests/test_multi_*.c`).
- `tests/tutorial/` holds the code of the course tutorials verbatim (it is in Spanish on purpose).

## How fork() works (important)

The child is the same .exe launched again with the same command line, and it **re-executes
main() from the beginning**. Its fork() returns 0; until it gets there, earlier fork() calls return a fake pid, pipe() returns the inherited pipes and stdout/stderr are silenced (`ufork_replaying()`). Consequences: the code before fork() must be deterministic and free of external effects (files, network, user input, rand without a fixed seed); variables are not copied; `getpid()` before the fork gives the child's pid. Nested fork() (the child forks) works with the same mechanism.

## Pipes

Real CRT fds: `read/write/close/dup/dup2` work without wrappers (also `dup2(fd[1], STDOUT_FILENO)`), and `read()` returns 0 at EOF. Only the pipes created with `pipe()` and still open are passed to the child; files and sockets are not. Close the ends you do not use in each process, or the reader will not see EOF.

## Processes

- wait status: `WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG`, `WNOHANG`.
- Orphans: the child outlives the parent and `getppid()` returns 1.
  `-DUFORK_KILL_ORPHANS`: children die with the parent (Job Object).
- Zombies: they do not exist on Windows; `wait()` works but `ps` does not show them.
- `getuid()/geteuid()/getgid()` return 1000 (fixed value).
- At most 64 simultaneous children per process, 128 pipes.

## Signals

Numbers as on Linux: SIGHUP 1, SIGINT 2, SIGQUIT 3, SIGKILL 9, SIGUSR1 10, SIGUSR2 12,
SIGALRM 14, SIGTERM 15, SIGCHLD 17, SIGCONT 18. Catchable with `signal/sigaction`: those
(except SIGKILL), SIGABRT, and SIGILL/SIGFPE/SIGSEGV through the CRT. Ctrl+C/Break = SIGINT;
closing the console/logoff/shutdown = SIGTERM.

`kill(pid, sig)` works towards children of fork() and towards any process that uses unixlib and has already installed a handler (or called `pause/sigsuspend/alarm`). Without a mailbox the default action applies (terminate; SIGCHLD/SIGCONT are ignored). A signal sent to a child that is still starting up stays pending (it is delivered with a 100 ms margin, `-DUFORK_TERM_GRACE_MS=n`). `kill(-pid, s)` acts on `pid`; `kill(0, s)` on all the other processes with a unixlib mailbox.

Differences from POSIX: the handler runs in a watcher thread (it does not interrupt the main one) and deliveries are serial; the mask is process-wide; `sa_mask`, `SA_RESTART`, `SA_RESETHAND`, `SA_NODEFER` are ignored; `siginfo_t` only carries `si_signo`; SIGCHLD is not generated on its own; SIGSTOP is not supported; without a handler, SIGTERM/SIGUSR1... terminate the process and `wait()` reports it as a signal.

## System V IPC (shm.h, sem.h, msg.h)

`shmget/shmat/shmdt/shmctl`, `semget/semop/semctl`, `msgget/msgsnd/msgrcv/msgctl`, `ftok`, `getpagesize`, `key_t`, `IPC_CREAT/IPC_EXCL/IPC_NOWAIT/IPC_RMID/IPC_STAT`, `SETVAL`, `struct sembuf`, `MSG_NOERROR/MSG_EXCEPT`. Every object is a *file mapping* whose name is derived from the key (`Local\unixlib_shm_k000003e7`...), so processes with the same key (even unrelated ones) see the same object. A named mutex and event block and wake up the processes in `semop`/`msgsnd`/`msgrcv`.

- `union semun` is **not** defined by the library (like glibc): your program defines it.
- After `IPC_RMID`, the same key creates a new, zeroed object even if another process still
  keeps the old one open.
- `IPC_PRIVATE` works; children find the object again through fork()'s replay.
- Queues: 16384 bytes of data, maximum message 8192 (as on Linux). `msgsz` does not include the `long type`, which is 4 bytes on Windows.

**fork() replay and IPC.** The child repeats the calls made before the fork. So that they do not
destroy the shared state, while `ufork_replaying()` is true: `shmget/semget/msgget` open the existing object; `semop`, `msgsnd`, `msgrcv`, `semctl(SETVAL/SETALL/IPC_RMID)`, `shmctl(IPC_RMID)` and `msgctl(IPC_RMID)` do nothing; `shmat` gives a **private** (copy-on-write) view, so repeated `*shm = 0;` or
`sprintf(shm, ...)` do not touch the real segment. When the replay ends that view is replaced, **at the same address**, by the shared one. Limits: a `msgrcv` made before the fork is not reproduced (the child does not get that data); up to 64 live objects per type and 128 views per process; IPC_PRIVATE is only inherited one level (direct child).

Differences from Unix: `SEM_UNDO` is ignored; a blocked `semop/msgrcv` is not interrupted by signals (no `EINTR`); `shmat` returns addresses aligned to 128 bytes, not to a page; `shm_nattch` is approximate (a process that dies without `shmdt` does not decrement it); there are no permissions and no `ipcs/ipcrm`; objects disappear when the last process that has them open ends.
