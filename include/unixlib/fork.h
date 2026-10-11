#ifndef UNIXLIB_FORK_H
#define UNIXLIB_FORK_H

/* ================================================================
 *  unixlib/fork.h - fork/wait/waitpid/getppid sobre Windows.
 *
 *  Modelo ("re-ejecución determinista"):
 *    Windows no puede clonar un proceso. fork() lanza de nuevo el
 *    mismo .exe con la misma línea de comandos; el hijo re-ejecuta
 *    main() desde el principio y la llamada fork() correspondiente
 *    devuelve 0 en él. Mientras el hijo "se pone al día":
 *      - las llamadas a fork() anteriores devuelven un pid ficticio,
 *      - las llamadas a pipe() devuelven los pipes heredados,
 *      - stdout/stderr se silencian (ver ufork_replaying()).
 *    Por tanto el código previo al fork() debe ser determinista y
 *    sin efectos secundarios externos (archivos, red, rand sin
 *    semilla fija...). Las variables NO se copian.
 *
 *  Header-only: no hay que enlazar nada. Todas las funciones son
 *  static inline y el estado compartido usa UNIXLIB_SHARED (selectany).
 *
 *  API prefijada: ufork_fork, ufork_wait, ufork_waitpid,
 *                 ufork_getppid, ufork_replaying
 *  Nombres POSIX (fork(), wait(s), waitpid(p,s,o), getppid()) se
 *  activan como macros salvo que definas UNIXLIB_NO_POSIX_NAMES.
 *  Tambien: getuid()/geteuid()/getgid() (valor fijo 1000: Windows no
 *  tiene uid). getpid() y _exit() son los del CRT de MinGW.
 *
 *  Huerfanos: como en POSIX, un hijo sobrevive a su padre y su getppid()
 *  pasa a devolver 1. Compila con -DUFORK_KILL_ORPHANS para que los hijos
 *  mueren con el padre (Job Object).
 *
 *  Compilador objetivo: TDM-GCC / MinGW-w64 (>= 9.2, Windows Vista+).
 * ================================================================ */

#ifdef _WIN32

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601      /* Windows 7: listas de atributos, SRWLOCK */
#endif
#ifndef WINVER
#  define WINVER _WIN32_WINNT
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>
#include <process.h>      /* getpid() */
#include <sys/types.h>    /* pid_t */

#ifndef UNIXLIB_SHARED
   /* Estado compartido entre unidades de traduccion (header-only):
    * COMDAT/selectany -> una sola instancia en el ejecutable. */
#  define UNIXLIB_SHARED __declspec(selectany)
#endif

#ifndef WNOHANG
#  define WNOHANG 1
#endif
/* Estado compatible con POSIX: (codigo << 8) o numero de señal. */
#ifndef WIFEXITED
#  define WIFEXITED(s)    (((s) & 0x7f) == 0)
#  define WEXITSTATUS(s)  (((s) >> 8) & 0xff)
#  define WIFSIGNALED(s)  (((s) & 0x7f) != 0)
#  define WTERMSIG(s)     ((s) & 0x7f)
#endif

/* Codigo de salida con el que se representa "muerto por la señal N". */
#define UFORK_SIGEXIT_BASE 0xDEAD0000UL
#define UFORK_SIGEXIT(sig) (UFORK_SIGEXIT_BASE | (unsigned long)(sig))

#include <unixlib/sigdefs.h>
#include <unixlib/pipes.h>   /* registro de pipes heredables */

/* ---- implementacion ---- */
/* Huerfanos: por defecto sobreviven al padre (como en POSIX).
 * -DUFORK_KILL_ORPHANS los mata con el padre (Job Object).
 * -DUFORK_NO_QUIET_REPLAY: no silencia stdout/stderr durante el replay.
 * -DUFORK_TERM_GRACE_MS=n: margen (def. 100 ms) antes de entregar un
 *  SIGTERM llegado mientras el hijo aun arrancaba, para que main()
 *  alcance a instalar su handler. */
#ifndef UFORK_TERM_GRACE_MS
#  define UFORK_TERM_GRACE_MS 100
#endif

