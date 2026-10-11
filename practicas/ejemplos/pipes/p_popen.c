// p_popen.c: ejecutar un comando y leer su salida con popen()
#include <stdio.h>

#ifdef _WIN32
#define COMANDO "dir /b"
#else
#define COMANDO "ls"
#endif

int main(void) {
  FILE *p = popen(COMANDO, "r"); // equivale a: pipe + fork + exec + dup2
  if (p == NULL) {
    perror("popen");
    return 1;
  }
  char linea[256];
  int n = 0;
  while (fgets(linea, sizeof linea, p) != NULL) {
    printf("%3d: %s", ++n, linea);
  }
  int estado = pclose(p);
  printf("El comando listó %d entradas (estado %d)\n", n, estado);
  return 0;
}
