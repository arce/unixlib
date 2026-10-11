// t_periodico.c: tarea periódica cada 2 segundos con alarm()
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define PERIODO 2
#define VECES 5

static volatile sig_atomic_t pendiente = 0;

static void tic(int sig) {
  (void)sig;
  pendiente = 1; // el manejador solo avisa; el trabajo se hace afuera
  alarm(PERIODO);
}

int main(void) {
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGALRM);
  sigemptyset(&vacio);

  signal(SIGALRM, tic);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);
  alarm(PERIODO);
  for (int n = 0; n < VECES;) {
    sigsuspend(&vacio); // espera la señal sin perder ninguna
    if (pendiente) {
      pendiente = 0;
      time_t t = time(NULL);
      char hora[32];
      strftime(hora, sizeof hora, "%H:%M:%S", localtime(&t));
      printf("tarea %d ejecutada a las %s\n", ++n, hora);
      fflush(stdout);
    }
  }
  alarm(0); // cancelar la alarma pendiente
  return 0;
}
