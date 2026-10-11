// s_mascara.c: bloquear SIGINT con sigprocmask y detectarla con sigpending
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static void manejador(int sig) {
  (void)sig;
  printf("\n[manejador] SIGINT entregada al desbloquearla\n");
}

int main(void) {
  struct sigaction sa;
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  sigset_t bloqueo, pendientes;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGINT);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);

  printf("SIGINT bloqueada durante 5 segundos. Presione Ctrl+C si quiere.\n");
  for (int i = 0; i < 5; i++) {
    printf(".");
    fflush(stdout);
    sleep(1);
  }

  sigpending(&pendientes);
  if (sigismember(&pendientes, SIGINT))
    printf("\nHay una SIGINT pendiente.\n");
  else
    printf("\nNo llegó ninguna SIGINT.\n");

  sigprocmask(SIG_UNBLOCK, &bloqueo, NULL); // si había una pendiente, se entrega aquí
  printf("SIGINT desbloqueada. Fin.\n");
  return 0;
}
