// 07_buffer_sem.c: productores y consumidores con semáforos
#include "semaforo.h"
#include "util.h"

#define K 8
#define PRODUCTORES 3
#define CONSUMIDORES 2
#define ITEMS 20000
#define FIN (-1) // dato especial: "no habrá más datos"

static int datos[K];
static int entra = 0, sale = 0;
static semaforo_t vacios; // cuenta los espacios libres (empieza en K)
static semaforo_t llenos; // cuenta los datos disponibles (empieza en 0)
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // protege los índices

static void poner(int dato) {
  semaforo_wait(&vacios); // espera un espacio libre
  CHECK(pthread_mutex_lock(&mutex));
  datos[entra] = dato;
  entra = (entra + 1) % K;
  CHECK(pthread_mutex_unlock(&mutex));
  semaforo_post(&llenos); // hay un dato más
}

static int sacar(void) {
  semaforo_wait(&llenos); // espera un dato
  CHECK(pthread_mutex_lock(&mutex));
  int dato = datos[sale];
  sale = (sale + 1) % K;
  CHECK(pthread_mutex_unlock(&mutex));
  semaforo_post(&vacios); // hay un espacio más
  return dato;
}

static void *productor(void *arg) {
  (void)arg;
  for (int v = 1; v <= ITEMS; v++)
    poner(v);
  return NULL;
}

static long sumas[CONSUMIDORES];

static void *consumidor(void *arg) {
  int id = (int)(intptr_t)arg;
  int dato;
  while ((dato = sacar()) != FIN)
    sumas[id] += dato;
  return NULL;
}

int main(void) {
  semaforo_init(&vacios, K);
  semaforo_init(&llenos, 0);

  pthread_t prod[PRODUCTORES], cons[CONSUMIDORES];
  for (int i = 0; i < CONSUMIDORES; i++)
    CHECK(pthread_create(&cons[i], NULL, consumidor, (void *)(intptr_t)i));
  for (int i = 0; i < PRODUCTORES; i++)
    CHECK(pthread_create(&prod[i], NULL, productor, NULL));

  for (int i = 0; i < PRODUCTORES; i++)
    CHECK(pthread_join(prod[i], NULL));
  for (int i = 0; i < CONSUMIDORES; i++)
    poner(FIN); // un FIN por consumidor
  for (int i = 0; i < CONSUMIDORES; i++)
    CHECK(pthread_join(cons[i], NULL));

  long total = 0;
  for (int i = 0; i < CONSUMIDORES; i++) {
    printf("consumidor %d: suma %ld\n", i, sumas[i]);
    total += sumas[i];
  }
  long esperado = (long)PRODUCTORES * ITEMS * (ITEMS + 1) / 2;
  printf("total: %ld, esperado: %ld\n", total, esperado);
  semaforo_destroy(&vacios);
  semaforo_destroy(&llenos);
  if (total != esperado) {
    printf("ERROR\n");
    return 1;
  }
  printf("OK\n");
  return 0;
}
