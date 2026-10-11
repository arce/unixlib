#ifndef UNIXLIB_IPC_H
#define UNIXLIB_IPC_H

/* ================================================================
 *  unixlib/ipc.h - base comun de System V IPC (shm.h, sem.h, msg.h).
 *
 *  Windows no tiene IPC de System V. Aqui cada objeto (segmento,
 *  conjunto de semaforos, cola de mensajes) es un FILE MAPPING con
 *  nombre derivado de la key:
 *
 *      Local\unixlib_<tipo>_k<key>[_g<n>]      (mapping)
 *      ...\_m   mutex nombrado   ...\_e   evento nombrado
 *
 *  - Todos los procesos que usen la misma key ven el mismo objeto
 *    (como en Unix, aunque no esten emparentados).
 *  - IPC_PRIVATE crea un objeto con nombre unico; los hijos lo
 *    reencuentran por el replay de fork().
 *  - IPC_RMID marca el objeto como borrado: una nueva key igual crea
 *    un objeto nuevo (generacion siguiente), aunque algun proceso aun
 *    conserve abierto el viejo. Windows lo libera al cerrar el ultimo
 *    handle (al terminar el ultimo proceso).
 *
 *  fork() y el replay: el hijo vuelve a ejecutar desde main() las
 *  llamadas anteriores al fork. Para que no destruyan el estado ya
 *  compartido, mientras ufork_replaying() es verdadero:
 *    - shmget/semget/msgget abren el objeto existente (no lo recrean).
 *    - semop, msgsnd, msgrcv, semctl(SETVAL/SETALL/IPC_RMID),
 *      shmctl(IPC_RMID), msgctl(IPC_RMID) NO hacen nada.
 *    - shmat devuelve una vista PRIVADA (copy-on-write): las escrituras
 *      repetidas (*shm = 0; sprintf(shm,...)) no tocan el segmento real.
 *      Al terminar el replay la vista se reemplaza, en la MISMA
 *      direccion, por la vista compartida.
 *
 *  Header-only: static inline + estado UNIXLIB_SHARED.
 * ================================================================ */

#ifdef _WIN32

#include <unixlib/fork.h>      /* windows.h, ufork_replaying(), UNIXLIB_SHARED */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* ---- errno que mingw puede no definir ---- */
#ifndef EIDRM
#  define EIDRM 111
#endif
#ifndef ENOMSG
#  define ENOMSG 122
#endif
#ifndef EFBIG
#  define EFBIG 27
#endif
#ifndef E2BIG
#  define E2BIG 7
#endif
#ifndef ERANGE
#  define ERANGE 34
#endif

/* ---- permisos (sys/stat.h de mingw ya trae S_IRUSR/S_IWUSR) ---- */
#ifndef S_IRUSR
#  define S_IRUSR 0400
#  define S_IWUSR 0200
#endif
#ifndef S_IRGRP
#  define S_IRGRP 0040
#  define S_IWGRP 0020
#endif
#ifndef S_IROTH
#  define S_IROTH 0004
#  define S_IWOTH 0002
#endif
#ifndef S_IRWXU
#  define S_IRWXU 0700
#endif

/* ---- tipos y constantes de <sys/ipc.h> ---- */
#ifndef UNIXLIB_KEY_T_DEFINED
#  define UNIXLIB_KEY_T_DEFINED
typedef int key_t;
#endif

#define IPC_CREAT   01000
#define IPC_EXCL    02000
#define IPC_NOWAIT  04000
#define IPC_RMID    0
#define IPC_SET     1
#define IPC_STAT    2
#define IPC_INFO    3
#define IPC_PRIVATE ((key_t)0)

struct ipc_perm {
  key_t          __key;
  unsigned int   uid, gid, cuid, cgid;
  unsigned short mode;
  unsigned short __seq;
};

/* ftok(): mismo papel que en Unix (no necesita ficheros reales). */
static inline key_t uipc_ftok(const char *path, int id)
{
  struct _stat64 st;
  unsigned long long ino = 0;
  if (!path || _stat64(path, &st) != 0) return (key_t)-1;
  /* Windows no da inodo: mezclar tiempo de creacion y tamaño. */
  ino = (unsigned long long)st.st_ctime * 31u + (unsigned long long)st.st_size;
  return (key_t)(((unsigned)(id & 0xff) << 24) | (unsigned)(ino & 0xffffff));
}

static inline int uipc_getpagesize(void)
{
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return (int)si.dwPageSize;
}

