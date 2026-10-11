// t_dormir.c: sleep() y usleep(), y cuánto duermen realmente
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

static double ahora(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  double t0 = ahora();
  sleep(1);
  printf("sleep(1)        tardó %.3f s\n", ahora() - t0);

  t0 = ahora();
  usleep(250000); // 250 000 microsegundos = 250 ms
  printf("usleep(250000)  tardó %.3f s\n", ahora() - t0);

  t0 = ahora();
  for (int i = 0; i < 10; i++)
    usleep(1000); // 10 veces 1 ms: ¿cuánto tarda en realidad?
  printf("10 x usleep(1000) tardó %.3f s (ideal: 0.010)\n", ahora() - t0);
  return 0;
}
