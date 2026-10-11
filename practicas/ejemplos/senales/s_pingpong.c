// s_pingpong.c: padre e hijo se turnan con SIGUSR1 (sin perder señales)
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TURNOS 5

static volatile sig_atomic_t recibida = 0;

static void manejador(int sig) {
  (void)sig;
  recibida = 1;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  // Se bloquea SIGUSR1 desde antes de crear al hijo: así ninguna señal se pierde
  // y solo se entrega dentro de sigsuspend().
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGUSR1);
  sigemptyset(&vacio);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);

  pid_t hijo = fork();
  if (hijo == 0) {
    for (int i = 1; i <= TURNOS; i++) {
      while (!recibida)
        sigsuspend(&vacio);
      recibida = 0;
      printf("  hijo : pong %d\n", i);
      fflush(stdout);
      kill(getppid(), SIGUSR1);
    }
    return 0;
  }

  for (int i = 1; i <= TURNOS; i++) {
    printf("padre: ping %d\n", i);
    fflush(stdout);
    kill(hijo, SIGUSR1);
    while (!recibida)
      sigsuspend(&vacio);
    recibida = 0;
  }
  wait(NULL);
  printf("Fin del juego.\n");
  return 0;
}
