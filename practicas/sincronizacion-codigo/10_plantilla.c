// 10_plantilla.c: plantilla para simulaciones con hilos que llegan "en forma
// continua". Clientes que llegan al azar y se atienden en 3 ventanillas.
#include "semaforo.h"
#include "util.h"

#define CLIENTES 12
#define VENTANILLAS 3

static semaforo_t ventanillas;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t terminaron = PTHREAD_COND_INITIALIZER;
static int pendientes = CLIENTES; // clientes que aún no terminan
static int atendidos = 0;

static void *cliente(void *arg) {
  int id = (int)(intptr_t)arg;
  unsigned semilla = (unsigned)id * 7919u + 1;

  printf("Cliente %2d llega\n", id);
  semaforo_wait(&ventanillas); // espera una ventanilla libre
  printf("Cliente %2d es atendido\n", id);
  pausa_ms(&semilla, 100, 300); // el trabajo del cliente
  printf("Cliente %2d termina\n", id);
  semaforo_post(&ventanillas);

  CHECK(pthread_mutex_lock(&mutex));
  atendidos++;
  pendientes--;
  if (pendientes == 0)
    CHECK(pthread_cond_signal(&terminaron));
  CHECK(pthread_mutex_unlock(&mutex));
  return NULL;
}

int main(void) {
  semaforo_init(&ventanillas, VENTANILLAS);

  // Los hilos cliente son "detached": nadie los espera con pthread_join.
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr));
  CHECK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));

  unsigned semilla = 1;
  for (int id = 1; id <= CLIENTES; id++) {
    pausa_ms(&semilla, 20, 150); // tiempo entre llegadas
    pthread_t hilo;
    CHECK(pthread_create(&hilo, &attr, cliente, (void *)(intptr_t)id));
  }
  CHECK(pthread_attr_destroy(&attr));

  // Como no hay join, main espera con una variable de condición.
  CHECK(pthread_mutex_lock(&mutex));
  while (pendientes > 0)
    CHECK(pthread_cond_wait(&terminaron, &mutex));
  CHECK(pthread_mutex_unlock(&mutex));

  printf("Se atendieron %d clientes.\n", atendidos);
  return atendidos == CLIENTES ? 0 : 1;
}
