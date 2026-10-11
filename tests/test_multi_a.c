/* Cabeceras incluidas desde dos .c: debe enlazar y compartir estado. */
#include <stdio.h>
#include <unixlib/unixlib.h>

int child_in_other_tu(void);

int main(void)
{
  pid_t pid = fork();
  if (pid == 0) _exit(3);
  int st = 0;
  waitpid(pid, &st, 0);
  int a = WEXITSTATUS(st);
  int b = child_in_other_tu();
  printf("multi-TU: %d %d\n", a, b);
  return (a == 3 && b == 5) ? 0 : 1;
}
