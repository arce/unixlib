// 03_interbloqueo.c: interbloqueo (deadlock) con dos mutex
//
// Uso: ./03_interbloqueo mal | orden
#include "util.h"

static pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
static int con_orden;

static void *hilo1(void *arg) {
  (void)arg;
  unsigned semilla = 1;
  CHECK(pthread_mutex_lock(&mutex_a));
  printf("hilo 1: tiene A, quiere B\n");
  pausa_ms(&semilla, 100, 100); // da tiempo a que el hilo 2 tome su mutex
  CHECK(pthread_mutex_lock(&mutex_b));
  printf("hilo 1: tiene A y B\n");
  CHECK(pthread_mutex_unlock(&mutex_b));
  CHECK(pthread_mutex_unlock(&mutex_a));
  return NULL;
}

static void *hilo2(void *arg) {
  (void)arg;
  unsigned semilla = 2;
  // Con orden: A y luego B, igual que el hilo 1.
  // Sin orden: B y luego A, al revés que el hilo 1.
  pthread_mutex_t *primero = con_orden ? &mutex_a : &mutex_b;
  pthread_mutex_t *segundo = con_orden ? &mutex_b : &mutex_a;

  CHECK(pthread_mutex_lock(primero));
  printf("hilo 2: tiene %s, quiere %s\n", con_orden ? "A" : "B",
         con_orden ? "B" : "A");
  pausa_ms(&semilla, 100, 100);
  CHECK(pthread_mutex_lock(segundo));
  printf("hilo 2: tiene A y B\n");
  CHECK(pthread_mutex_unlock(segundo));
  CHECK(pthread_mutex_unlock(primero));
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2 || (strcmp(argv[1], "mal") != 0 && strcmp(argv[1], "orden") != 0)) {
    fprintf(stderr, "uso: %s mal|orden\n", argv[0]);
    return 1;
  }
  con_orden = strcmp(argv[1], "orden") == 0;
  if (!con_orden) // si a los 2 segundos sigue vivo, hay un interbloqueo
    guardian(2, "¡INTERBLOQUEO! Cada hilo espera el mutex que tiene el otro.");

  pthread_t t1, t2;
  CHECK(pthread_create(&t1, NULL, hilo1, NULL));
  CHECK(pthread_create(&t2, NULL, hilo2, NULL));
  CHECK(pthread_join(t1, NULL));
  CHECK(pthread_join(t2, NULL));
  printf("terminaron sin problemas\n");
  return 0;
}