/* ================================================================
 *  Cabecera comun (primeros UIPC__HDR bytes de cada mapping)
 * ================================================================ */
#define UIPC__HDR       128
#define UIPC__MAX_OBJS  64
#define UIPC__MAX_GEN   16
#define UIPC__WAIT_MS   200        /* red de seguridad ante despertares perdidos */

enum { UIPC_SHM = 0, UIPC_SEM = 1, UIPC_MSG = 2, UIPC__NKINDS = 3 };

typedef struct uipc__hdr {
  volatile LONG ready;      /* 0 hasta que el creador termina de inicializar */
  volatile LONG deleted;    /* IPC_RMID */
  LONG          kind;
  LONG          key;
  LONG64        size;       /* shm: bytes; sem: nsems; msg: qbytes */
  volatile LONG nattch;
  LONG          cpid;
  LONG          lpid;       /* ultimo pid que opero (otras: lspid) */
  LONG          lrpid;      /* msg: ultimo pid que recibio */
  LONG          mode;
  LONG64        atime;      /* shm: shmat | sem: semop | msg: msgsnd */
  LONG64        dtime;      /* shm: shmdt | msg: msgrcv */
  LONG64        ctime_;
  LONG          pad[6];
} uipc__hdr;

typedef struct uipc__obj {
  int        used;
  int        kind;
  key_t      key;
  int        local_deleted;   /* RMID visto por este proceso */
  HANDLE     map;
  HANDLE     mx;              /* mutex  (sem/msg) */
  HANDLE     ev;              /* evento (sem/msg) */
  uipc__hdr *hdr;             /* vista compartida (cabecera, o todo) */
  char       name[96];
  int        nviews;          /* shm: vistas de datos abiertas */
  volatile LONG users;        /* sem/msg: llamadas en curso en este proceso */
} uipc__obj;

UNIXLIB_SHARED SRWLOCK   uipc__lock = SRWLOCK_INIT;
UNIXLIB_SHARED uipc__obj uipc__objs[UIPC__NKINDS][UIPC__MAX_OBJS] = {{{0}}};
UNIXLIB_SHARED LONG      uipc__privseq[UIPC__NKINDS] = {0, 0, 0};

static inline const char *uipc__kname(int kind)
{
  return kind == UIPC_SHM ? "shm" : kind == UIPC_SEM ? "sem" : "msg";
}

static inline int uipc__rep(void) { return ufork_replaying(); }

static inline LONG64 uipc__now(void) { return (LONG64)time(NULL); }

/* ---- nombres ---- */
static inline void uipc__name(char *out, size_t n, int kind, key_t key,
                              int gen, int priv, unsigned creator, long seq)
{
  if (priv)
    snprintf(out, n, "Local\\unixlib_%s_p%u_%ld", uipc__kname(kind), creator, seq);
  else if (gen == 0)
    snprintf(out, n, "Local\\unixlib_%s_k%08x", uipc__kname(kind), (unsigned)key);
  else
    snprintf(out, n, "Local\\unixlib_%s_k%08x_g%d", uipc__kname(kind), (unsigned)key, gen);
}

/* ---- cierre de las vistas/handles de una entrada ---- */
static inline void uipc__release(uipc__obj *o)
{
  if (o->hdr) UnmapViewOfFile(o->hdr);
  if (o->ev)  CloseHandle(o->ev);
  if (o->mx)  CloseHandle(o->mx);
  if (o->map) CloseHandle(o->map);
  memset(o, 0, sizeof *o);
}

/* ---- apertura de sincronizacion (mutex+evento) de un objeto ---- */
static inline int uipc__open_sync(uipc__obj *o)
{
  char nm[128];
  snprintf(nm, sizeof nm, "%s_m", o->name);
  o->mx = CreateMutexA(NULL, FALSE, nm);
  snprintf(nm, sizeof nm, "%s_e", o->name);
  o->ev = CreateEventA(NULL, TRUE, FALSE, nm);   /* manual-reset + PulseEvent */
  return o->mx && o->ev;
}

/* Espera (acotada) a que el creador termine de inicializar. */
static inline int uipc__wait_ready(uipc__hdr *h)
{
  for (int i = 0; i < 5000; ++i) {
    if (h->ready) return 1;
    Sleep(i < 50 ? 0 : 1);
  }
  return h->ready != 0;
}

/* Intenta mapear la cabecera/todo; devuelve NULL si falla. */
static inline uipc__hdr *uipc__map_hdr(HANDLE map, size_t bytes)
{
  return (uipc__hdr *)MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
}

