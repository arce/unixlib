/* Parent and child communicating through a pipe. */
#include <stdio.h>
#include <string.h>
#include <unixlib/unixlib.h>

int main(void)
{
  int fd[2];
  if (pipe(fd) != 0) { perror("pipe"); return 1; }

  printf("parent %d: creating child\n", (int)getpid());   /* printed once */
  pid_t pid = fork();
  if (pid < 0) { perror("fork"); return 1; }

  if (pid == 0) {                       /* ---- child ---- */
    const char *msg = "hello from the child\n";
    close(fd[0]);
    write(fd[1], msg, strlen(msg));
    close(fd[1]);
    _exit(42);
  }

  close(fd[1]);                         /* ---- parent ---- */
  char buf[128];
  ssize_t n;
  while ((n = read(fd[0], buf, sizeof buf - 1)) > 0) {
    buf[n] = '\0';
    printf("parent received: %s", buf);
  }
  close(fd[0]);

  int st;
  waitpid(pid, &st, 0);
  if (WIFEXITED(st)) printf("child exited with %d\n", WEXITSTATUS(st));
  return 0;
}