#define UFORK__MAX_KIDS   64              /* limite de WaitForMultipleObjects */
#define UFORK__MAX_PATH_N 32              /* profundidad maxima de forks anidados */
#define UFORK__FAKE_PID   ((pid_t)0x7fffffff)

#ifndef STATUS_CONTROL_C_EXIT
#  define STATUS_CONTROL_C_EXIT 0xC000013AUL
#endif

/* term/map/pend: buzon de señales del hijo, creado por el padre. */
typedef struct { pid_t pid; HANDLE h; HANDLE term; HANDLE map;
                 volatile LONG *pend; } ufork__kid_t;

UNIXLIB_SHARED SRWLOCK ufork__lock = SRWLOCK_INIT;
UNIXLIB_SHARED ufork__kid_t ufork__kids[UFORK__MAX_KIDS] = {{0}};
UNIXLIB_SHARED int ufork__nkids = 0;
UNIXLIB_SHARED HANDLE ufork__job = 0;

UNIXLIB_SHARED INIT_ONCE ufork__once = INIT_ONCE_STATIC_INIT;
UNIXLIB_SHARED int ufork__is_child = 0;
UNIXLIB_SHARED int ufork__replaying = 0;
UNIXLIB_SHARED pid_t ufork__ppid = 0;
UNIXLIB_SHARED int ufork__path[UFORK__MAX_PATH_N] = {0}; /* nº de llamada fork() que devuelve 0 */
UNIXLIB_SHARED int ufork__npath = 0;
UNIXLIB_SHARED int ufork__calls = 0; /* llamadas a fork() hasta ahora */
UNIXLIB_SHARED int ufork__saved_out = -1;
UNIXLIB_SHARED int ufork__saved_err = -1;
UNIXLIB_SHARED HANDLE ufork__parent_h = 0;     /* handle del padre (hijo) */
UNIXLIB_SHARED HANDLE ufork__sig_ev = 0;       /* buzon propio: evento */
UNIXLIB_SHARED volatile LONG *ufork__sig_pend = 0;  /* bits pendientes */
UNIXLIB_SHARED LONG   ufork__watcher_started = 0;
/* signals.h lo fija; recibe cada señal desde el hilo vigilante. */
typedef void (*ufork__deliver_fn)(int);
UNIXLIB_SHARED ufork__deliver_fn ufork__deliver_hook = 0;
/* Gancho opcional: se invoca en el hijo cuando termina el replay
 * (la ultima llamada a fork() devuelve 0). Lo usa shm.h. */
UNIXLIB_SHARED void (*ufork__replay_end_hook)(void) = 0;

/* ---------------------------------------------------------------- */
static inline void ufork__quiet_begin(void)
{
#ifndef UFORK_NO_QUIET_REPLAY
  fflush(NULL);
  ufork__saved_out = _dup(1);
  ufork__saved_err = _dup(2);
  int nul = _open("NUL", _O_WRONLY | _O_BINARY | _O_NOINHERIT);
  if (nul >= 0) {
    if (ufork__saved_out >= 0) _dup2(nul, 1);
    if (ufork__saved_err >= 0) _dup2(nul, 2);
    _close(nul);
  }
#endif
}

static inline void ufork__quiet_end(void)
{
#ifndef UFORK_NO_QUIET_REPLAY
  fflush(NULL);                       /* lo pendiente del replay va a NUL */
  if (ufork__saved_out >= 0) { _dup2(ufork__saved_out, 1); _close(ufork__saved_out); }
  if (ufork__saved_err >= 0) { _dup2(ufork__saved_err, 2); _close(ufork__saved_err); }
  ufork__saved_out = ufork__saved_err = -1;
#endif
}

static inline WCHAR *ufork__get_env(const WCHAR *name)
{
  DWORD n = GetEnvironmentVariableW(name, NULL, 0);
  if (n == 0) return NULL;
  WCHAR *v = (WCHAR *)malloc(n * sizeof(WCHAR));
  if (v && GetEnvironmentVariableW(name, v, n) == 0) { free(v); v = NULL; }
  return v;
}

