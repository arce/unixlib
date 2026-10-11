// semaforo.h: semáforo contador construido con un mutex y una variable de
// condición. Funciona igual en Linux y macOS (donde sem_init no está
// disponible).
#ifndef SEMAFORO_H
#define SEMAFORO_H

#include "util.h"

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  unsigned valor; // el contador del semáforo
} semaforo_t;

static inline void semaforo_init(semaforo_t *s, unsigned valor) {
  CHECK(pthread_mutex_init(&s->mutex, NULL));
  CHECK(pthread_cond_init(&s->cond, NULL));
  s->valor = valor;
}

// Espera hasta que el contador sea mayor que cero y lo decrementa.
static inline void semaforo_wait(semaforo_t *s) {
  CHECK(pthread_mutex_lock(&s->mutex));
  while (s->valor == 0)
    CHECK(pthread_cond_wait(&s->cond, &s->mutex));
  s->valor--;
  CHECK(pthread_mutex_unlock(&s->mutex));
}

// Incrementa el contador y despierta a un hilo en espera, si lo hay.
static inline void semaforo_post(semaforo_t *s) {
  CHECK(pthread_mutex_lock(&s->mutex));
  s->valor++;
  CHECK(pthread_cond_signal(&s->cond));
  CHECK(pthread_mutex_unlock(&s->mutex));
}

static inline void semaforo_destroy(semaforo_t *s) {
  CHECK(pthread_cond_destroy(&s->cond));
  CHECK(pthread_mutex_destroy(&s->mutex));
}

#endif
