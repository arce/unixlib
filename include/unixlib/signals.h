#ifndef UNIXLIB_SIGNALS_H
#define UNIXLIB_SIGNALS_H

/* ================================================================
 *  unixlib/signals.h - señales POSIX sobre Windows (header-only).
 *
 *  API prefijada: usig_signal, usig_sigaction, usig_sigprocmask,
 *    usig_sigsuspend, usig_sigpending, usig_sigemptyset/fillset/addset/
 *    delset/ismember, usig_pause, usig_alarm, usig_raise, usig_kill
 *  Macros POSIX con los nombres habituales (signal, sigaction, kill,
 *  raise, pause, alarm...); desactivar con UNIXLIB_NO_POSIX_NAMES.
 *
 *  Numeros (como Linux): SIGHUP 1, SIGINT 2, SIGQUIT 3, SIGKILL 9,
 *  SIGUSR1 10, SIGUSR2 12, SIGALRM 14, SIGTERM 15, SIGCHLD 17,
 *  SIGCONT 18, SIGSTOP 19 (+ SIGILL/SIGFPE/SIGSEGV/SIGABRT del CRT).
 *
 *  Se pueden capturar con signal()/sigaction():
 *    SIGHUP SIGINT SIGQUIT SIGUSR1 SIGUSR2 SIGALRM SIGTERM SIGCHLD
 *    SIGCONT SIGABRT                  (entre procesos y con raise)
 *    SIGILL SIGFPE SIGSEGV            (los entrega el CRT)
 *  SIGKILL no se captura; SIGSTOP no esta soportada (ENOSYS).
 *  Ctrl+C / Ctrl+Break llegan como SIGINT; cerrar la consola, cerrar
 *  sesion o apagar llegan como SIGTERM.
 *
 *  Entre procesos: kill() funciona hacia hijos de fork() y hacia
 *  cualquier proceso que use unixlib y ya haya instalado un handler
 *  (o llamado a pause/sigsuspend/alarm). Si el destino no tiene buzon
 *  se aplica la accion por defecto: terminar (SIGCHLD/SIGCONT: ignorar).
 *  Una señal enviada a un hijo que aun arranca queda pendiente.
 *    kill(pid, 0)   existe el proceso?
 *    kill(-pid, s)  se aplica al proceso |pid|
 *    kill(0, s)     a todos los demas procesos con buzon unixlib
 *
 *  Diferencias con POSIX (importantes):
 *   - El handler se ejecuta en un hilo vigilante, no interrumpe el hilo
 *     principal. Entregas y handlers van de uno en uno (en serie).
 *   - La mascara (sigprocmask) es de todo el proceso.
 *   - sa_mask, SA_RESTART, SA_RESETHAND, SA_NODEFER se ignoran;
 *     SA_SIGINFO entrega siginfo_t con solo si_signo.
 *   - SIGCHLD no se genera automaticamente al terminar un hijo.
 *   - pause()/sigsuspend() no pierden una señal entregada justo antes.
 *   - Sin handler, SIGTERM/SIGKILL... terminan el proceso y wait() lo
 *     informa con WIFSIGNALED/WTERMSIG.
 *  Si activas los macros, incluye este header DESPUES de <signal.h>.
 * ================================================================ */

#include <signal.h>
#include <unixlib/sigdefs.h>

#ifdef _WIN32

#include <unixlib/fork.h>   /* windows.h, pid_t, UFORK_SIGEXIT, buzon */

typedef void (*usig_handler_t)(int);

#ifndef _POSIX
typedef _sigset_t sigset_t;           /* MinGW solo lo define con _POSIX */
#endif

typedef struct {
  int   si_signo;
  int   si_errno;
  int   si_code;
  pid_t si_pid;                       /* no disponible: 0 */
  unsigned si_uid;
  void *si_addr;
  int   si_status;
} siginfo_t;

struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  };
  sigset_t sa_mask;
  int      sa_flags;
  void   (*sa_restorer)(void);
};

#ifndef STATUS_CONTROL_C_EXIT
#  define STATUS_CONTROL_C_EXIT 0xC000013AUL
#endif

/* ---- implementacion ---- */
#define USIG__N    32
#define USIG__IGN  ((void *)(uintptr_t)1)

