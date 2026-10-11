// d_ruta.c: directorio actual, cambio de directorio y ruta absoluta
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define RUTA_ABSOLUTA(rel, buf) _fullpath((buf), (rel), sizeof(buf))
#else
#define RUTA_ABSOLUTA(rel, buf) realpath((rel), (buf))
#endif

int main(void) {
  char actual[1024], absoluta[1024];

  if (getcwd(actual, sizeof actual) != NULL)
    printf("directorio actual : %s\n", actual);

  if (RUTA_ABSOLUTA(".", absoluta) != NULL)
    printf("'.' equivale a    : %s\n", absoluta);

  if (chdir("..") == 0 && getcwd(actual, sizeof actual) != NULL)
    printf("después de cd ..  : %s\n", actual);
  else
    perror("chdir");
  return 0;
}
