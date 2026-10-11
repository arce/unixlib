/* Padre e hijo comunicados por un pipe. */
#include <stdio.h>
#include <string.h>
#include <unixlib/unixlib.h>

int main(void)
{
  int fd[2];
  if (pipe(fd) != 0) { perror("pipe"); return 1; }

  printf("padre %d: creando hijo\n", (int)getpid());   /* se imprime una vez */
  pid_t pid = fork();
  if (pid < 0) { perror("fork"); return 1; }

  if (pid == 0) {                       /* ---- hijo ---- */
    const char *msg = "hola desde el hijo\n";
    close(fd[0]);
    write(fd[1], msg, strlen(msg));
    close(fd[1]);
    _exit(42);
  }

  close(fd[1]);                         /* ---- padre ---- */
  char buf[128];
  ssize_t n;
  while ((n = read(fd[0], buf, sizeof buf - 1)) > 0) {
    buf[n] = '\0';
    printf("padre recibio: %s", buf);
  }
  close(fd[0]);

  int st;
  waitpid(pid, &st, 0);
  if (WIFEXITED(st)) printf("hijo termino con %d\n", WEXITSTATUS(st));
  return 0;
}
