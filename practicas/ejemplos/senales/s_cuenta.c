// s_cuenta.c: contar las veces que el usuario presiona Ctrl+C
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t cuenta = 0; // única forma segura de compartir datos con un manejador

static void manejador(int sig) {
  (void)sig;
  cuenta++;
}

int main(void) {
  struct sigaction sa;
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  printf("Presione Ctrl+C tres veces para terminar.\n");
  int mostrada = 0;
  while (cuenta < 3) {
    pause(); // espera una señal sin gastar CPU
    if (cuenta != mostrada) {
      mostrada = cuenta;
      printf("Ctrl+C recibido %d vez/veces\n", mostrada);
      fflush(stdout);
    }
  }
  printf("Total: %d. Adiós.\n", (int)cuenta);
  return 0;
}