/* ----------------------------------------------------------------
 *  uipc__acquire: busca/crea el objeto (kind,key) y lo registra.
 *    total  : bytes del mapping al CREARLO (incluye UIPC__HDR)
 *    hbytes : bytes a mapear para la vista de control (0 = todo)
 *    size   : valor a guardar en hdr->size (solo al crear)
 *    check  : size pedido para validar contra el existente
 *  Devuelve id (>=1) o -1 con errno.
 *  *created = 1 si lo creo este proceso (para que el llamador
 *  inicialice el contenido antes de publicar con uipc__publish()).
 * ---------------------------------------------------------------- */
static inline int uipc__acquire(int kind, key_t key, int flags, size_t total,
                                size_t hbytes, LONG64 size, LONG64 check,
                                uipc__obj **out, int *created)
{
  int id = -1;
  uipc__obj *o = NULL;
  int priv = (key == IPC_PRIVATE);
  *created = 0;

  AcquireSRWLockExclusive(&uipc__lock);

  /* ranura libre */
  int slot = -1;
  for (int i = 0; i < UIPC__MAX_OBJS; ++i)
    if (!uipc__objs[kind][i].used) { slot = i; break; }
  if (slot < 0) { errno = ENOSPC; goto done; }

  /* mismo objeto ya abierto en este proceso */
  if (!priv) {
    for (int i = 0; i < UIPC__MAX_OBJS; ++i) {
      uipc__obj *e = &uipc__objs[kind][i];
      if (e->used && e->key == key && !e->local_deleted && !e->hdr->deleted) {
        if ((flags & IPC_CREAT) && (flags & IPC_EXCL)) { errno = EEXIST; goto done; }
        if (check > e->hdr->size && kind != UIPC_MSG) { errno = EINVAL; goto done; }
        *out = e; id = i + 1; goto done;
      }
    }
  }

  o = &uipc__objs[kind][slot];
  memset(o, 0, sizeof *o);
  o->kind = kind;
  o->key = key;

  HANDLE map = NULL;
  uipc__hdr *h = NULL;
  int existed = 0;

  if (priv) {
    /* Nombre determinista para que el hijo (replay) lo reencuentre:
     * creador = el propio proceso, o el padre mientras se repite. */
    unsigned creator = (ufork_replaying() && ufork__ppid) ? (unsigned)ufork__ppid
                                                          : (unsigned)GetCurrentProcessId();
    long seq = (long)uipc__privseq[kind]++;
    uipc__name(o->name, sizeof o->name, kind, key, 0, 1, creator, seq);
    map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                             (DWORD)((unsigned long long)total >> 32),
                             (DWORD)(total & 0xffffffffu), o->name);
    if (!map) { errno = ENOMEM; goto fail; }
    existed = (GetLastError() == ERROR_ALREADY_EXISTS);
  } else {
    /* 1) existe alguno vivo en alguna generacion? */
    for (int g = 0; g < UIPC__MAX_GEN && !map; ++g) {
      uipc__name(o->name, sizeof o->name, kind, key, g, 0, 0, 0);
      HANDLE m = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, o->name);
      if (!m) continue;
      uipc__hdr *t = uipc__map_hdr(m, hbytes);
      if (t && uipc__wait_ready(t) && !t->deleted) { map = m; h = t; existed = 1; }
      else { if (t) UnmapViewOfFile(t); CloseHandle(m); }
    }
    if (!map) {
      if (!(flags & IPC_CREAT)) { errno = ENOENT; goto fail; }
      /* 2) crear en la primera generacion libre */
      for (int g = 0; g < UIPC__MAX_GEN && !map; ++g) {
        uipc__name(o->name, sizeof o->name, kind, key, g, 0, 0, 0);
        HANDLE m = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                      (DWORD)((unsigned long long)total >> 32),
                                      (DWORD)(total & 0xffffffffu), o->name);
        if (!m) { errno = ENOMEM; goto fail; }
        if (GetLastError() != ERROR_ALREADY_EXISTS) { map = m; existed = 0; break; }
        /* otro proceso lo creo justo ahora: usarlo si sigue vivo */
        uipc__hdr *t = uipc__map_hdr(m, hbytes);
        if (t && uipc__wait_ready(t) && !t->deleted) { map = m; h = t; existed = 1; }
        else { if (t) UnmapViewOfFile(t); CloseHandle(m); }
      }
      if (!map) { errno = ENOSPC; goto fail; }
    }
  }
  o->map = map;

  if (existed && priv) {
    /* IPC_PRIVATE reencontrado (replay en el hijo) */
    h = uipc__map_hdr(map, hbytes);
    if (!h || !uipc__wait_ready(h)) { errno = ENOENT; goto fail; }
  }

  if (existed) {
    if (!h) h = uipc__map_hdr(map, hbytes);
    if (!h) { errno = ENOMEM; goto fail; }
    if (!priv && (flags & IPC_CREAT) && (flags & IPC_EXCL)) { errno = EEXIST; goto fail; }
    if (check > h->size && kind != UIPC_MSG) { errno = EINVAL; goto fail; }
    o->hdr = h;
  } else {
    h = uipc__map_hdr(map, hbytes);
    if (!h) { errno = ENOMEM; goto fail; }
    o->hdr = h;
    h->kind = kind;
    h->key = key;
    h->size = size;
    h->cpid = (LONG)GetCurrentProcessId();
    h->mode = flags & 0777;
    h->ctime_ = uipc__now();
    *created = 1;       /* el llamador inicializa y luego uipc__publish() */
  }
  if (kind != UIPC_SHM && !uipc__open_sync(o)) { errno = ENOMEM; goto fail; }

  o->used = 1;
  *out = o;
  id = slot + 1;
  goto done;

