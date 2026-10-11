// util.h: utilidades comunes de los ejemplos (Linux, macOS y Windows/MinGW)
#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h> // Sleep
#endif

// Termina el programa con un mensaje si una función de pthread falla.
#define CHECK(llamada)                                                        \
  do {                                                                        \
    int rc_ = (llamada);                                                      \
    if (rc_ != 0) {                                                           \
      fprintf(stderr, "%s:%d: %s falló: %s\n", __FILE__, __LINE__, #llamada,  \
              strerror(rc_));                                                 \
      exit(1);                                                                \
    }                                                                         \
  } while (0)

// Número pseudoaleatorio propio de cada hilo (generador congruencial lineal).
// Se evita rand() (compartido entre hilos) y rand_r() (no existe en Windows).
static inline unsigned aleatorio(unsigned *semilla) {
  *semilla = *semilla * 1103515245u + 12345u;
  return (*semilla >> 16) & 0x7fff;
}

// Duerme el hilo entre min_ms y max_ms milisegundos (simula trabajo).
// Cada hilo debe usar su propia semilla.
static inline void pausa_ms(unsigned *semilla, int min_ms, int max_ms) {
  int ms = min_ms;
  if (max_ms > min_ms)
    ms += (int)(aleatorio(semilla) % (unsigned)(max_ms - min_ms + 1));
#ifdef _WIN32
  Sleep((DWORD)ms);
#else
  struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
  nanosleep(&ts, NULL);
#endif
}

// Hora actual del reloj de pared (la que usa pthread_cond_timedwait).
// En Windows se obtiene con GetSystemTimeAsFileTime, para no depender de que
// la versión de MinGW incluya clock_gettime.
static inline void hora_actual(struct timespec *ts) {
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft); // intervalos de 100 ns desde 1601
  unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) |
                         ft.dwLowDateTime;
  t -= 116444736000000000ULL; // ajusta a 1970
  ts->tv_sec = (time_t)(t / 10000000ULL);
  ts->tv_nsec = (long)(t % 10000000ULL) * 100L;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

// Perro guardián: si el programa sigue vivo pasados `segundos`, imprime el
// mensaje y termina con éxito. Se usa para detectar un interbloqueo, que de
// otro modo dejaría el programa colgado para siempre.
typedef struct {
  int segundos;
  const char *mensaje;
} guardian_t;

static inline void *guardian_hilo(void *arg) {
  const guardian_t *g = arg;
  unsigned semilla = 0;
  pausa_ms(&semilla, g->segundos * 1000, g->segundos * 1000);
  printf("\n%s\n", g->mensaje);
  fflush(stdout);
  exit(0);
  return NULL;
}

static inline void guardian(int segundos, const char *mensaje) {
  static guardian_t g; // debe seguir vivo mientras corra el hilo
  g.segundos = segundos;
  g.mensaje = mensaje;
  pthread_t hilo;
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr));
  CHECK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
  CHECK(pthread_create(&hilo, &attr, guardian_hilo, &g));
  CHECK(pthread_attr_destroy(&attr));
}

#endif
