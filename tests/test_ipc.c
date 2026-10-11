/* Tests for shm.h / sem.h / msg.h. Same code on Windows (TDM-GCC)
* and Unix.  The parent checks and the result is reported through the
* exit status of each child. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unixlib/unixlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>

union semun { int val; struct semid_ds *buf; unsigned short *array; };

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); \
                                  failures++; } } while (0)

struct m { long type; char text[32]; };
#define MSZ sizeof(((struct m *)0)->text)

static void sem_set(int id, int n, int v)
{ union semun a; a.val = v; semctl(id, n, SETVAL, a); }
static void sem_op1(int id, int n, int op)
{ struct sembuf sb; sb.sem_num = (unsigned short)n; sb.sem_op = (short)op; sb.sem_flg = 0; semop(id, &sb, 1); }

int main(void)
{
  int st = 0;
  pid_t pid;
  struct m mm;

  /* 1. shm written BEFORE the fork is inherited; the child's replay does not clobber it */
  {
    int sid = shmget(0x5501, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);
    int qid = msgget(0x5502, IPC_CREAT | S_IRUSR | S_IWUSR);
    CHECK(sid != -1 && qid != -1);
    int *shm = (int *)shmat(sid, NULL, 0);
    CHECK(shm != (int *)-1);
    shm[0] = 0;                      /* the child repeats this during the replay */
    shm[1] = 7;
    pid = fork();
    if (pid == 0) {
      msgrcv(qid, &mm, MSZ, 1, 0);   /* waits for the parent to write 42 */
      int ok = (shm[0] == 42 && shm[1] == 7);
      shm[2] = 99;                   /* the child writes after the fork */
      shmdt(shm);
      _exit(ok ? 0 : 1);
    }
    shm[0] = 42;                     /* after the fork */
    mm.type = 1; strcpy(mm.text, "go");
    CHECK(msgsnd(qid, &mm, MSZ, IPC_NOWAIT) == 0);
    CHECK(waitpid(pid, &st, 0) == pid);
    CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0);
    CHECK(shm[2] == 99);
    struct shmid_ds ds;
    CHECK(shmctl(sid, IPC_STAT, &ds) == 0 && ds.shm_segsz >= 4096 - 0);
    CHECK(shmdt(shm) == 0);
    CHECK(shmctl(sid, IPC_RMID, NULL) == 0);
    CHECK(msgctl(qid, IPC_RMID, NULL) == 0);
    /* after RMID, the same key creates a new, zeroed segment */
    int sid2 = shmget(0x5501, 64, IPC_CREAT | S_IRUSR | S_IWUSR);
    int *p2 = (int *)shmat(sid2, NULL, 0);
    CHECK(p2 != (int *)-1 && p2[0] == 0 && p2[1] == 0);
    shmdt(p2);
    shmctl(sid2, IPC_RMID, NULL);
  }

  /* 2. semaphore as a mutex: 2 x 1000 increments */
  {
    int sem = semget(0x5511, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    sem_set(sem, 0, 1);
    int sid = shmget(0x5512, 64, IPC_CREAT | S_IRUSR | S_IWUSR);
    int *shm = (int *)shmat(sid, NULL, 0);
    *shm = 0;
    pid = fork();
    for (int i = 0; i < 1000; i++) {
      sem_op1(sem, 0, -1);
      int t = *shm; t++; *shm = t;
      sem_op1(sem, 0, +1);
    }
    if (pid == 0) { shmdt(shm); _exit(0); }
    CHECK(waitpid(pid, &st, 0) == pid);
    CHECK(*shm == 2000);
    CHECK(semctl(sem, 0, GETVAL) == 1);
    shmdt(shm);
    shmctl(sid, IPC_RMID, NULL);
    semctl(sem, 0, IPC_RMID);
  }

  /* 3. semop: IPC_NOWAIT, atomic semop and GETALL */
  {
    int sem = semget(0x5521, 2, IPC_CREAT | S_IRUSR | S_IWUSR);
    sem_set(sem, 0, 1);
    sem_set(sem, 1, 0);
    struct sembuf two[2] = {{0, -1, 0}, {1, -1, IPC_NOWAIT}};
    errno = 0;
    CHECK(semop(sem, two, 2) == -1 && errno == EAGAIN);
    CHECK(semctl(sem, 0, GETVAL) == 1);        /* nothing was applied */
    unsigned short all[2] = {0, 0};
    union semun a; a.array = all;
    CHECK(semctl(sem, 0, GETALL, a) == 0 && all[0] == 1 && all[1] == 0);
    semctl(sem, 0, IPC_RMID);
  }

  /* 4. messages: types, NOWAIT, E2BIG, ordering between types */
  {
    int q = msgget(0x5531, IPC_CREAT | S_IRUSR | S_IWUSR);
    errno = 0;
    CHECK(msgrcv(q, &mm, MSZ, 0, IPC_NOWAIT) == -1 && errno == ENOMSG);
    mm.type = 3; strcpy(mm.text, "tres");  CHECK(msgsnd(q, &mm, MSZ, 0) == 0);
    mm.type = 1; strcpy(mm.text, "uno");   CHECK(msgsnd(q, &mm, MSZ, 0) == 0);
    mm.type = 2; strcpy(mm.text, "dos");   CHECK(msgsnd(q, &mm, MSZ, 0) == 0);
    CHECK(msgrcv(q, &mm, MSZ, 2, 0) == (int)MSZ && strcmp(mm.text, "dos") == 0);
    CHECK(msgrcv(q, &mm, MSZ, -2, 0) == (int)MSZ && strcmp(mm.text, "uno") == 0);
    errno = 0;
    CHECK(msgrcv(q, &mm, 4, 0, 0) == -1 && errno == E2BIG);
    CHECK(msgrcv(q, &mm, 4, 0, MSG_NOERROR) == 4 && mm.type == 3);
    struct msqid_ds ds;
    CHECK(msgctl(q, IPC_STAT, &ds) == 0 && ds.msg_qnum == 0);
    msgctl(q, IPC_RMID, NULL);
  }

  /* 5. chain of 2 children with 2 queues (tutorial exercise) */
  {
    int q1 = msgget(0x5541, IPC_CREAT | S_IRUSR | S_IWUSR);
    int q2 = msgget(0x5542, IPC_CREAT | S_IRUSR | S_IWUSR);
    pid_t a = fork();
    if (a == 0) {
      struct m x;
      msgrcv(q1, &x, MSZ, 0, 0);
      for (char *c = x.text; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
      msgsnd(q2, &x, MSZ, 0);
      _exit(0);
    }
    pid_t b = fork();
    if (b == 0) {
      struct m x;
      msgrcv(q2, &x, MSZ, 0, 0);
      _exit(strcmp(x.text, "HOLA MUNDO") == 0 ? 0 : 1);
    }
    mm.type = 1; strcpy(mm.text, "hola mundo");
    msgsnd(q1, &mm, MSZ, 0);
    CHECK(waitpid(a, &st, 0) == a && WEXITSTATUS(st) == 0);
    CHECK(waitpid(b, &st, 0) == b && WEXITSTATUS(st) == 0);
    msgctl(q1, IPC_RMID, NULL);
    msgctl(q2, IPC_RMID, NULL);
  }

  printf(failures ? "IPC RESULT: %d failure(s)\n" : "IPC RESULT: all OK\n", failures);
  return failures ? 1 : 0;
}
