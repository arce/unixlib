// test_mm.c: pruebas del asignador con lista libre
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mm.h"

// Al liberar todo, la memoria debe volver a ser bloques libres sin perder bytes.
static void assert_all_free(void) { assert(mm_free_bytes() == mm_heap_bytes()); }

static void test_basico(void) {
  int *a = mm_malloc(10 * sizeof(int));
  assert(a != NULL);
  assert((uintptr_t)a % 16 == 0); // alineado
  for (int i = 0; i < 10; i++)
    a[i] = i;
  for (int i = 0; i < 10; i++)
    assert(a[i] == i);
  mm_free(a);

  assert_all_free();
  assert(mm_free_blocks() == 1); // un solo bloque: todo se fusionó
  mm_free(NULL);                 // no debe hacer nada

  void *p1 = mm_malloc(64);
  mm_free(p1);
  void *p2 = mm_malloc(64);
  assert(p1 == p2); // reutiliza el bloque liberado
  mm_free(p2);
}

static void test_fusion(void) {
  char *a = mm_malloc(100);
  char *b = mm_malloc(100);
  char *c = mm_malloc(100);
  assert(a && b && c);

  mm_free(a);
  assert(mm_free_blocks() == 2); // hueco de a + resto de la región
  mm_free(c);
  assert(mm_free_blocks() == 2); // c se fusiona con el resto
  mm_free(b);
  assert(mm_free_blocks() == 1); // b se fusiona con ambos lados
  assert_all_free();
}

static void test_extremos(void) {
  assert(mm_malloc(0) == NULL);
  assert(mm_malloc(SIZE_MAX) == NULL);

  size_t n = 1024 * 1024; // más grande que una región normal
  char *big = mm_malloc(n);
  assert(big != NULL);
  memset(big, 0xAB, n);
  mm_free(big);
  assert_all_free();
}

static void test_estrategias(void) {
  // Se crean tres huecos de distinto tamaño separados por bloques en uso:
  //   [a: 300] [s1] [b: 100] [s2] [c: 200] [s3]
  char *a = mm_malloc(300);
  char *s1 = mm_malloc(16);
  char *b = mm_malloc(100);
  char *s2 = mm_malloc(16);
  char *c = mm_malloc(200);
  char *s3 = mm_malloc(16);
  assert(a && s1 && b && s2 && c && s3);
  mm_free(a);
  mm_free(b);
  mm_free(c);

  mm_set_strategy(MM_FIRST_FIT);
  char *p = mm_malloc(90);
  assert(p == a); // el primer hueco que cabe, aunque sea de 300
  mm_free(p);

  mm_set_strategy(MM_BEST_FIT);
  p = mm_malloc(90);
  assert(p == b); // el hueco más ajustado
  mm_free(p);

  mm_set_strategy(MM_FIRST_FIT);
  mm_free(s1);
  mm_free(s2);
  mm_free(s3);
  assert_all_free();
}

#define N 200
static void test_estres(mm_strategy_t strategy) {
  mm_set_strategy(strategy);
  srand(42);
  unsigned char *ptrs[N] = {0};
  size_t sizes[N] = {0};

  for (int iter = 0; iter < 20000; iter++) {
    int i = rand() % N;
    if (ptrs[i] != NULL) { // verificar que nadie pisó los datos y liberar
      for (size_t k = 0; k < sizes[i]; k++)
        assert(ptrs[i][k] == (unsigned char)i);
      mm_free(ptrs[i]);
      ptrs[i] = NULL;
    } else {
      sizes[i] = 1 + (size_t)rand() % 2000;
      ptrs[i] = mm_malloc(sizes[i]);
      assert(ptrs[i] != NULL);
      memset(ptrs[i], i, sizes[i]);
    }
  }
  for (int i = 0; i < N; i++)
    mm_free(ptrs[i]);
  assert_all_free();
  mm_set_strategy(MM_FIRST_FIT);
}

int main(void) {
  test_basico();
  test_fusion();
  test_extremos();
  test_estrategias();
  test_estres(MM_FIRST_FIT);
  test_estres(MM_BEST_FIT);
  puts("test_mm: OK");
  return 0;
}
