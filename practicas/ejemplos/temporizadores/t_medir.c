// t_medir.c: medir el tiempo de una tarea con gettimeofday() y clock()
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static double ahora(void) { // segundos (con fracción) desde un instante fijo
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  double inicio = ahora();
  clock_t cpu0 = clock();

  volatile double suma = 0; // la tarea a medir
  for (long i = 1; i <= 50000000L; i++)
    suma += 1.0 / i;

  double real = ahora() - inicio;
  double cpu = (double)(clock() - cpu0) / CLOCKS_PER_SEC;
  printf("suma = %.6f\n", suma);
  printf("tiempo real: %.3f s, tiempo de CPU: %.3f s\n", real, cpu);
  return 0;
}
