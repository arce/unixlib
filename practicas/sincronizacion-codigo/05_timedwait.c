// 05_timedwait.c: esperar una condición, pero solo hasta un plazo
//
// Un carro espera a tener 5 pasajeros, pero sale con los que haya si pasa 1 segundo.
#include "util.h"

#define PASAJEROS_TOTALES 6
#define CAPACIDAD 5

static int llegados = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void *pasajeros(void *arg) {
  (void)arg;
  unsigned semilla = 1;
  for (int i = 0; i < PASAJEROS_TOTALES; i++) {
    pausa_ms(&semilla, 300, 300); // llega un pasajero cada 300 ms
    CHECK(pthread_mutex_lock(&mutex));
    llegados++;
    printf("llegó el pasajero %d\n", llegados);
    CHECK(pthread_cond_signal(&cond));
    CHECK(pthread_mutex_unlock(&mutex));
  }
  return NULL;
}

static void *carro(void *arg) {
  (void)arg;

  // El plazo es una hora ABSOLUTA: ahora + 1 segundo.
  struct timespec limite;
  hora_actual(&limite);
  limite.tv_sec += 1;

  CHECK(pthread_mutex_lock(&mutex));
  int rc = 0;
  while (llegados < CAPACIDAD && rc != ETIMEDOUT) {
    rc = pthread_cond_timedwait(&cond, &mutex, &limite);
    if (rc != 0 && rc != ETIMEDOUT) {
      fprintf(stderr, "pthread_cond_timedwait falló: %s\n", strerror(rc));
      exit(1);
    }
  }
  if (llegados >= CAPACIDAD)
    printf("carro: salgo lleno con %d pasajeros\n", CAPACIDAD);
  else
    printf("carro: se acabó la espera, salgo con %d pasajeros\n", llegados);
  CHECK(pthread_mutex_unlock(&mutex));
  return NULL;
}

int main(void) {
  pthread_t t1, t2;
  CHECK(pthread_create(&t1, NULL, pasajeros, NULL));
  CHECK(pthread_create(&t2, NULL, carro, NULL));
  CHECK(pthread_join(t1, NULL));
  CHECK(pthread_join(t2, NULL));
  return 0;
}