/* "ppid:i1,i2,..." */
static inline void ufork__parse_child_env(const WCHAR *v)
{
  WCHAR *end;
  ufork__ppid = (pid_t)wcstol(v, &end, 10);
  while (*end == L':' || *end == L',') {
    long n = wcstol(end + 1, &end, 10);
    if (n > 0 && ufork__npath < UFORK__MAX_PATH_N) ufork__path[ufork__npath++] = (int)n;
  }
}

/* ---------------------------------------------------------------- */
/* Buzon de señales entre procesos de unixlib. Cada proceso que puede
 * recibir señales tiene
 *    evento "Local\\unixlib_sev_<pid>"  (manual-reset)
 *    mapa   "Local\\unixlib_spd_<pid>"  (DWORD de bits pendientes)
 * Quien envia activa el bit y el evento; un hilo vigilante del
 * destinatario entrega la señal (handler o accion por defecto).
 * El padre crea el buzon del hijo ANTES de reanudarlo, asi que una señal
 * enviada mientras el hijo arranca queda pendiente y no se pierde. */
static inline void ufork__sig_names(WCHAR *ev, WCHAR *map, DWORD pid)
{
  if (ev)  { _snwprintf(ev, 64, L"Local\\unixlib_sev_%lu", (unsigned long)pid);
             ev[63] = L'\0'; }
  if (map) { _snwprintf(map, 64, L"Local\\unixlib_spd_%lu", (unsigned long)pid);
             map[63] = L'\0'; }
}

static inline void ufork__or(volatile LONG *p, LONG bit)
{
  LONG o;
  do { o = *p; } while (InterlockedCompareExchange(p, o | bit, o) != o);
}

static inline void ufork__andnot(volatile LONG *p, LONG bit)
{
  LONG o;
  do { o = *p; } while (InterlockedCompareExchange(p, o & ~bit, o) != o);
}

static inline DWORD WINAPI ufork__sig_thread(LPVOID arg)
{
  (void)arg;
  for (;;) {
    if (WaitForSingleObject(ufork__sig_ev, INFINITE) != WAIT_OBJECT_0)
      return 0;
    ResetEvent(ufork__sig_ev);
    if (ufork__replaying) {            /* llego durante el arranque: pendiente */
      while (ufork__replaying) Sleep(5);
      Sleep(UFORK_TERM_GRACE_MS);
    }
    LONG mask = InterlockedExchange(ufork__sig_pend, 0);
    for (int sg = 1; sg < 32; ++sg) {
      if (!(mask & (LONG)(1UL << sg))) continue;
      if (ufork__deliver_hook) ufork__deliver_hook(sg);
      else if (sg != SIGCHLD && sg != SIGCONT)
        ExitProcess((UINT)UFORK_SIGEXIT(sg));
    }
  }
}

static inline void ufork__start_sig_watcher(void)
{
  if (InterlockedCompareExchange(&ufork__watcher_started, 1, 0) != 0) return;
  WCHAR en[64], mn[64];
  ufork__sig_names(en, mn, GetCurrentProcessId());
  HANDLE m = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                0, sizeof(LONG), mn);
  if (!m) return;
  void *v = MapViewOfFile(m, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LONG));
  if (!v) { CloseHandle(m); return; }
  ufork__sig_pend = (volatile LONG *)v;
  ufork__sig_ev = CreateEventW(NULL, TRUE, FALSE, en);
  if (!ufork__sig_ev) return;
  HANDLE t = CreateThread(NULL, 0, ufork__sig_thread, NULL, 0, NULL);
  if (t) CloseHandle(t);
}

/* Deja `sig` en el buzon de `pid`. 1 = entregada; 0 = pid no tiene buzon. */
static inline int ufork__send_signal(pid_t pid, int sig)
{
  LONG bit = (LONG)(1UL << sig);
  int found = 0;
  AcquireSRWLockShared(&ufork__lock);
  for (int i = 0; i < ufork__nkids; ++i)
    if (ufork__kids[i].pid == pid && ufork__kids[i].term &&
        ufork__kids[i].pend) {
      ufork__or(ufork__kids[i].pend, bit);
      SetEvent(ufork__kids[i].term);
      found = 1;
      break;
    }
  ReleaseSRWLockShared(&ufork__lock);
  if (found) return 1;

  WCHAR en[64], mn[64];
  ufork__sig_names(en, mn, (DWORD)pid);
  HANDLE ev = OpenEventW(EVENT_MODIFY_STATE, FALSE, en);
  if (!ev) return 0;
  HANDLE m = OpenFileMappingW(FILE_MAP_WRITE, FALSE, mn);
  if (!m) { CloseHandle(ev); return 0; }
  volatile LONG *p = (volatile LONG *)MapViewOfFile(m, FILE_MAP_WRITE, 0, 0,
                                                    sizeof(LONG));
  int ok = 0;
  if (p) {
    ufork__or(p, bit);
    UnmapViewOfFile((void *)p);
    SetEvent(ev);
    ok = 1;
  }
  CloseHandle(m);
  CloseHandle(ev);
  return ok;
}

