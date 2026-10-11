// 04_condicion.c: variable de condición. Un hilo espera a que otro prepare un dato.
#include "util.h"

static int listo = 0; // la condición que se espera
static int dato = 0;  // el dato que protege la condición
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void *espera(void *arg) {
  (void)arg;
  CHECK(pthread_mutex_lock(&mutex));
  while (!listo) // siempre en un while, nunca en un if
    CHECK(pthread_cond_wait(&cond, &mutex));
  printf("espera: el dato listo es %d\n", dato);
  CHECK(pthread_mutex_unlock(&mutex));
  return NULL;
}

static void *avisa(void *arg) {
  (void)arg;
  unsigned semilla = 1;
  pausa_ms(&semilla, 200, 200); // simula preparar el dato
  CHECK(pthread_mutex_lock(&mutex));
  dato = 42;
  listo = 1; // primero se cambia el estado...
  CHECK(pthread_cond_signal(&cond)); // ...y después se avisa
  CHECK(pthread_mutex_unlock(&mutex));
  return NULL;
}

int main(void) {
  pthread_t t1, t2;
  CHECK(pthread_create(&t1, NULL, espera, NULL));
  CHECK(pthread_create(&t2, NULL, avisa, NULL));
  CHECK(pthread_join(t1, NULL));
  CHECK(pthread_join(t2, NULL));
  return dato == 42 ? 0 : 1;
}
