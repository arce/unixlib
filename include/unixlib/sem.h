#ifndef UNIXLIB_SEM_H
#define UNIXLIB_SEM_H

/* ================================================================
 *  unixlib/sem.h - System V semaphores (semget/semop/semctl).
 *
 *  Each set lives in a file mapping (values, pids, counters)
 *  protected by a named mutex; blocked processes wait
 *  on a named event. semop() is atomic for the whole array of
 *  operations, as on Unix.
 *
 *  'union semun' is NOT defined: like glibc, the program defines it
 *  (the tutorial does). semctl() reads its 4th argument as a union
 *  of the same size ({int; pointer}).
 *
 *  Differences from Unix:
 *    - SEM_UNDO is accepted but ignored (nothing is undone on exit).
 *    - A blocked semop() is not interrupted by signals (no EINTR).
 *    - At most 1024 semaphores per set, maximum value 32767.
 *
 *  API prefijada: usem_get / usem_op / usem_ctl.
 *  Macros POSIX: semget semop semctl.
 * ================================================================ */

#include <unixlib/ipc.h>

#ifdef _WIN32

#include <stdarg.h>

#define GETPID   11
#define GETVAL   12
#define GETALL   13
#define GETNCNT  14
#define GETZCNT  15
#define SETVAL   16
#define SETALL   17
#define SEM_UNDO 0x1000
#define SEMVMX   32767
#define USEM__MAX 1024

struct sembuf {
  unsigned short sem_num;
  short          sem_op;
  short          sem_flg;
};

struct semid_ds {
  struct ipc_perm sem_perm;
  time_t          sem_otime, sem_ctime;
  unsigned long   sem_nsems;
};

typedef struct usem__s {
  volatile LONG val;
  volatile LONG pid;
  volatile LONG ncnt;
  volatile LONG zcnt;
} usem__s;

/* semctl argument: same size/ABI as the program's 'union semun'. */
union usem__arg { int val; void *ptr; };

static inline usem__s *usem__tab(uipc__obj *o)
{
  return (usem__s *)((char *)o->hdr + UIPC__HDR);
}

static inline int usem_get(key_t key, int nsems, int flags)
{
  uipc__obj *o = NULL;
  int created = 0;
  if (nsems < 0 || nsems > USEM__MAX) { errno = EINVAL; return -1; }
  int n = nsems ? nsems : 1;
  int id = uipc__acquire(UIPC_SEM, key, flags,
                         UIPC__HDR + (size_t)n * sizeof(usem__s), 0,
                         (LONG64)n, (LONG64)nsems, &o, &created);
  if (id < 0) return -1;
  if (created) uipc__publish(o);   /* values start at 0, as on Unix */
  return id;
}

static inline int usem_op(int id, struct sembuf *sops, size_t nsops)
{
  uipc__obj *o = uipc__get(UIPC_SEM, id);
  if (!o) return -1;
  if (uipc__rep()) return 0;                 /* replay: the parent already did it */
  if (!sops || nsops == 0) { errno = EINVAL; return -1; }

  LONG nsems = (LONG)o->hdr->size;
  for (size_t i = 0; i < nsops; ++i)
    if (sops[i].sem_num >= nsems) { errno = EFBIG; return -1; }

  usem__s *S = usem__tab(o);
  LONG tmp[USEM__MAX];
  int rc = 0;

  uipc__enter(o);
  uipc__lock_obj(o);
  for (;;) {
    if (o->hdr->deleted) { errno = EIDRM; rc = -1; break; }

    for (LONG i = 0; i < nsems; ++i) tmp[i] = S[i].val;
    long blocked = -1;
    int  range = 0;
    for (size_t i = 0; i < nsops; ++i) {
      LONG *v = &tmp[sops[i].sem_num];
      short op = sops[i].sem_op;
      if (op > 0) {
        if (*v + op > SEMVMX) { range = 1; break; }
        *v += op;
      } else if (op < 0) {
        if (*v + op < 0) { blocked = (long)i; break; }
        *v += op;
      } else if (*v != 0) { blocked = (long)i; break; }
    }
    if (range) { errno = ERANGE; rc = -1; break; }

    if (blocked < 0) {                       /* everything is possible: apply */
      for (size_t i = 0; i < nsops; ++i) {
        usem__s *s = &S[sops[i].sem_num];
        s->val = tmp[sops[i].sem_num];
        s->pid = (LONG)GetCurrentProcessId();
      }
      o->hdr->lpid  = (LONG)GetCurrentProcessId();
      o->hdr->atime = uipc__now();
      uipc__wake_obj(o);
      break;
    }

    if (sops[blocked].sem_flg & IPC_NOWAIT) { errno = EAGAIN; rc = -1; break; }

    usem__s *w = &S[sops[blocked].sem_num];
    volatile LONG *cnt = sops[blocked].sem_op < 0 ? &w->ncnt : &w->zcnt;
    InterlockedIncrement(cnt);
    uipc__sleep_obj(o);                      /* releases the mutex and waits */
    InterlockedDecrement(cnt);
  }
  uipc__unlock_obj(o);
  uipc__leave(o);
  return rc;
}

