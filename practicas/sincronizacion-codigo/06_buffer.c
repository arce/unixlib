// 06_buffer.c: productores y consumidores con un buffer acotado
// (un mutex y dos variables de condición)
#include "util.h"

#define K 8 // capacidad del buffer
#define PRODUCTORES 3
#define CONSUMIDORES 2
#define ITEMS 20000 // datos que produce cada productor

typedef struct {
  int datos[K];
  int entra;   // posición donde se coloca el próximo dato
  int sale;    // posición del próximo dato a retirar
  int cuenta;  // cuántos datos hay ahora
  int cerrado; // 1 cuando ya no se producirán más datos
  pthread_mutex_t mutex;
  pthread_cond_t no_lleno;  // hay al menos un espacio libre
  pthread_cond_t no_vacio;  // hay al menos un dato
} buffer_t;

static buffer_t buf = {.mutex = PTHREAD_MUTEX_INITIALIZER,
                       .no_lleno = PTHREAD_COND_INITIALIZER,
                       .no_vacio = PTHREAD_COND_INITIALIZER};
static int max_ocupado = 0;

static void poner(int dato) {
  CHECK(pthread_mutex_lock(&buf.mutex));
  while (buf.cuenta == K) // lleno: esperar un espacio
    CHECK(pthread_cond_wait(&buf.no_lleno, &buf.mutex));
  buf.datos[buf.entra] = dato;
  buf.entra = (buf.entra + 1) % K;
  buf.cuenta++;
  if (buf.cuenta > max_ocupado)
    max_ocupado = buf.cuenta;
  CHECK(pthread_cond_signal(&buf.no_vacio)); // ahora hay un dato
  CHECK(pthread_mutex_unlock(&buf.mutex));
}

// Devuelve 1 y deja el dato en *dato, o 0 si el buffer está cerrado y vacío.
static int sacar(int *dato) {
  CHECK(pthread_mutex_lock(&buf.mutex));
  while (buf.cuenta == 0 && !buf.cerrado) // vacío: esperar un dato
    CHECK(pthread_cond_wait(&buf.no_vacio, &buf.mutex));
  if (buf.cuenta == 0) { // cerrado y vacío: no habrá más datos
    CHECK(pthread_mutex_unlock(&buf.mutex));
    return 0;
  }
  *dato = buf.datos[buf.sale];
  buf.sale = (buf.sale + 1) % K;
  buf.cuenta--;
  CHECK(pthread_cond_signal(&buf.no_lleno)); // ahora hay un espacio
  CHECK(pthread_mutex_unlock(&buf.mutex));
  return 1;
}

// Avisa a todos los consumidores que ya no llegarán más datos.
static void cerrar(void) {
  CHECK(pthread_mutex_lock(&buf.mutex));
  buf.cerrado = 1;
  CHECK(pthread_cond_broadcast(&buf.no_vacio));
  CHECK(pthread_mutex_unlock(&buf.mutex));
}

static void *productor(void *arg) {
  (void)arg;
  for (int v = 1; v <= ITEMS; v++)
    poner(v);
  return NULL;
}

static long sumas[CONSUMIDORES]; // cada consumidor escribe solo en su posición

static void *consumidor(void *arg) {
  int id = (int)(intptr_t)arg;
  int dato;
  while (sacar(&dato))
    sumas[id] += dato;
  return NULL;
}

int main(void) {
  pthread_t prod[PRODUCTORES], cons[CONSUMIDORES];

  for (int i = 0; i < CONSUMIDORES; i++)
    CHECK(pthread_create(&cons[i], NULL, consumidor, (void *)(intptr_t)i));
  for (int i = 0; i < PRODUCTORES; i++)
    CHECK(pthread_create(&prod[i], NULL, productor, NULL));

  for (int i = 0; i < PRODUCTORES; i++)
    CHECK(pthread_join(prod[i], NULL));
  cerrar(); // todos los productores terminaron
  for (int i = 0; i < CONSUMIDORES; i++)
    CHECK(pthread_join(cons[i], NULL));

  long total = 0;
  for (int i = 0; i < CONSUMIDORES; i++) {
    printf("consumidor %d: suma %ld\n", i, sumas[i]);
    total += sumas[i];
  }
  long esperado = (long)PRODUCTORES * ITEMS * (ITEMS + 1) / 2;
  printf("total: %ld, esperado: %ld, ocupación máxima: %d de %d\n", total,
         esperado, max_ocupado, K);
  if (total != esperado || buf.cuenta != 0 || max_ocupado > K) {
    printf("ERROR\n");
    return 1;
  }
  printf("OK\n");
  return 0;
}
