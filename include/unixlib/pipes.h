#ifndef UNIXLIB_PIPES_H
#define UNIXLIB_PIPES_H

/* ================================================================
 *  unixlib/pipes.h - pipe() con descriptores REALES del CRT.
 *
 *  Los fds devueltos son fds normales de MSVCRT: read(), write(),
 *  close(), dup(), dup2(), fdopen()... funcionan sin envoltorios,
 *  y read() devuelve 0 en EOF. Nada de tabla propia: no colisiona
 *  con stdin/stdout/stderr.
 *
 *  Herencia: al llamar a fork(), solo los pipes creados con pipe()
 *  que sigan abiertos se pasan al hijo (lista explicita de handles,
 *  no se hereda nada mas). Archivos y sockets NO se heredan.
 *  Como en POSIX, cierra en cada proceso los extremos que no uses
 *  o el lector nunca vera EOF.
 *
 *  Header-only: static inline + estado UNIXLIB_SHARED.
 *
 *  API prefijada: upipe_pipe.  Macro POSIX: pipe(fd)
 *  (desactivar con UNIXLIB_NO_POSIX_NAMES).
 * ================================================================ */

#ifdef _WIN32

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#  define WINVER _WIN32_WINNT
#endif

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <unistd.h>       /* read/write/close/dup2, STDIN_FILENO... */

#ifndef UNIXLIB_SHARED
   /* Estado compartido entre unidades de traduccion (header-only):
    * COMDAT/selectany -> una sola instancia en el ejecutable. */
#  define UNIXLIB_SHARED __declspec(selectany)
#endif

#define UPIPE_MAX_HANDLES 256

/* Definida en unixlib/fork.h (se incluye al final de este archivo). */
static inline void ufork__ensure_init(void);

/* ---- implementacion ---- */
/* ----------------------------------------------------------------
 *  Registro de pipes creados con upipe_pipe(), en orden de creacion.
 *  - seq numera cada llamada a upipe_pipe() (para el replay del hijo).
 *  - Se guarda el HANDLE junto al fd para detectar extremos cerrados
 *    (close() es el del CRT y no pasa por aqui) o fds reutilizados.
 * ---------------------------------------------------------------- */
#define UPIPE__REG_MAX 128

typedef struct {
  unsigned seq;
  int      fd[2];        /* [0]=lectura  [1]=escritura */
  HANDLE   h[2];
} upipe__reg_t;

UNIXLIB_SHARED SRWLOCK upipe__lock = SRWLOCK_INIT;
UNIXLIB_SHARED upipe__reg_t upipe__reg[UPIPE__REG_MAX] = {{0}};
UNIXLIB_SHARED int upipe__nreg = 0;
UNIXLIB_SHARED unsigned upipe__seq = 0; /* siguiente numero de pipe() */

/* Estado de upipe__prepare/upipe__restore. */
UNIXLIB_SHARED HANDLE upipe__marked[UPIPE_MAX_HANDLES] = {0};
UNIXLIB_SHARED DWORD upipe__marked_flags[UPIPE_MAX_HANDLES] = {0};
UNIXLIB_SHARED int upipe__nmarked = 0;

static inline int upipe__end_open(int fd, HANDLE h)
{
  if (fd < 0 || !h) return 0;
  intptr_t cur = _get_osfhandle(fd);
  if (cur == -1 || (HANDLE)cur != h) return 0;
  return GetFileType(h) == FILE_TYPE_PIPE;
}

/* Elimina entradas con ambos extremos cerrados. */
static inline void upipe__reg_compact(void)
{
  int j = 0;
  for (int i = 0; i < upipe__nreg; ++i) {
    if (upipe__end_open(upipe__reg[i].fd[0], upipe__reg[i].h[0]) ||
        upipe__end_open(upipe__reg[i].fd[1], upipe__reg[i].h[1]))
      upipe__reg[j++] = upipe__reg[i];
  }
  upipe__nreg = j;
}

/* ----------------------------------------------------------------
 *  Hijo: pipes heredados indexados por seq.
 * ---------------------------------------------------------------- */
static inline int upipe_pipe(int fd[2])
{
  ufork__ensure_init();
  if (!fd) { errno = EFAULT; return -1; }

  AcquireSRWLockExclusive(&upipe__lock);
  unsigned seq = upipe__seq++;

  /* Replay: devuelve el pipe heredado que corresponde a esta llamada. */
  for (int i = 0; i < upipe__nreg; ++i) {
    if (upipe__reg[i].seq == seq) {
      fd[0] = upipe__reg[i].fd[0];
      fd[1] = upipe__reg[i].fd[1];
      ReleaseSRWLockExclusive(&upipe__lock);
      return 0;
    }
  }

  if (upipe__nreg == UPIPE__REG_MAX) upipe__reg_compact();
  if (upipe__nreg == UPIPE__REG_MAX) {
    upipe__seq--;
    ReleaseSRWLockExclusive(&upipe__lock);
    errno = EMFILE;
    return -1;
  }

  int p[2];
  /* _O_NOINHERIT: nada se hereda salvo lo que fork() marque. */
  if (_pipe(p, 65536, _O_BINARY | _O_NOINHERIT) != 0) {
    upipe__seq--;
    ReleaseSRWLockExclusive(&upipe__lock);
    return -1;                       /* errno ya lo fijo _pipe */
  }

  upipe__reg_t *r = &upipe__reg[upipe__nreg++];
  r->seq   = seq;
  r->fd[0] = p[0];  r->h[0] = (HANDLE)_get_osfhandle(p[0]);
  r->fd[1] = p[1];  r->h[1] = (HANDLE)_get_osfhandle(p[1]);
  ReleaseSRWLockExclusive(&upipe__lock);

  fd[0] = p[0];
  fd[1] = p[1];
  return 0;
}