static inline BOOL CALLBACK ufork__init_cb(PINIT_ONCE o, PVOID p, PVOID *c)
{
  (void)o; (void)p; (void)c;
  WCHAR *cv = ufork__get_env(L"UFORK_CHILD");
  if (cv) {
    ufork__parse_child_env(cv);
    free(cv);
    SetEnvironmentVariableW(L"UFORK_CHILD", NULL);
    if (ufork__npath > 0) {
      ufork__is_child  = 1;
      ufork__replaying = 1;
      /* CREATE_NEW_PROCESS_GROUP desactiva Ctrl+C: reactivarlo. */
      SetConsoleCtrlHandler(NULL, FALSE);
      ufork__parent_h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)ufork__ppid);
      ufork__quiet_begin();
      ufork__start_sig_watcher();
      WCHAR *pv = ufork__get_env(L"UFORK_PIPES");
      if (pv) {
        int n = WideCharToMultiByte(CP_UTF8, 0, pv, -1, NULL, 0, NULL, NULL);
        char *s = n > 0 ? (char *)malloc((size_t)n) : NULL;
        if (s) {
          WideCharToMultiByte(CP_UTF8, 0, pv, -1, s, n, NULL, NULL);
          upipe__import(s);
          free(s);
        }
        free(pv);
      }
    }
  }
  SetEnvironmentVariableW(L"UFORK_PIPES", NULL);
  return TRUE;
}

static inline void ufork__ensure_init(void)
{
  InitOnceExecuteOnce(&ufork__once, ufork__init_cb, NULL, NULL);
}

/* Se ejecuta antes de main() si este objeto esta enlazado. */
__attribute__((constructor)) static void ufork__ctor(void)
{
  ufork__ensure_init();
}

/* ---------------------------------------------------------------- */
static inline int ufork__is_child_pid(pid_t pid)
{
  int found = 0;
  AcquireSRWLockShared(&ufork__lock);
  for (int i = 0; i < ufork__nkids; ++i)
    if (ufork__kids[i].pid == pid) { found = 1; break; }
  ReleaseSRWLockShared(&ufork__lock);
  return found;
}

static inline int ufork_replaying(void)
{
  ufork__ensure_init();
  return ufork__replaying;
}

static inline pid_t ufork_getppid(void)
{
  ufork__ensure_init();
  if (ufork__is_child) {
    /* Padre muerto -> huerfano: adoptado por "init" (pid 1), como en Unix. */
    if (!ufork__parent_h ||
        WaitForSingleObject(ufork__parent_h, 0) == WAIT_OBJECT_0)
      return 1;
    return ufork__ppid;
  }

  pid_t ppid = 0;
  HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (s != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32W e;
    e.dwSize = sizeof(e);
    DWORD me = GetCurrentProcessId();
    for (BOOL ok = Process32FirstW(s, &e); ok; ok = Process32NextW(s, &e))
      if (e.th32ProcessID == me) { ppid = (pid_t)e.th32ParentProcessID; break; }
    CloseHandle(s);
  }
  return ppid;
}

/* ---------------------------------------------------------------- */
static inline int ufork__ensure_job(void)
{
#ifndef UFORK_KILL_ORPHANS
  return 0;
#else
  if (ufork__job) return 1;
  HANDLE j = CreateJobObjectW(NULL, NULL);
  if (!j) return 0;
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
  ZeroMemory(&li, sizeof(li));
  li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(j, JobObjectExtendedLimitInformation,
                               &li, sizeof(li))) {
    CloseHandle(j);
    return 0;
  }
  ufork__job = j;
  return 1;
