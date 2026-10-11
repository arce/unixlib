// t_alarma.c: cuenta regresiva con alarm() y SIGALRM
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t restante = 5;

static void tic(int sig) {
  (void)sig;
  restante--;
  if (restante > 0)
    alarm(1); // reprogramar: alarm() solo dispara una vez
}

int main(void) {
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGALRM);
  sigemptyset(&vacio);

  signal(SIGALRM, tic);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL); // SIGALRM solo llega dentro de sigsuspend
  alarm(1);
  while (restante > 0) {
    printf("faltan %d segundos\n", restante);
    fflush(stdout);
    sigsuspend(&vacio); // desbloquea SIGALRM y duerme, de forma atómica
  }
  printf("¡tiempo!\n");
  return 0;
}
