// 02_trylock.c: pthread_mutex_trylock, para no quedarse esperando un mutex
//
// Cada hilo hace trabajo propio y de vez en cuando anota su avance en un
// total compartido. Si el mutex está ocupado NO espera: sigue trabajando y
// anota todo junto en el siguiente intento.
#include "util.h"

#define HILOS 4
#define UNIDADES 100 // unidades de trabajo por hilo

static long total = 0; // compartido, protegido por mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int ocupado[HILOS]; // cuántas veces encontró el mutex ocupado

static void *trabajador(void *arg) {
  int id = (int)(intptr_t)arg;
  unsigned semilla = (unsigned)id + 1;
  long pendiente = 0; // avance que todavía no se ha anotado en total

  for (int i = 0; i < UNIDADES; i++) {
    pausa_ms(&semilla, 1, 3); // trabajo propio
    pendiente++;

    int rc = pthread_mutex_trylock(&mutex);
    if (rc == 0) { // lo obtuvo: anota todo lo pendiente
      total += pendiente;
      pendiente = 0;
      pausa_ms(&semilla, 2, 2); // anotar es lento (por ejemplo, escribir a disco)
      CHECK(pthread_mutex_unlock(&mutex));
    } else if (rc == EBUSY) { // estaba ocupado: no espera, sigue trabajando
      ocupado[id]++;
    } else {
      fprintf(stderr, "pthread_mutex_trylock falló: %s\n", strerror(rc));
      exit(1);
    }
  }

  // Al terminar sí hay que anotar lo que quedó, y ahora sí se espera.
  CHECK(pthread_mutex_lock(&mutex));
  total += pendiente;
  CHECK(pthread_mutex_unlock(&mutex));
  return NULL;
}

int main(void) {
  pthread_t hilos[HILOS];
  for (int i = 0; i < HILOS; i++)
    CHECK(pthread_create(&hilos[i], NULL, trabajador, (void *)(intptr_t)i));
  for (int i = 0; i < HILOS; i++)
    CHECK(pthread_join(hilos[i], NULL));

  int fallidos = 0;
  for (int i = 0; i < HILOS; i++)
    fallidos += ocupado[i];
  printf("total: %ld (esperado %d)\n", total, HILOS * UNIDADES);
  printf("veces que el mutex estaba ocupado: %d\n", fallidos);
  return total == HILOS * UNIDADES ? 0 : 1;
}
