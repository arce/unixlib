// demo_mm.c: ejemplo de uso del asignador con lista libre
#include <stdio.h>

#include "mm.h"

int main(void) {
  int *arreglo = mm_malloc(10 * sizeof(int));
  char *texto = mm_malloc(200);
  double *valores = mm_malloc(5 * sizeof(double));
  if (arreglo == NULL || texto == NULL || valores == NULL)
    return 1;

  for (int i = 0; i < 10; i++)
    arreglo[i] = 10 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", arreglo[i]);
  printf("\n\n");

  printf("Después de tres mm_malloc:\n");
  mm_dump();

  mm_free(texto); // libera el bloque del medio
  printf("\nDespués de liberar texto:\n");
  mm_dump();

  mm_free(arreglo); // se fusiona con el hueco vecino
  printf("\nDespués de liberar arreglo (se fusiona con el hueco):\n");
  mm_dump();

  mm_free(valores); // todo vuelve a ser un único bloque libre
  printf("\nDespués de liberar valores:\n");
  mm_dump();
  return 0;
}
