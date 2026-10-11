/* The parent sends SIGINT to the child; the child catches it. Needs a console. */
#include <stdio.h>
#include <unixlib/unixlib.h>

static volatile sig_atomic_t got;
static void on_int(int s) { (void)s; got = 1; }

int main(void)
{
  int fd[2];
  pipe(fd);
  pid_t pid = fork();
  if (pid == 0) {
    char c;
    signal(SIGINT, on_int);
    close(fd[1]);
    while (!got) {
      /* the parent keeps its write end open; the child just waits for
       * the signal. It wakes up every time 'got' changes. */
      if (read(fd[0], &c, 1) != 1 && !got) break;
    }
    _exit(got ? 10 : 11);
  }
  kill(pid, SIGINT);
  int st;
  waitpid(pid, &st, 0);
  printf("child exited: %d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
  return 0;
}