static inline int usem_ctl(int id, int num, int cmd, ...)
{
  union usem__arg arg;
  arg.ptr = NULL;
  if (cmd == SETVAL || cmd == GETALL || cmd == SETALL ||
      cmd == IPC_STAT || cmd == IPC_SET) {
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, union usem__arg);
    va_end(ap);
  }

  uipc__obj *o = uipc__get(UIPC_SEM, id);
  if (!o) return -1;

  if (cmd == IPC_RMID) {
    if (uipc__rep()) return 0;               /* replay: the parent will do it */
    uipc__remove(o);
    return 0;
  }

  LONG nsems = (LONG)o->hdr->size;
  usem__s *S = usem__tab(o);
  int rc = 0;

  if ((cmd == GETVAL || cmd == SETVAL || cmd == GETPID ||
       cmd == GETNCNT || cmd == GETZCNT) && (num < 0 || num >= nsems)) {
    errno = EINVAL;
    return -1;
  }
  if ((cmd == SETVAL && uipc__rep()) || (cmd == SETALL && uipc__rep()))
    return 0;                                /* replay: do not clobber the state */

  uipc__enter(o);
  uipc__lock_obj(o);
  if (o->hdr->deleted) {
    errno = EIDRM;
    rc = -1;
  } else switch (cmd) {
  case GETVAL:  rc = (int)S[num].val;  break;
  case GETPID:  rc = (int)S[num].pid;  break;
  case GETNCNT: rc = (int)S[num].ncnt; break;
  case GETZCNT: rc = (int)S[num].zcnt; break;
  case SETVAL:
    if (arg.val < 0 || arg.val > SEMVMX) { errno = ERANGE; rc = -1; break; }
    S[num].val = arg.val;
    S[num].pid = (LONG)GetCurrentProcessId();
    o->hdr->ctime_ = uipc__now();
    uipc__wake_obj(o);
    break;
  case GETALL:
    if (!arg.ptr) { errno = EFAULT; rc = -1; break; }
    for (LONG i = 0; i < nsems; ++i) ((unsigned short *)arg.ptr)[i] = (unsigned short)S[i].val;
    break;
  case SETALL:
    if (!arg.ptr) { errno = EFAULT; rc = -1; break; }
    for (LONG i = 0; i < nsems; ++i)
      if (((unsigned short *)arg.ptr)[i] > SEMVMX) { errno = ERANGE; rc = -1; break; }
    if (rc) break;
    for (LONG i = 0; i < nsems; ++i) S[i].val = ((unsigned short *)arg.ptr)[i];
    o->hdr->ctime_ = uipc__now();
    uipc__wake_obj(o);
    break;
  case IPC_STAT: {
    struct semid_ds *b = (struct semid_ds *)arg.ptr;
    if (!b) { errno = EFAULT; rc = -1; break; }
    memset(b, 0, sizeof *b);
    b->sem_perm.__key = o->key;
    b->sem_perm.mode  = (unsigned short)o->hdr->mode;
    b->sem_otime = (time_t)o->hdr->atime;
    b->sem_ctime = (time_t)o->hdr->ctime_;
    b->sem_nsems = (unsigned long)nsems;
    break;
  }
  case IPC_SET: {
    struct semid_ds *b = (struct semid_ds *)arg.ptr;
    if (b) o->hdr->mode = b->sem_perm.mode & 0777;
    break;
  }
  default:
    errno = EINVAL;
    rc = -1;
  }
  uipc__unlock_obj(o);
  uipc__leave(o);
  return rc;
}

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define semget(k, n, f)   usem_get((k), (n), (f))
#  define semop(i, s, n)    usem_op((i), (s), (n))
#  define semctl(...)       usem_ctl(__VA_ARGS__)
#endif

#else /* no Windows */
#include <sys/sem.h>
#define usem_get semget
#define usem_op  semop
#define usem_ctl semctl
#endif

#endif