#endif
}

/* Asigna el hijo al job; en Windows < 8 puede fallar si el padre ya esta
 * en un job: en ese caso se sigue sin la politica de huerfanos. */
static inline void ufork__job_add(HANDLE proc)
{
#ifdef UFORK_KILL_ORPHANS
  if (ufork__ensure_job()) AssignProcessToJobObject(ufork__job, proc);
#else
  (void)proc;
#endif
}


static inline int ufork__std_is_console(HANDLE h)
{
  return h == NULL || h == INVALID_HANDLE_VALUE ||
         GetFileType(h) == FILE_TYPE_CHAR;
}

static inline void ufork__set_env_utf8(const WCHAR *name, const char *v)
{
  if (!v || !*v) { SetEnvironmentVariableW(name, NULL); return; }
  int n = MultiByteToWideChar(CP_UTF8, 0, v, -1, NULL, 0);
  WCHAR *w = n > 0 ? (WCHAR *)malloc((size_t)n * sizeof(WCHAR)) : NULL;
  if (!w) return;
  MultiByteToWideChar(CP_UTF8, 0, v, -1, w, n);
  SetEnvironmentVariableW(name, w);
  free(w);
}

/* Lanza el hijo cuyo fork() es la llamada numero `call`.
 * Devuelve su pid o -1 (errno). Con ufork__lock tomado. */
