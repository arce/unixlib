#ifndef UNIXLIB_SHM_H
#define UNIXLIB_SHM_H

/* ================================================================
 *  unixlib/shm.h - memoria compartida System V (shmget/shmat/
 *  shmdt/shmctl) sobre file mappings con nombre.  Ver ipc.h.
 *
 *  - El segmento es una vista directa del mapping: lo que escribe un
 *    proceso lo ven los demas al instante (sin copias).
 *  - shmat() devuelve una direccion alineada a 128 bytes (no a pagina).
 *  - En el replay de fork() el hijo recibe una vista privada
 *    (copy-on-write) que se sustituye, en la misma direccion, por la
 *    compartida cuando fork() devuelve 0 (ver ipc.h).
 *  - shm_nattch es aproximado: un proceso que termina sin shmdt()
 *    no lo decrementa.
 *
 *  API prefijada: ushm_get / ushm_at / ushm_dt / ushm_ctl.
 *  Macros POSIX: shmget shmat shmdt shmctl (UNIXLIB_NO_POSIX_NAMES
 *  las desactiva).
 * ================================================================ */

#include <unixlib/ipc.h>

#ifdef _WIN32

#define SHM_RDONLY 010000
#define SHM_RND    020000
#define SHM_LOCK   11
#define SHM_UNLOCK 12
#define SHMLBA     4096

typedef unsigned long shmatt_t;

struct shmid_ds {
  struct ipc_perm shm_perm;
  size_t          shm_segsz;
  time_t          shm_atime, shm_dtime, shm_ctime;
  pid_t           shm_cpid, shm_lpid;
  shmatt_t        shm_nattch;
};

#define USHM__MAX_ATT 128

typedef struct ushm__att {
  int   used;
  int   slot;      /* indice en uipc__objs[UIPC_SHM] */
  int   ro;
  int   priv;      /* vista copy-on-write del replay */
  void *base;      /* inicio de la vista (cabecera) */
} ushm__att;

UNIXLIB_SHARED ushm__att ushm__atts[USHM__MAX_ATT] = {{0}};

/* Fin del replay: vistas privadas -> compartidas, misma direccion. */
static inline void ushm__remap_private(void)
{
  AcquireSRWLockExclusive(&uipc__lock);
  for (int i = 0; i < USHM__MAX_ATT; ++i) {
    ushm__att *a = &ushm__atts[i];
    if (!a->used || !a->priv) continue;
    uipc__obj *o = &uipc__objs[UIPC_SHM][a->slot];
    void *p = NULL;
    UnmapViewOfFile(a->base);
    for (int t = 0; t < 1000 && !p; ++t) {
      p = MapViewOfFileEx(o->map, FILE_MAP_WRITE, 0, 0, 0, a->base);
      if (!p) Sleep(0);
    }
    if (!p) {
      fprintf(stderr, "unixlib: no se pudo remapear la memoria compartida "
                      "en %p (error %lu)\n", a->base, GetLastError());
      ExitProcess(1);
    }
    a->priv = 0;
  }
  ReleaseSRWLockExclusive(&uipc__lock);
}

static inline int ushm_get(key_t key, size_t size, int flags)
{
  uipc__obj *o = NULL;
  int created = 0;
  size_t sz = size ? size : 1;
  int id = uipc__acquire(UIPC_SHM, key, flags, UIPC__HDR + sz, UIPC__HDR,
                         (LONG64)sz, (LONG64)size, &o, &created);
  if (id < 0) return -1;
  if (created) uipc__publish(o);    /* el mapping ya viene a cero */
  return id;
}

