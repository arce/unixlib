// demo_arena.c: ejemplo de uso del asignador de pila
#include <stdio.h>

#include "arena.h"

int main(void) {
  int *a = arena_alloc(10 * sizeof(int));
  if (a == NULL) {
    fprintf(stderr, "sin memoria\n");
    return 1;
  }
  for (int i = 0; i < 10; i++)
    a[i] = 10 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", a[i]);
  printf("\nusado: %zu bytes\n", arena_used());

  arena_free(a); // la pila queda vacía otra vez
  printf("usado: %zu bytes\n", arena_used());

  a = arena_alloc(10 * sizeof(int)); // reutiliza el mismo espacio
  for (int i = 0; i < 10; i++)
    a[i] = 20 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", a[i]);
  printf("\n");

  arena_free(a);
  return 0;
}
