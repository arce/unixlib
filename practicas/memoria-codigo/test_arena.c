// test_arena.c: pruebas del asignador de pila
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "arena.h"

int main(void) {
  int *a = arena_alloc(10 * sizeof(int));
  assert(a != NULL);
  assert((uintptr_t)a % 16 == 0); // alineado
  for (int i = 0; i < 10; i++)
    a[i] = i;

  char *b = arena_alloc(100);
  assert(b != NULL && b > (char *)a); // la pila crece hacia arriba
  arena_free(b);

  char *c = arena_alloc(100);
  assert(c == b); // reutiliza el espacio liberado

  arena_free(a); // libera a y todo lo posterior (c)
  assert(arena_used() == 0);

  assert(arena_alloc(0) == NULL);
  assert(arena_alloc(20000) == NULL); // más grande que la pila

  void *big = arena_alloc(9000);
  assert(big != NULL);
  assert(arena_alloc(2000) == NULL); // ya no cabe
  arena_free(big);
  assert(arena_used() == 0);

  puts("test_arena: OK");
  return 0;
}
