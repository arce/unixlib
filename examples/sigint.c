/* El padre envia SIGINT al hijo; el hijo lo captura. Requiere consola. */
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
      /* el padre mantiene abierto su extremo de escritura; el hijo
       * solo espera la señal. Se despierta cada vez que cambia 'got'. */
      if (read(fd[0], &c, 1) != 1 && !got) break;
    }
    _exit(got ? 10 : 11);
  }
  kill(pid, SIGINT);
  int st;
  waitpid(pid, &st, 0);
  printf("hijo termino: %d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
  return 0;
}
