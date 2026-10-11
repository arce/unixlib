// 08_lectores_escritores.c: lectores y escritores con prioridad a los lectores
#include "util.h"

#define LECTORES 4
#define ESCRITORES 2
#define VUELTAS 40

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cambio = PTHREAD_COND_INITIALIZER;
static int lectores = 0;    // cuántos lectores están leyendo ahora
static int escribiendo = 0; // 1 si hay un escritor escribiendo
static int max_lectores = 0;

// La "base de datos": un par de números que siempre deben ser iguales.
// Un escritor los cambia en dos pasos, así que un lector que llegue en medio
// vería a != b.
static int a = 0, b = 0;

static void empezar_lectura(void) {
  CHECK(pthread_mutex_lock(&mutex));
  while (escribiendo) // los lectores solo esperan a un escritor activo
    CHECK(pthread_cond_wait(&cambio, &mutex));
  lectores++;
  if (lectores > max_lectores)
    max_lectores = lectores;
  CHECK(pthread_mutex_unlock(&mutex));
}

static void terminar_lectura(void) {
  CHECK(pthread_mutex_lock(&mutex));
  lectores--;
  if (lectores == 0) // el último lector avisa a los escritores
    CHECK(pthread_cond_broadcast(&cambio));
  CHECK(pthread_mutex_unlock(&mutex));
}

static void empezar_escritura(void) {
  CHECK(pthread_mutex_lock(&mutex));
  while (escribiendo || lectores > 0) // un escritor espera a todos
    CHECK(pthread_cond_wait(&cambio, &mutex));
  escribiendo = 1;
  CHECK(pthread_mutex_unlock(&mutex));
}

static void terminar_escritura(void) {
  CHECK(pthread_mutex_lock(&mutex));
  escribiendo = 0;
  CHECK(pthread_cond_broadcast(&cambio)); // pueden entrar lectores o escritores
  CHECK(pthread_mutex_unlock(&mutex));
}

static int inconsistencias[LECTORES];

static void *lector(void *arg) {
  int id = (int)(intptr_t)arg;
  unsigned semilla = 100 + (unsigned)id;
  for (int i = 0; i < VUELTAS; i++) {
    empezar_lectura();
    if (a != b)
      inconsistencias[id]++;
    pausa_ms(&semilla, 1, 3); // leyendo...
    if (a != b)
      inconsistencias[id]++;
    terminar_lectura();
    pausa_ms(&semilla, 1, 3);
  }
  return NULL;
}

static void *escritor(void *arg) {
  unsigned semilla = 200 + (unsigned)(intptr_t)arg;
  for (int i = 0; i < VUELTAS; i++) {
    empezar_escritura();
    a++;
    pausa_ms(&semilla, 1, 3); // en este momento a != b
    b = a;
    terminar_escritura();
    pausa_ms(&semilla, 1, 5);
  }
  return NULL;
}

int main(void) {
  pthread_t hl[LECTORES], he[ESCRITORES];
  for (int i = 0; i < LECTORES; i++)
    CHECK(pthread_create(&hl[i], NULL, lector, (void *)(intptr_t)i));
  for (int i = 0; i < ESCRITORES; i++)
    CHECK(pthread_create(&he[i], NULL, escritor, (void *)(intptr_t)i));
  for (int i = 0; i < LECTORES; i++)
    CHECK(pthread_join(hl[i], NULL));
  for (int i = 0; i < ESCRITORES; i++)
    CHECK(pthread_join(he[i], NULL));

  int malas = 0;
  for (int i = 0; i < LECTORES; i++)
    malas += inconsistencias[i];
  printf("escrituras: %d (esperadas %d)\n", a, ESCRITORES * VUELTAS);
  printf("máximo de lectores simultáneos: %d\n", max_lectores);
  printf("lecturas inconsistentes: %d\n", malas);
  if (malas != 0 || a != b || a != ESCRITORES * VUELTAS) {
    printf("ERROR\n");
    return 1;
  }
  printf("OK\n");
  return 0;
}
