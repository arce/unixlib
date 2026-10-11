// 01_carrera.c: una condición de carrera, con y sin mutex
//
// Uso: ./01_carrera sin | con
#include "util.h"

#define HILOS 4
#define VUELTAS 500000

// volatile evita que el compilador guarde el contador en un registro durante
// todo el ciclo. NO hace atómica la operación contador++.
static volatile long contador = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int usar_mutex;

static void *incrementa(void *arg) {
  (void)arg;
  for (int i = 0; i < VUELTAS; i++) {
    if (usar_mutex)
      CHECK(pthread_mutex_lock(&mutex));
    contador++; // sección crítica: leer, sumar 1 y escribir
    if (usar_mutex)
      CHECK(pthread_mutex_unlock(&mutex));
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2 || (strcmp(argv[1], "sin") != 0 && strcmp(argv[1], "con") != 0)) {
    fprintf(stderr, "uso: %s sin|con\n", argv[0]);
    return 1;
  }
  usar_mutex = strcmp(argv[1], "con") == 0;

  pthread_t hilos[HILOS];
  for (int i = 0; i < HILOS; i++)
    CHECK(pthread_create(&hilos[i], NULL, incrementa, NULL));
  for (int i = 0; i < HILOS; i++)
    CHECK(pthread_join(hilos[i], NULL));

  long esperado = (long)HILOS * VUELTAS;
  printf("esperado: %ld, obtenido: %ld\n", esperado, contador);
  if (contador != esperado) {
    printf("¡Se perdieron %ld incrementos!\n", esperado - contador);
    return usar_mutex ? 1 : 0; // sin mutex, perder incrementos es lo esperado
  }
  return 0;
}