static inline pid_t ufork__do_spawn(int call)
{
  pid_t result = -1;
  WCHAR *exe = (WCHAR *)malloc(32768 * sizeof(WCHAR));
  WCHAR *cmd = _wcsdup(GetCommandLineW());
  if (!exe || !cmd) { errno = ENOMEM; goto out_free; }
  if (GetModuleFileNameW(NULL, exe, 32768) == 0) { errno = EINVAL; goto out_free; }

  /* --- handles a heredar: pipes abiertos + stdio redirigido --- */
  HANDLE hs[UPIPE_MAX_HANDLES + 3];
  int nh = 0;
  char *spec = upipe__prepare(hs, &nh);       /* toma el lock de pipes */

  HANDLE sh[3] = { GetStdHandle(STD_INPUT_HANDLE),
                   GetStdHandle(STD_OUTPUT_HANDLE),
                   GetStdHandle(STD_ERROR_HANDLE) };
  DWORD  sfl[3] = { 0, 0, 0 };
  int    smark[3] = { 0, 0, 0 };
  int    redirected = !(ufork__std_is_console(sh[0]) && ufork__std_is_console(sh[1]) &&
                        ufork__std_is_console(sh[2]));
  if (redirected) {
    for (int i = 0; i < 3; ++i) {
      if (sh[i] == NULL || sh[i] == INVALID_HANDLE_VALUE) continue;
      if (ufork__std_is_console(sh[i])) continue;    /* consola: la hereda sola */
      if (!GetHandleInformation(sh[i], &sfl[i])) continue;
      if (SetHandleInformation(sh[i], HANDLE_FLAG_INHERIT,
                               HANDLE_FLAG_INHERIT)) {
        smark[i] = 1;
        int dup = 0;
        for (int k = 0; k < nh; ++k) if (hs[k] == sh[i]) dup = 1;
        if (!dup) hs[nh++] = sh[i];
      }
    }
  }

  /* --- entorno que el hijo lee en ufork__init_cb() --- */
  {
    WCHAR path[32 * 12 + 32];
    int off = _snwprintf(path, 32, L"%lu", GetCurrentProcessId());
    for (int i = 0; i < ufork__npath && i < UFORK__MAX_PATH_N; ++i)
      off += _snwprintf(path + off, 16, i == 0 ? L":%d" : L",%d", ufork__path[i]);
    off += _snwprintf(path + off, 16, ufork__npath == 0 ? L":%d" : L",%d", call);
    path[off] = L'\0';
    SetEnvironmentVariableW(L"UFORK_CHILD", path);
    ufork__set_env_utf8(L"UFORK_PIPES", spec);
  }

  /* --- lista de atributos con los handles permitidos --- */
  STARTUPINFOEXW si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.StartupInfo.cb = sizeof(si);
  if (redirected) {
    si.StartupInfo.dwFlags   = STARTF_USESTDHANDLES;
    si.StartupInfo.hStdInput  = sh[0];
    si.StartupInfo.hStdOutput = sh[1];
    si.StartupInfo.hStdError  = sh[2];
  }

  LPPROC_THREAD_ATTRIBUTE_LIST al = NULL;
  SIZE_T alsz = 0;
  if (nh > 0) {
    InitializeProcThreadAttributeList(NULL, 1, 0, &alsz);
    al = (LPPROC_THREAD_ATTRIBUTE_LIST)malloc(alsz);
    if (al && InitializeProcThreadAttributeList(al, 1, 0, &alsz)) {
      if (UpdateProcThreadAttribute(al, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                    hs, (SIZE_T)nh * sizeof(HANDLE),
                                    NULL, NULL)) {
        si.lpAttributeList = al;
      } else {
        DeleteProcThreadAttributeList(al);
        free(al);
        al = NULL;
      }
    } else { free(al); al = NULL; }
  }

  /* CREATE_NEW_PROCESS_GROUP: permite kill(hijo, SIGINT) (Ctrl+Break). */
  DWORD flags = CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED;
  BOOL ok;
  if (al) {
    ok = CreateProcessW(exe, cmd, NULL, NULL, TRUE,
                        flags | EXTENDED_STARTUPINFO_PRESENT,
                        NULL, NULL, &si.StartupInfo, &pi);
    if (!ok) {   /* p.ej. Win7 con handles de consola: heredar sin lista */
      si.lpAttributeList = NULL;
      si.StartupInfo.cb  = sizeof(STARTUPINFOW);
      ok = CreateProcessW(exe, cmd, NULL, NULL, TRUE, flags,
                          NULL, NULL, &si.StartupInfo, &pi);
    }
  } else {
    si.StartupInfo.cb = sizeof(STARTUPINFOW);
    ok = CreateProcessW(exe, cmd, NULL, NULL, redirected ? TRUE : FALSE,
                        flags, NULL, NULL, &si.StartupInfo, &pi);
  }
  DWORD err = GetLastError();

  if (al) { DeleteProcThreadAttributeList(al); free(al); }
  SetEnvironmentVariableW(L"UFORK_CHILD", NULL);
  SetEnvironmentVariableW(L"UFORK_PIPES", NULL);
  for (int i = 0; i < 3; ++i)
    if (smark[i])
      SetHandleInformation(sh[i], HANDLE_FLAG_INHERIT,
                           sfl[i] & HANDLE_FLAG_INHERIT);
  upipe__restore();                           /* suelta el lock de pipes */
  free(spec);

  if (!ok) { SetLastError(err); errno = EAGAIN; goto out_free; }

  ufork__job_add(pi.hProcess);
  {
    WCHAR en[64], mn[64];
    ufork__kid_t *k = &ufork__kids[ufork__nkids];
    ufork__sig_names(en, mn, pi.dwProcessId);
    k->term = NULL; k->pend = NULL;
    k->map  = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                 0, sizeof(LONG), mn);
    if (k->map) {
      k->pend = (volatile LONG *)MapViewOfFile(k->map, FILE_MAP_ALL_ACCESS,
                                               0, 0, sizeof(LONG));
      if (k->pend) k->term = CreateEventW(NULL, TRUE, FALSE, en);
    }
  }
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);
  ufork__kids[ufork__nkids].pid = (pid_t)pi.dwProcessId;
  ufork__kids[ufork__nkids].h   = pi.hProcess;
  ufork__nkids++;
  result = (pid_t)pi.dwProcessId;

out_free:
  free(exe);
  free(cmd);
  return result;
}