UNIXLIB_SHARED void *usig__h[USIG__N] = {0};          /* NULL == SIG_DFL */
UNIXLIB_SHARED int   usig__f[USIG__N] = {0};          /* sa_flags */
UNIXLIB_SHARED volatile LONG usig__blocked = 0;
UNIXLIB_SHARED volatile LONG usig__pending = 0;
UNIXLIB_SHARED HANDLE usig__wake = 0;                 /* auto-reset */
UNIXLIB_SHARED INIT_ONCE usig__once = INIT_ONCE_STATIC_INIT;
UNIXLIB_SHARED HANDLE usig__alarm_timer = 0;
UNIXLIB_SHARED ULONGLONG usig__alarm_due = 0;

static inline LONG usig__bit(int sig) { return (LONG)(1UL << sig); }

static inline int usig__valid(int sig)       /* capturable por unixlib */
{
  return sig == SIGHUP || sig == SIGINT || sig == SIGQUIT ||
         sig == SIGUSR1 || sig == SIGUSR2 || sig == SIGALRM ||
         sig == SIGTERM || sig == SIGCHLD || sig == SIGCONT ||
         sig == SIGABRT;
}

static inline int usig__crt(int sig)         /* los entrega el CRT */
{
  return sig == SIGILL || sig == SIGFPE || sig == SIGSEGV;
}

static inline void usig__default_action(int sig)
{
  if (sig == SIGCHLD || sig == SIGCONT) return;       /* ignorar */
  /* Mismo codigo que usa kill(): el padre lo ve como WIFSIGNALED. */
  ExitProcess((UINT)UFORK_SIGEXIT(sig));
}

/* Ejecuta la accion de `sig` (sin mirar la mascara). */
static inline void usig__run(int sig)
{
  void *h = usig__h[sig];
  if (h == USIG__IGN) return;
  if (h == NULL) { usig__default_action(sig); return; }
  if (usig__f[sig] & SA_SIGINFO) {
    siginfo_t si;
    memset(&si, 0, sizeof(si));
    si.si_signo = sig;
    ((void (*)(int, siginfo_t *, void *))h)(sig, &si, NULL);
  } else {
    ((usig_handler_t)h)(sig);
  }
  if (usig__wake) SetEvent(usig__wake);               /* despierta pause() */
}

static inline void usig__dispatch(int sig)
{
  if (sig < 1 || sig >= USIG__N) return;
  if (usig__blocked & usig__bit(sig)) {
    ufork__or(&usig__pending, usig__bit(sig));
    return;
  }
  usig__run(sig);
}

/* Entrega las pendientes que ya no estan bloqueadas. */
static inline void usig__flush(void)
{
  for (;;) {
    LONG p = usig__pending & ~usig__blocked;
    if (!p) return;
    int sg = 1;
    while (sg < USIG__N && !(p & usig__bit(sg))) ++sg;
    if (sg >= USIG__N) return;
    ufork__andnot(&usig__pending, usig__bit(sg));
    usig__run(sg);
  }
}

/* Hilo propio de Windows para eventos de consola. */
static inline BOOL WINAPI usig__console_handler(DWORD ev)
{
  int sig;
  switch (ev) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      sig = SIGINT;
      break;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      sig = SIGTERM;
      break;
    default:
      return FALSE;
  }
  void *h = usig__h[sig];
  if (h == USIG__IGN) return TRUE;
  if (h == NULL) return FALSE;                        /* accion por defecto */
  usig__dispatch(sig);
  return TRUE;
}

static inline BOOL CALLBACK usig__init_cb(PINIT_ONCE o, PVOID p, PVOID *c)
{
  (void)o; (void)p; (void)c;
  usig__wake = CreateEventW(NULL, FALSE, FALSE, NULL);
  ufork__deliver_hook = usig__dispatch;
  SetConsoleCtrlHandler(usig__console_handler, TRUE);
  ufork__start_sig_watcher();                         /* buzon propio */
  return TRUE;
}

static inline void usig__init(void)
{
  ufork__ensure_init();
  InitOnceExecuteOnce(&usig__once, usig__init_cb, NULL, NULL);
}

/* Si este objeto esta enlazado, el hilo vigilante de fork.h puede
 * entregar señales con handler desde el primer momento. */
__attribute__((constructor)) static void usig__ctor(void)
{
  ufork__deliver_hook = usig__dispatch;
}

/* ---------------------------------------------------------------- */
static inline int usig_sigemptyset(sigset_t *s) { *s = 0; return 0; }
static inline int usig_sigfillset(sigset_t *s)
{
  *s = (sigset_t)~(sigset_t)0 & ~(sigset_t)1;
  return 0;
}
static inline int usig_sigaddset(sigset_t *s, int sig)
{
  if (sig < 1 || sig >= USIG__N) { errno = EINVAL; return -1; }
  *s |= (sigset_t)1 << sig;
  return 0;
}
static inline int usig_sigdelset(sigset_t *s, int sig)
{
  if (sig < 1 || sig >= USIG__N) { errno = EINVAL; return -1; }
  *s &= ~((sigset_t)1 << sig);
  return 0;
}
static inline int usig_sigismember(const sigset_t *s, int sig)
{
  if (sig < 1 || sig >= USIG__N) { errno = EINVAL; return -1; }
  return (*s >> sig) & 1;
}

