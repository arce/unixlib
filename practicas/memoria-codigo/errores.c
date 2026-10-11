// errores.c: los errores clásicos con memoria dinámica
//
// Uso: ./errores fuga | doble-free | uso-despues-de-free | desbordamiento
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fuga(void) {
  char *p = malloc(100);
  strcpy(p, "hola");
  printf("%s\n", p);
  // error: falta free(p)
}

static void doble_free(void) {
  int *p = malloc(sizeof *p);
  *p = 42;
  free(p);
  free(p); // error: ya estaba liberado
}

static void uso_despues_de_free(void) {
  int *p = malloc(4 * sizeof *p);
  p[0] = 42;
  free(p);
  printf("%d\n", p[0]); // error: lee memoria liberada
}

static void desbordamiento(void) {
  int *p = malloc(4 * sizeof *p);
  for (int i = 0; i <= 4; i++) // error: i == 4 se sale del bloque
    p[i] = i;
  free(p);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "uso: %s fuga|doble-free|uso-despues-de-free|desbordamiento\n",
            argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "fuga") == 0)
    fuga();
  else if (strcmp(argv[1], "doble-free") == 0)
    doble_free();
  else if (strcmp(argv[1], "uso-despues-de-free") == 0)
    uso_despues_de_free();
  else if (strcmp(argv[1], "desbordamiento") == 0)
    desbordamiento();
  else
    return 1;
  return 0;
}
