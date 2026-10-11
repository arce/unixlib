// t_cronometro.c: cronómetro que se detiene con Ctrl+C
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

static volatile sig_atomic_t detener = 0;

static void al_interrumpir(int sig) {
  (void)sig;
  detener = 1;
}

static double ahora(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  signal(SIGINT, al_interrumpir);
  printf("Cronómetro en marcha. Presione Ctrl+C para detenerlo.\n");
  double inicio = ahora();
  while (!detener) {
    printf("\r%7.1f s", ahora() - inicio);
    fflush(stdout);
    usleep(100000);
  }
  printf("\nTiempo total: %.2f segundos\n", ahora() - inicio);
  return 0;
}