fail:
  if (o) uipc__release(o);
  id = -1;
done:
  ReleaseSRWLockExclusive(&uipc__lock);
  return id;
}

static inline void uipc__publish(uipc__obj *o)
{
  InterlockedExchange(&o->hdr->ready, 1);
}

/* Entrada por id; NULL (errno EINVAL) si no existe. */
static inline uipc__obj *uipc__get(int kind, int id)
{
  uipc__obj *o = NULL;
  AcquireSRWLockShared(&uipc__lock);
  if (id >= 1 && id <= UIPC__MAX_OBJS && uipc__objs[kind][id - 1].used)
    o = &uipc__objs[kind][id - 1];
  ReleaseSRWLockShared(&uipc__lock);
  if (!o) errno = EINVAL;
  return o;
}

/* ---- exclusion mutua y espera sobre el objeto (sem/msg) ---- */
static inline void uipc__lock_obj(uipc__obj *o)
{
  WaitForSingleObject(o->mx, INFINITE);      /* WAIT_ABANDONED: tomado igualmente */
}
static inline void uipc__unlock_obj(uipc__obj *o) { ReleaseMutex(o->mx); }

/* Con el mutex tomado: lo suelta, espera un cambio y lo vuelve a tomar. */
static inline void uipc__sleep_obj(uipc__obj *o)
{
  SignalObjectAndWait(o->mx, o->ev, UIPC__WAIT_MS, FALSE);
  WaitForSingleObject(o->mx, INFINITE);
}
/* Con el mutex tomado: avisa a los que esperan. */
static inline void uipc__wake_obj(uipc__obj *o) { PulseEvent(o->ev); }

/* ---- marcar borrado (IPC_RMID) ---- */
static inline void uipc__mark_deleted(uipc__obj *o)
{
  InterlockedExchange(&o->hdr->deleted, 1);
  o->local_deleted = 1;
  if (o->ev) PulseEvent(o->ev);
}

/* ---- operaciones en curso (para liberar tras IPC_RMID sin carreras) ---- */
static inline void uipc__enter(uipc__obj *o) { InterlockedIncrement(&o->users); }
static inline void uipc__leave(uipc__obj *o)
{
  if (InterlockedDecrement(&o->users) == 0 && o->local_deleted) {
    AcquireSRWLockExclusive(&uipc__lock);
    if (o->used && o->users == 0) uipc__release(o);
    ReleaseSRWLockExclusive(&uipc__lock);
  }
}
/* IPC_RMID de sem/msg: marca y libera cuando no queden llamadas en curso. */
static inline void uipc__remove(uipc__obj *o)
{
  uipc__mark_deleted(o);
  AcquireSRWLockExclusive(&uipc__lock);
  if (o->users == 0) uipc__release(o);
  ReleaseSRWLockExclusive(&uipc__lock);
}


#ifndef UNIXLIB_NO_POSIX_NAMES
#  define getpagesize()  uipc_getpagesize()
#  define ftok(p, i)     uipc_ftok((p), (i))
#endif

#else /* ---------------- no Windows: sistema real ---------------- */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <unistd.h>
#define uipc_getpagesize getpagesize
#define uipc_ftok        ftok
#endif /* _WIN32 */

#endif /* UNIXLIB_IPC_H */