static inline pid_t ufork_fork(void)
{
  ufork__ensure_init();
  AcquireSRWLockExclusive(&ufork__lock);
  int n = ++ufork__calls;

  /* Hijo poniendose al dia: no se lanza nada. */
  if (ufork__is_child && n <= ufork__path[ufork__npath - 1]) {
    int mine = 0;
    for (int i = 0; i < ufork__npath; ++i) if (ufork__path[i] == n) mine = 1;
    int last = (n == ufork__path[ufork__npath - 1]);
    if (last) ufork__replaying = 0;
    ReleaseSRWLockExclusive(&ufork__lock);
    if (last) {
      ufork__quiet_end();
      if (ufork__replay_end_hook) ufork__replay_end_hook();
    }
    return mine ? 0 : UFORK__FAKE_PID;
  }

  pid_t pid;
  if (ufork__nkids >= UFORK__MAX_KIDS || ufork__npath >= UFORK__MAX_PATH_N - 1) {
    errno = EAGAIN;
    pid = -1;
  } else {
    pid = ufork__do_spawn(n);
  }
  ReleaseSRWLockExclusive(&ufork__lock);
  return pid;
}

/* ---------------------------------------------------------------- */
static inline int ufork__make_status(DWORD c)
{
  if (c == STATUS_CONTROL_C_EXIT)              return SIGINT;
  if ((c & 0xFFFFFF80UL) == UFORK_SIGEXIT_BASE) return (int)(c & 0x7f);
  if (c == 0xC000001DUL)                       return SIGILL;
  if (c >= 0xC0000090UL && c <= 0xC0000096UL)  return SIGFPE;
  if (c >= 0xC0000000UL && c <= 0xC00000FFUL)  return SIGSEGV;
  return (int)((c & 0xff) << 8);
}

static inline pid_t ufork_waitpid(pid_t pid, int *status, int options)
{
  ufork__ensure_init();
  HANDLE hs[UFORK__MAX_KIDS];
  pid_t  ps[UFORK__MAX_KIDS];
  int n = 0;

  AcquireSRWLockShared(&ufork__lock);
  for (int i = 0; i < ufork__nkids; ++i)
    if (pid <= 0 || ufork__kids[i].pid == pid) {
      hs[n] = ufork__kids[i].h;
      ps[n] = ufork__kids[i].pid;
      n++;
    }
  ReleaseSRWLockShared(&ufork__lock);

  if (n == 0) { errno = ECHILD; return -1; }

  DWORD r = WaitForMultipleObjects((DWORD)n, hs, FALSE,
                                   (options & WNOHANG) ? 0 : INFINITE);
  if (r == WAIT_TIMEOUT) return 0;
  if (r >= WAIT_OBJECT_0 + (DWORD)n) { errno = ECHILD; return -1; }

  int i = (int)(r - WAIT_OBJECT_0);
  DWORD code = 0;
  GetExitCodeProcess(hs[i], &code);
  if (status) *status = ufork__make_status(code);

  AcquireSRWLockExclusive(&ufork__lock);
  for (int k = 0; k < ufork__nkids; ++k)
    if (ufork__kids[k].pid == ps[i]) {
      CloseHandle(ufork__kids[k].h);
      if (ufork__kids[k].pend) UnmapViewOfFile((void *)ufork__kids[k].pend);
      if (ufork__kids[k].map)  CloseHandle(ufork__kids[k].map);
      if (ufork__kids[k].term) CloseHandle(ufork__kids[k].term);
      ufork__kids[k] = ufork__kids[--ufork__nkids];
      break;
    }
  ReleaseSRWLockExclusive(&ufork__lock);
  return ps[i];
}

static inline pid_t ufork_wait(int *status)
{
  return ufork_waitpid(-1, status, 0);
}

/* Windows no tiene uid/gid: valor fijo, solo para portar codigo. */
static inline unsigned ufork_getuid(void) { return 1000; }

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define getuid()           ufork_getuid()
#  define geteuid()          ufork_getuid()
#  define getgid()           ufork_getuid()
#  define fork()             ufork_fork()
#  define wait(s)            ufork_wait(s)
#  define waitpid(p, s, o)   ufork_waitpid((p), (s), (o))
#  define getppid()          ufork_getppid()
#endif

#else  /* ================= UNIX ================= */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ufork_fork       fork
#define ufork_wait       wait
#define ufork_waitpid    waitpid
#define ufork_getppid    getppid
#define ufork_replaying() 0

#endif /* _WIN32 */

#endif /* UNIXLIB_FORK_H */
