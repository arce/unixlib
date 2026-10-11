// d_gestion.c: crear, renombrar y borrar directorios y archivos
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define CREAR_DIR(ruta) mkdir(ruta) // en Windows mkdir no recibe permisos
#else
#define CREAR_DIR(ruta) mkdir(ruta, 0755)
#endif

static void resultado(const char *accion, int r) {
  printf("%-28s %s\n", accion, r == 0 ? "ok" : "falló");
  if (r != 0)
    perror("   motivo");
}

int main(void) {
  resultado("mkdir prueba", CREAR_DIR("prueba"));

  FILE *f = fopen("prueba/datos.txt", "w");
  if (f != NULL) {
    fputs("hola\n", f);
    fclose(f);
  }

  resultado("rename datos -> final", rename("prueba/datos.txt", "prueba/final.txt"));

  struct stat st;
  if (stat("prueba/final.txt", &st) == 0)
    printf("final.txt mide %lld bytes\n", (long long)st.st_size);

  resultado("rmdir prueba (no vacío)", rmdir("prueba")); // debe fallar: aún tiene un archivo
  resultado("remove final.txt", remove("prueba/final.txt"));
  resultado("rmdir prueba", rmdir("prueba"));
  return 0;
}