/* ----------------------------------------------------------------
 *  Padre: serializa los pipes abiertos como
 *     "seq:rh:wh;"  (handles en hex, 0 = extremo cerrado)
 * ---------------------------------------------------------------- */
static inline char *upipe__prepare(HANDLE *hs, int *nh)
{
  *nh = 0;
  upipe__nmarked = 0;

  AcquireSRWLockExclusive(&upipe__lock);   /* se libera en upipe__restore() */

  size_t cap = (size_t)upipe__nreg * 48 + 16;
  char *out = (char *)malloc(cap);
  if (!out) { ReleaseSRWLockExclusive(&upipe__lock); return NULL; }
  size_t off = 0;
  out[0] = '\0';

  for (int i = 0; i < upipe__nreg; ++i) {
    unsigned long long hv[2] = { 0, 0 };
    for (int k = 0; k < 2; ++k) {
      if (!upipe__end_open(upipe__reg[i].fd[k], upipe__reg[i].h[k])) continue;
      if (*nh >= UPIPE_MAX_HANDLES) continue;
      HANDLE h = upipe__reg[i].h[k];
      DWORD fl = 0;
      if (!GetHandleInformation(h, &fl)) continue;
      if (!SetHandleInformation(h, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
        continue;
      upipe__marked[upipe__nmarked]       = h;
      upipe__marked_flags[upipe__nmarked] = fl & HANDLE_FLAG_INHERIT;
      upipe__nmarked++;
      hs[(*nh)++] = h;
      hv[k] = (unsigned long long)(uintptr_t)h;
    }
    int w = snprintf(out + off, cap - off, "%u:%llx:%llx;",
                     upipe__reg[i].seq, hv[0], hv[1]);
    if (w < 0 || (size_t)w >= cap - off) break;
    off += (size_t)w;
  }
  return out;
}

static inline void upipe__restore(void)
{
  for (int i = 0; i < upipe__nmarked; ++i)
    SetHandleInformation(upipe__marked[i], HANDLE_FLAG_INHERIT,
                         upipe__marked_flags[i]);
  upipe__nmarked = 0;
  ReleaseSRWLockExclusive(&upipe__lock);
}

/* ----------------------------------------------------------------
 *  Hijo: abre fds del CRT sobre los handles heredados.
 *  Un extremo cerrado en el padre se sustituye por un fd a NUL, para
 *  que el close() que re-ejecuta el hijo siga siendo valido.
 * ---------------------------------------------------------------- */
static inline int upipe__adopt(unsigned long long hv, int rdonly)
{
  if (hv) {
    HANDLE h = (HANDLE)(uintptr_t)hv;
    int fd = _open_osfhandle((intptr_t)h, rdonly ? _O_RDONLY : _O_WRONLY);
    if (fd >= 0) {
      _setmode(fd, _O_BINARY);
      SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
      return fd;
    }
  }
  return _open("NUL", (rdonly ? _O_RDONLY : _O_WRONLY) | _O_BINARY |
                      _O_NOINHERIT);
}

static inline void upipe__import_locked(const char *spec)
{
  const char *p = spec;
  while (*p && upipe__nreg < UPIPE__REG_MAX) {
    unsigned seq;
    unsigned long long rh, wh;
    int used = 0;
    if (sscanf(p, "%u:%llx:%llx;%n", &seq, &rh, &wh, &used) < 3 || !used)
      break;
    upipe__reg_t *r = &upipe__reg[upipe__nreg++];
    r->seq   = seq;
    r->fd[0] = upipe__adopt(rh, 1);
    r->fd[1] = upipe__adopt(wh, 0);
    r->h[0]  = r->fd[0] >= 0 ? (HANDLE)_get_osfhandle(r->fd[0]) : NULL;
    r->h[1]  = r->fd[1] >= 0 ? (HANDLE)_get_osfhandle(r->fd[1]) : NULL;
    p += used;
  }
}

static inline void upipe__import(const char *spec)
{
  if (!spec || !*spec) return;
  AcquireSRWLockExclusive(&upipe__lock);
  upipe__import_locked(spec);
  ReleaseSRWLockExclusive(&upipe__lock);
}

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define pipe(fd) upipe_pipe(fd)
#endif

#include <unixlib/fork.h>

#else  /* ================= UNIX ================= */

#include <unistd.h>
#include <sys/types.h>

#define upipe_pipe pipe

#endif /* _WIN32 */

#endif /* UNIXLIB_PIPES_H */
