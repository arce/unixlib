#ifndef UNIXLIB_MSG_H
#define UNIXLIB_MSG_H

/* ================================================================
 *  unixlib/msg.h - System V message queues (msgget/msgsnd/
 *  msgrcv/msgctl).
 *
 *  Each queue is a file mapping with an arena of records
 *  [length, type, data], protected by a named mutex; blocked senders
 *  and receivers wait on a named event.
 *
 *  Linux semantics:
 *    - msgsz excludes the initial 'long type' field; the text starts at
 *      (char*)msgp + sizeof(long)  (on Windows long is 4 bytes).
 *    - msgrcv: msgtyp 0 = first; >0 = first of that type (or different
 *      from that type with MSG_EXCEPT); <0 = the lowest type <= |msgtyp|.
 *    - IPC_NOWAIT -> EAGAIN (msgsnd, cola llena) / ENOMSG (msgrcv).
 *    - Message larger than msgsz: E2BIG, or truncated with MSG_NOERROR.
 *    - Default capacity 16384 bytes of data (msg_qbytes),
 *      maximum message 8192 bytes.
 *  Difference: a blocked msgrcv()/msgsnd() is not interrupted by
 *  signals (no EINTR).
 *
 *  API prefijada: umsg_get / umsg_snd / umsg_rcv / umsg_ctl.
 *  Macros POSIX: msgget msgsnd msgrcv msgctl.
 * ================================================================ */

#include <unixlib/ipc.h>

#ifdef _WIN32

#define MSG_NOERROR 010000
#define MSG_EXCEPT  020000

#define UMSG__MAX_MSG   8192
#define UMSG__QBYTES    16384
#define UMSG__ARENA     65536

typedef unsigned long msgqnum_t;
typedef unsigned long msglen_t;

struct msgbuf {
  long mtype;
  char mtext[1];
};

struct msqid_ds {
  struct ipc_perm msg_perm;
  time_t          msg_stime, msg_rtime, msg_ctime;
  msglen_t        msg_cbytes;
  msgqnum_t       msg_qnum;
  msglen_t        msg_qbytes;
  pid_t           msg_lspid, msg_lrpid;
};

typedef struct umsg__q {
  LONG qnum;
  LONG cbytes;     /* bytes of data in the queue */
  LONG qbytes;     /* limit for cbytes */
  LONG used;       /* bytes used in the arena (with headers) */
  LONG pad[4];
} umsg__q;

typedef struct umsg__rec {
  LONG len;
  LONG type;
} umsg__rec;

static inline umsg__q *umsg__qp(uipc__obj *o)
{
  return (umsg__q *)((char *)o->hdr + UIPC__HDR);
}
static inline char *umsg__arena(uipc__obj *o)
{
  return (char *)o->hdr + UIPC__HDR + sizeof(umsg__q);
}
static inline LONG umsg__recsz(LONG len) { return (LONG)sizeof(umsg__rec) + ((len + 7) & ~7); }

static inline int umsg_get(key_t key, int flags)
{
  uipc__obj *o = NULL;
  int created = 0;
  int id = uipc__acquire(UIPC_MSG, key, flags,
                         UIPC__HDR + sizeof(umsg__q) + UMSG__ARENA, 0,
                         UMSG__QBYTES, 0, &o, &created);
  if (id < 0) return -1;
  if (created) {
    umsg__q *q = umsg__qp(o);
    q->qbytes = UMSG__QBYTES;
    uipc__publish(o);
  }
  return id;
}

static inline int umsg_snd(int id, const void *msgp, size_t msgsz, int flags)
{
  uipc__obj *o = uipc__get(UIPC_MSG, id);
  if (!o) return -1;
  if (uipc__rep()) return 0;                 /* replay: the parent already sent it */
  if (!msgp) { errno = EFAULT; return -1; }

  long type;
  memcpy(&type, msgp, sizeof type);
  if (type < 1 || msgsz > UMSG__MAX_MSG) { errno = EINVAL; return -1; }

  umsg__q *q = umsg__qp(o);
  char *A = umsg__arena(o);
  LONG len = (LONG)msgsz, need = umsg__recsz(len);
  int rc = 0;

  uipc__enter(o);
  uipc__lock_obj(o);
  for (;;) {
    if (o->hdr->deleted) { errno = EIDRM; rc = -1; break; }
    if (len > q->qbytes) { errno = EINVAL; rc = -1; break; }
    if (q->cbytes + len <= q->qbytes && q->used + need <= UMSG__ARENA) {
      umsg__rec r;
      r.len = len;
      r.type = (LONG)type;
      memcpy(A + q->used, &r, sizeof r);
      if (len) memcpy(A + q->used + sizeof r, (const char *)msgp + sizeof(long), (size_t)len);
      q->used += need;
      q->cbytes += len;
      q->qnum++;
      o->hdr->lpid  = (LONG)GetCurrentProcessId();
      o->hdr->atime = uipc__now();
      uipc__wake_obj(o);
      break;
    }
    if (flags & IPC_NOWAIT) { errno = EAGAIN; rc = -1; break; }
    uipc__sleep_obj(o);
  }
  uipc__unlock_obj(o);
  uipc__leave(o);
  return rc;
}

