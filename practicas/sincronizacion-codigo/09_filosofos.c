// 09_filosofos.c: la cena de los filósofos
//
// Uso: ./09_filosofos mal | orden | camarero
#include "semaforo.h"
#include "util.h"

#define N 5      // filósofos y tenedores
#define COMIDAS 3

static pthread_mutex_t tenedor[N];
static semaforo_t camarero; // deja sentarse a lo sumo a N-1 filósofos
static enum { MAL, ORDEN, CAMARERO } modo;

static void *filosofo(void *arg) {
  int i = (int)(intptr_t)arg;
  unsigned semilla = (unsigned)i + 1;
  int primero = i;           // tenedor de la izquierda
  int segundo = (i + 1) % N; // tenedor de la derecha

  // Solución "orden": todos toman primero el tenedor de menor número.
  if (modo == ORDEN && primero > segundo) {
    primero = (i + 1) % N;
    segundo = i;
  }

  for (int c = 1; c <= COMIDAS; c++) {
    printf("Filósofo %d piensa\n", i);
    pausa_ms(&semilla, 10, 30);

    if (modo == CAMARERO)
      semaforo_wait(&camarero);
    CHECK(pthread_mutex_lock(&tenedor[primero]));
    pausa_ms(&semilla, 50, 50); // da tiempo a que todos tomen el primero
    CHECK(pthread_mutex_lock(&tenedor[segundo]));

    printf("Filósofo %d come (comida %d)\n", i, c);
    pausa_ms(&semilla, 10, 30);

    CHECK(pthread_mutex_unlock(&tenedor[segundo]));
    CHECK(pthread_mutex_unlock(&tenedor[primero]));
    if (modo == CAMARERO)
      semaforo_post(&camarero);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "uso: %s mal|orden|camarero\n", argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "mal") == 0)
    modo = MAL;
  else if (strcmp(argv[1], "orden") == 0)
    modo = ORDEN;
  else if (strcmp(argv[1], "camarero") == 0)
    modo = CAMARERO;
  else {
    fprintf(stderr, "uso: %s mal|orden|camarero\n", argv[0]);
    return 1;
  }
  if (modo == MAL) // si a los 3 segundos sigue vivo, hay un interbloqueo
    guardian(3, "¡INTERBLOQUEO! Todos tienen un tenedor y esperan el otro.");

  for (int i = 0; i < N; i++)
    CHECK(pthread_mutex_init(&tenedor[i], NULL));
  semaforo_init(&camarero, N - 1);

  pthread_t hilos[N];
  for (int i = 0; i < N; i++)
    CHECK(pthread_create(&hilos[i], NULL, filosofo, (void *)(intptr_t)i));
  for (int i = 0; i < N; i++)
    CHECK(pthread_join(hilos[i], NULL));

  printf("Todos comieron %d veces.\n", COMIDAS);
  return 0;
}