static inline void *ushm_at(int id, const void *addr, int flags)
{
  uipc__obj *o = uipc__get(UIPC_SHM, id);
  if (!o) return (void *)-1;
  if (o->hdr->deleted) { errno = EIDRM; return (void *)-1; }

  int ro   = (flags & SHM_RDONLY) != 0;
  int priv = !ro && uipc__rep();
  DWORD acc = ro ? FILE_MAP_READ : (priv ? FILE_MAP_COPY : FILE_MAP_WRITE);
  void *base = MapViewOfFileEx(o->map, acc, 0, 0, 0,
                               addr ? (char *)addr - UIPC__HDR : NULL);
  if (!base) { errno = addr ? EINVAL : ENOMEM; return (void *)-1; }

  AcquireSRWLockExclusive(&uipc__lock);
  int k = -1;
  for (int i = 0; i < USHM__MAX_ATT; ++i)
    if (!ushm__atts[i].used) { k = i; break; }
  if (k < 0) {
    ReleaseSRWLockExclusive(&uipc__lock);
    UnmapViewOfFile(base);
    errno = ENOMEM;
    return (void *)-1;
  }
  ushm__atts[k].used = 1;
  ushm__atts[k].slot = id - 1;
  ushm__atts[k].ro   = ro;
  ushm__atts[k].priv = priv;
  ushm__atts[k].base = base;
  o->nviews++;
  ReleaseSRWLockExclusive(&uipc__lock);

  if (priv) ufork__replay_end_hook = ushm__remap_private;
  InterlockedIncrement(&o->hdr->nattch);
  o->hdr->lpid  = (LONG)GetCurrentProcessId();
  o->hdr->atime = uipc__now();
  return (char *)base + UIPC__HDR;
}

static inline int ushm_dt(const void *addr)
{
  int rc = -1;
  AcquireSRWLockExclusive(&uipc__lock);
  for (int i = 0; i < USHM__MAX_ATT; ++i) {
    ushm__att *a = &ushm__atts[i];
    if (!a->used || (char *)a->base + UIPC__HDR != (const char *)addr) continue;
    uipc__obj *o = &uipc__objs[UIPC_SHM][a->slot];
    UnmapViewOfFile(a->base);
    InterlockedDecrement(&o->hdr->nattch);
    o->hdr->lpid  = (LONG)GetCurrentProcessId();
    o->hdr->dtime = uipc__now();
    o->nviews--;
    a->used = 0;
    if (o->local_deleted && o->nviews == 0) uipc__release(o);
    rc = 0;
    break;
  }
  ReleaseSRWLockExclusive(&uipc__lock);
  if (rc != 0) errno = EINVAL;
  return rc;
}

static inline int ushm_ctl(int id, int cmd, struct shmid_ds *buf)
{
  uipc__obj *o = uipc__get(UIPC_SHM, id);
  if (!o) return -1;
  switch (cmd) {
  case IPC_RMID:
    if (uipc__rep()) return 0;          /* replay: ya lo hara el padre */
    AcquireSRWLockExclusive(&uipc__lock);
    uipc__mark_deleted(o);
    if (o->nviews == 0) uipc__release(o);
    ReleaseSRWLockExclusive(&uipc__lock);
    return 0;
  case IPC_STAT:
    if (!buf) { errno = EFAULT; return -1; }
    memset(buf, 0, sizeof *buf);
    buf->shm_perm.__key = o->key;
    buf->shm_perm.mode  = (unsigned short)o->hdr->mode;
    buf->shm_segsz = (size_t)o->hdr->size;
    buf->shm_atime = (time_t)o->hdr->atime;
    buf->shm_dtime = (time_t)o->hdr->dtime;
    buf->shm_ctime = (time_t)o->hdr->ctime_;
    buf->shm_cpid  = (pid_t)o->hdr->cpid;
    buf->shm_lpid  = (pid_t)o->hdr->lpid;
    buf->shm_nattch = o->hdr->nattch > 0 ? (shmatt_t)o->hdr->nattch : 0;
    return 0;
  case IPC_SET:
    if (buf) o->hdr->mode = buf->shm_perm.mode & 0777;
    return 0;
  case SHM_LOCK:
  case SHM_UNLOCK:
    return 0;
  default:
    errno = EINVAL;
    return -1;
  }
}

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define shmget(k, s, f)  ushm_get((k), (s), (f))
#  define shmat(i, a, f)   ushm_at((i), (a), (f))
#  define shmdt(a)         ushm_dt(a)
#  define shmctl(i, c, b)  ushm_ctl((i), (c), (b))
#endif

#else /* no Windows */
#include <sys/shm.h>
#define ushm_get shmget
#define ushm_at  shmat
#define ushm_dt  shmdt
#define ushm_ctl shmctl
#endif

#endif
