/* Segunda unidad de traduccion: usa las mismas cabeceras. */
#include <unixlib/unixlib.h>

int child_in_other_tu(void)
{
  int fd[2];
  if (pipe(fd) != 0) return -1;
  pid_t pid = fork();
  if (pid == 0) _exit(5);
  close(fd[0]); close(fd[1]);
  int st = 0;
  if (waitpid(pid, &st, 0) != pid) return -2;
  return WEXITSTATUS(st);
}