static inline ssize_t umsg_rcv(int id, void *msgp, size_t msgsz, long msgtyp, int flags)
{
  uipc__obj *o = uipc__get(UIPC_MSG, id);
  if (!o) return -1;
  if (uipc__rep()) return 0;                 /* replay: the parent already received it */
  if (!msgp) { errno = EFAULT; return -1; }

  umsg__q *q = umsg__qp(o);
  char *A = umsg__arena(o);
  ssize_t rc = 0;

  uipc__enter(o);
  uipc__lock_obj(o);
  for (;;) {
    if (o->hdr->deleted) { errno = EIDRM; rc = -1; break; }

    /* find the matching message */
    LONG off = 0, best = -1;
    LONG besttype = 0;
    while (off < q->used) {
      umsg__rec r;
      memcpy(&r, A + off, sizeof r);
      int match;
      if (msgtyp == 0)      match = 1;
      else if (msgtyp > 0)  match = (flags & MSG_EXCEPT) ? (r.type != msgtyp) : (r.type == msgtyp);
      else                  match = (r.type <= -msgtyp);
      if (match) {
        if (msgtyp >= 0) { best = off; break; }          /* the first one */
        if (best < 0 || r.type < besttype) { best = off; besttype = r.type; }
      }
      off += umsg__recsz(r.len);
    }

    if (best >= 0) {
      umsg__rec r;
      memcpy(&r, A + best, sizeof r);
      if ((size_t)r.len > msgsz && !(flags & MSG_NOERROR)) {
        errno = E2BIG; rc = -1; break;
      }
      size_t n = (size_t)r.len < msgsz ? (size_t)r.len : msgsz;
      long t = (long)r.type;
      memcpy(msgp, &t, sizeof t);
      if (n) memcpy((char *)msgp + sizeof(long), A + best + sizeof r, n);
      LONG sz = umsg__recsz(r.len);
      memmove(A + best, A + best + sz, (size_t)(q->used - best - sz));
      q->used  -= sz;
      q->cbytes -= r.len;
      q->qnum--;
      o->hdr->lrpid = (LONG)GetCurrentProcessId();
      o->hdr->dtime = uipc__now();
      uipc__wake_obj(o);
      rc = (ssize_t)n;
      break;
    }
    if (flags & IPC_NOWAIT) { errno = ENOMSG; rc = -1; break; }
    uipc__sleep_obj(o);
  }
  uipc__unlock_obj(o);
  uipc__leave(o);
  return rc;
}

static inline int umsg_ctl(int id, int cmd, struct msqid_ds *buf)
{
  uipc__obj *o = uipc__get(UIPC_MSG, id);
  if (!o) return -1;
  umsg__q *q = umsg__qp(o);

  switch (cmd) {
  case IPC_RMID:
    if (uipc__rep()) return 0;               /* replay: the parent will do it */
    uipc__remove(o);
    return 0;
  case IPC_STAT:
    if (!buf) { errno = EFAULT; return -1; }
    uipc__lock_obj(o);
    memset(buf, 0, sizeof *buf);
    buf->msg_perm.__key = o->key;
    buf->msg_perm.mode  = (unsigned short)o->hdr->mode;
    buf->msg_stime  = (time_t)o->hdr->atime;
    buf->msg_rtime  = (time_t)o->hdr->dtime;
    buf->msg_ctime  = (time_t)o->hdr->ctime_;
    buf->msg_cbytes = (msglen_t)q->cbytes;
    buf->msg_qnum   = (msgqnum_t)q->qnum;
    buf->msg_qbytes = (msglen_t)q->qbytes;
    buf->msg_lspid  = (pid_t)o->hdr->lpid;
    buf->msg_lrpid  = (pid_t)o->hdr->lrpid;
    uipc__unlock_obj(o);
    return 0;
  case IPC_SET:
    if (!buf) { errno = EFAULT; return -1; }
    uipc__lock_obj(o);
    o->hdr->mode = buf->msg_perm.mode & 0777;
    if (buf->msg_qbytes > 0) {
      LONG nb = (LONG)(buf->msg_qbytes > UMSG__ARENA / 2 ? UMSG__ARENA / 2 : buf->msg_qbytes);
      q->qbytes = nb;
      uipc__wake_obj(o);
    }
    uipc__unlock_obj(o);
    return 0;
  default:
    errno = EINVAL;
    return -1;
  }
}

#ifndef UNIXLIB_NO_POSIX_NAMES
#  define msgget(k, f)           umsg_get((k), (f))
#  define msgsnd(i, p, s, f)     umsg_snd((i), (p), (s), (f))
#  define msgrcv(i, p, s, t, f)  umsg_rcv((i), (p), (s), (t), (f))
#  define msgctl(i, c, b)        umsg_ctl((i), (c), (b))
#endif

#else /* no Windows */
#include <sys/msg.h>
#define umsg_get msgget
#define umsg_snd msgsnd
#define umsg_rcv msgrcv
#define umsg_ctl msgctl
#endif

#endif
