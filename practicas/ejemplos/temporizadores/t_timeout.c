// t_timeout.c: esperar a un proceso hijo como máximo 3 segundos
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define LIMITE_MS 3000

int main(void) {
  pid_t pid = fork();
  if (pid == 0) { // el hijo hace una tarea que tarda 10 segundos
    printf("hijo %d: trabajando 10 segundos...\n", (int)getpid());
    fflush(stdout);
    sleep(10);
    printf("hijo: terminé\n");
    return 0;
  }

  int estado, transcurrido = 0;
  pid_t r;
  while ((r = waitpid(pid, &estado, WNOHANG)) == 0 && transcurrido < LIMITE_MS) {
    usleep(100000); // revisar cada 100 ms
    transcurrido += 100;
  }
  if (r == 0) { // se agotó el tiempo y el hijo sigue vivo
    printf("padre: tiempo agotado (%d ms), cancelo al hijo\n", transcurrido);
    kill(pid, SIGTERM);
    waitpid(pid, &estado, 0);
    if (WIFSIGNALED(estado))
      printf("padre: el hijo terminó por la señal %d\n", WTERMSIG(estado));
  } else {
    printf("padre: el hijo terminó a tiempo\n");
  }
  return 0;
}