static inline int usig_sigaction(int sig, const struct sigaction *act,
                                 struct sigaction *old)
{
  if (sig == SIGKILL || sig == SIGSTOP) { errno = EINVAL; return -1; }

  if (usig__crt(sig)) {                 /* SIGILL/SIGFPE/SIGSEGV: del CRT */
    void (*prev)(int);
    if (act) {
      prev = signal(sig, (act->sa_flags & SA_SIGINFO) ? SIG_DFL
                                                      : act->sa_handler);
      if (prev == SIG_ERR) return -1;
    } else {
      prev = signal(sig, SIG_DFL);
      if (prev != SIG_ERR) signal(sig, prev);
    }
    if (old) { memset(old, 0, sizeof(*old)); old->sa_handler = prev; }
    return 0;
  }

  if (!usig__valid(sig)) { errno = EINVAL; return -1; }
  usig__init();

  if (old) {
    memset(old, 0, sizeof(*old));
    old->sa_handler = (void (*)(int))usig__h[sig];
    old->sa_flags   = usig__f[sig];
  }
  if (act) {
    usig__f[sig] = act->sa_flags;
    InterlockedExchangePointer((PVOID volatile *)&usig__h[sig],
                               (PVOID)act->sa_handler);
    /* abort() del CRT tambien debe llegar al handler de SIGABRT. */
    if (sig == SIGABRT)
      signal(SIGABRT, (act->sa_flags & SA_SIGINFO) ? SIG_DFL
                                                   : act->sa_handler);
  }
  return 0;
}

static inline usig_handler_t usig_signal(int sig, usig_handler_t handler)
{
  struct sigaction a, o;
  memset(&a, 0, sizeof(a));
  a.sa_handler = handler;
  if (usig_sigaction(sig, &a, &o) != 0) return SIG_ERR;
  return o.sa_handler;                  /* NULL == SIG_DFL */
}

static inline int usig_sigprocmask(int how, const sigset_t *set, sigset_t *old)
{
  usig__init();
  LONG cur = usig__blocked;
  if (old) *old = (sigset_t)cur;
  if (set) {
    LONG m = (LONG)*set & ~usig__bit(SIGKILL) & ~usig__bit(SIGSTOP);
    LONG nw;
    switch (how) {
      case SIG_BLOCK:   nw = cur | m;  break;
      case SIG_UNBLOCK: nw = cur & ~m; break;
      case SIG_SETMASK: nw = m;        break;
      default: errno = EINVAL; return -1;
    }
    InterlockedExchange(&usig__blocked, nw);
    usig__flush();
  }
  return 0;
}

static inline int usig_sigpending(sigset_t *set)
{
  *set = (sigset_t)(usig__pending & usig__blocked);
  return 0;
}

static inline int usig_pause(void)
{
  usig__init();
  WaitForSingleObject(usig__wake, INFINITE);
  errno = EINTR;
  return -1;
}

static inline int usig_sigsuspend(const sigset_t *mask)
{
  usig__init();
  LONG old = usig__blocked;
  InterlockedExchange(&usig__blocked, (LONG)*mask);
  usig__flush();
  WaitForSingleObject(usig__wake, INFINITE);
  InterlockedExchange(&usig__blocked, old);
  usig__flush();
  errno = EINTR;
  return -1;
}

static inline VOID CALLBACK usig__alarm_cb(PVOID p, BOOLEAN f)
{
  (void)p; (void)f;
  usig__alarm_due = 0;
  usig__dispatch(SIGALRM);
}

/* Devuelve los segundos que faltaban de la alarma anterior. */
static inline unsigned usig_alarm(unsigned seconds)
{
  usig__init();
  unsigned remaining = 0;
  ULONGLONG now = GetTickCount64();
  if (usig__alarm_timer) {
    if (usig__alarm_due > now)
      remaining = (unsigned)((usig__alarm_due - now + 999) / 1000);
    DeleteTimerQueueTimer(NULL, usig__alarm_timer, NULL);
    usig__alarm_timer = NULL;
    usig__alarm_due = 0;
  }
  if (seconds) {
    usig__alarm_due = now + (ULONGLONG)seconds * 1000;
    if (!CreateTimerQueueTimer(&usig__alarm_timer, NULL, usig__alarm_cb, NULL,
                               (DWORD)(seconds * 1000), 0, WT_EXECUTEDEFAULT))
      usig__alarm_timer = NULL;
  }
  return remaining;
}

