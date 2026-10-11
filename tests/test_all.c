/* unixlib tests. Same code on Windows (TDM-GCC) and Unix. */
#include <stdio.h>
#include <string.h>
#include <unixlib/unixlib.h>

static volatile sig_atomic_t got_term;
static void on_term(int s) { (void)s; got_term = 1; }
static volatile sig_atomic_t got_usr1, got_alrm;
static void on_usr1(int s) { (void)s; got_usr1++; }
static void on_alrm(int s) { (void)s; got_alrm = 1; }
static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); \
                                  failures++; } } while (0)

int main(void)
{
  int st = 0, fd[2], fd2[2];
  pid_t pid, a, b;

  /* 1. exit status */
  pid = fork();
  if (pid == 0) _exit(7);
  CHECK(pid > 0);
  CHECK(waitpid(pid, &st, 0) == pid);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 7);
  CHECK(wait(&st) == -1);                      /* no children left */

  /* 2. child -> parent pipe, EOF and getppid */
  CHECK(pipe(fd) == 0);
  pid = fork();
  if (pid == 0) {
    int pp = (int)getppid();
    close(fd[0]);
    write(fd[1], "hola", 4);
    write(fd[1], &pp, sizeof pp);
    close(fd[1]);
    _exit(0);
  }
  close(fd[1]);
  {
    char buf[32];
    ssize_t n, tot = 0;
    while ((n = read(fd[0], buf + tot, sizeof buf - (size_t)tot)) > 0)
      tot += n;
    CHECK(n == 0);                             /* EOF = 0 */
    CHECK(tot == 4 + (ssize_t)sizeof(int));
    CHECK(memcmp(buf, "hola", 4) == 0);
    int pp = 0;
    if (tot == 4 + (ssize_t)sizeof(int)) memcpy(&pp, buf + 4, sizeof pp);
    CHECK(pp == (int)getpid());
  }
  close(fd[0]);
  CHECK(wait(&st) == pid && WEXITSTATUS(st) == 0);

  /* 3. two children */
  a = fork();
  if (a == 0) _exit(1);
  b = fork();
  if (b == 0) _exit(2);
  CHECK(a > 0 && b > 0 && a != b);
  {
    int sum = 0, i;
    for (i = 0; i < 2; ++i) {
      pid_t w = wait(&st);
      CHECK(w == a || w == b);
      sum += WEXITSTATUS(st);
    }
    CHECK(sum == 3);
  }

  /* 4. kill(SIGKILL) on a child blocked in read() on a pipe */
  CHECK(pipe(fd2) == 0);
  pid = fork();
  if (pid == 0) {
    char c;
    close(fd2[1]);
    read(fd2[0], &c, 1);                       /* the parent never writes */
    _exit(0);
  }
  CHECK(kill(pid, 0) == 0);
  CHECK(kill(pid, SIGKILL) == 0);
  CHECK(waitpid(pid, &st, 0) == pid);
  CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL);
  close(fd2[0]); close(fd2[1]);

  /* 5. cooperative SIGTERM: the child runs its handler and exits cleanly */
  CHECK(pipe(fd) == 0);
  pid = fork();
  if (pid == 0) {
    signal(SIGTERM, on_term);
    close(fd[0]);
    write(fd[1], "r", 1);                      /* "handler installed" */
    while (!got_term) usleep(1000);
    _exit(9);
  }
  close(fd[1]);
  {
    char c = 0;
    CHECK(read(fd[0], &c, 1) == 1);
  }
  CHECK(kill(pid, SIGTERM) == 0);
  CHECK(waitpid(pid, &st, 0) == pid);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 9);
  close(fd[0]);

  /* 6. kill(-pid, SIGKILL) y getuid */
  CHECK(pipe(fd2) == 0);
  pid = fork();
  if (pid == 0) { char c; close(fd2[1]); read(fd2[0], &c, 1); _exit(0); }
  CHECK(kill(-pid, SIGKILL) == 0);
  CHECK(waitpid(pid, &st, 0) == pid);
  CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL);
  close(fd2[0]); close(fd2[1]);
  CHECK((int)getuid() >= 0);

  /* 7. sigaction + raise + sigprocmask (in the same process) */
  {
    struct sigaction sa;
    sigset_t m;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_usr1;
    CHECK(sigaction(SIGUSR1, &sa, NULL) == 0);
    CHECK(raise(SIGUSR1) == 0);
    CHECK(got_usr1 == 1);                      /* synchronous */
    sigemptyset(&m); sigaddset(&m, SIGUSR1);
    CHECK(sigprocmask(SIG_BLOCK, &m, NULL) == 0);
    raise(SIGUSR1);
    CHECK(got_usr1 == 1);                      /* blocked: pending */
    CHECK(sigprocmask(SIG_UNBLOCK, &m, NULL) == 0);
    CHECK(got_usr1 == 2);                      /* delivered on unblock */
  }

  /* 8. SIGUSR1 from parent to child; the child waits in pause() */
  CHECK(pipe(fd) == 0);
  pid = fork();
  if (pid == 0) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_usr1;
    got_usr1 = 0;
    sigaction(SIGUSR1, &sa, NULL);
    close(fd[0]);
    write(fd[1], "r", 1);
    while (got_usr1 == 0) pause();
    _exit(11);
  }
  close(fd[1]);
  { char c = 0; CHECK(read(fd[0], &c, 1) == 1); }
  CHECK(kill(pid, SIGUSR1) == 0);
  CHECK(waitpid(pid, &st, 0) == pid);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 11);
  close(fd[0]);

  /* 9. alarm + SIGALRM */
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alrm;
    sigaction(SIGALRM, &sa, NULL);
    alarm(1);
    pause();
    CHECK(got_alrm == 1);
  }

  /* 10. WNOHANG with no children */
  CHECK(waitpid(-1, &st, WNOHANG) == -1);

  printf(failures ? "RESULT: %d failure(s)\n" : "RESULT: all OK\n",
         failures);
  return failures ? 1 : 0;
}