/* ---------------------------------------------------------------- */
static inline int usig_raise(int sig)
{
  if (usig__crt(sig)) return raise(sig);                /* raise del CRT */
  if (sig == SIGKILL) usig__default_action(sig);        /* no capturable */
  if (!usig__valid(sig)) { errno = EINVAL; return -1; }
  usig__init();
  usig__dispatch(sig);                                  /* sincrono */
  return 0;
}

static inline int usig__terminate(pid_t pid, int sig)
{
  HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
  if (!h) { errno = GetLastError() == ERROR_ACCESS_DENIED ? EPERM : ESRCH;
            return -1; }
  BOOL ok = TerminateProcess(h, (UINT)UFORK_SIGEXIT(sig));
  CloseHandle(h);
  if (!ok) { errno = EPERM; return -1; }
  return 0;
}

static inline int usig_kill(pid_t pid, int sig)
{
  if (sig < 0 || sig >= USIG__N) { errno = EINVAL; return -1; }

  if (pid == 0) {                       /* a todos los procesos con buzon */
    if (sig == 0) return 0;
    if (!usig__valid(sig)) { errno = EINVAL; return -1; }
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (s == INVALID_HANDLE_VALUE) { errno = EPERM; return -1; }
    PROCESSENTRY32W e;
    e.dwSize = sizeof(e);
    DWORD me = GetCurrentProcessId();
    for (BOOL ok = Process32FirstW(s, &e); ok; ok = Process32NextW(s, &e))
      if (e.th32ProcessID != me && e.th32ProcessID != 0)
        ufork__send_signal((pid_t)e.th32ProcessID, sig);
    CloseHandle(s);
    return 0;
  }
  if (pid < 0) pid = -pid;              /* kill(-pid, ...): grupo del hijo */

  if (sig == 0) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                           (DWORD)pid);
    if (!h) { errno = GetLastError() == ERROR_ACCESS_DENIED ? EPERM : ESRCH;
              return -1; }
    DWORD code = 0;
    BOOL alive = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    CloseHandle(h);
    if (!alive) { errno = ESRCH; return -1; }
    return 0;
  }

  if (sig == SIGSTOP) { errno = ENOSYS; return -1; }
  if (sig != SIGKILL && !usig__valid(sig) && !usig__crt(sig)) {
    errno = EINVAL;
    return -1;
  }
  if (pid == (pid_t)GetCurrentProcessId()) return usig_raise(sig);

  if (sig == SIGKILL || usig__crt(sig)) return usig__terminate(pid, sig);
  if (ufork__send_signal(pid, sig)) return 0;

  /* Destino sin buzon: accion por defecto. */
  if (sig == SIGCHLD || sig == SIGCONT) {
    return usig_kill(pid, 0);
  }
  return usig__terminate(pid, sig);
}

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define signal(s, h)          usig_signal((s), (h))
#  define raise(s)              usig_raise(s)
#  define kill(p, s)            usig_kill((p), (s))
#  define sigaction(s, a, o)    usig_sigaction((s), (a), (o))
#  define sigprocmask(h, s, o)  usig_sigprocmask((h), (s), (o))
#  define sigsuspend(m)         usig_sigsuspend(m)
#  define sigpending(s)         usig_sigpending(s)
#  define sigemptyset(s)        usig_sigemptyset(s)
#  define sigfillset(s)         usig_sigfillset(s)
#  define sigaddset(s, n)       usig_sigaddset((s), (n))
#  define sigdelset(s, n)       usig_sigdelset((s), (n))
#  define sigismember(s, n)     usig_sigismember((s), (n))
#  define pause()               usig_pause()
#  define alarm(s)              usig_alarm(s)
#endif

#else  /* ================= UNIX ================= */

#include <unistd.h>
#include <sys/types.h>

typedef void (*usig_handler_t)(int);
#define usig_signal      signal
#define usig_sigaction   sigaction
#define usig_sigprocmask sigprocmask
#define usig_sigsuspend  sigsuspend
#define usig_sigpending  sigpending
#define usig_pause       pause
#define usig_alarm       alarm
#define usig_raise       raise
#define usig_kill        kill

#endif /* _WIN32 */

#endif /* UNIXLIB_SIGNALS_H */
