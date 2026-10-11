// d_recorrido.c: recorrido recursivo de un directorio
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void recorrer(const char *ruta, int nivel, long long *total, int *archivos) {
  DIR *d = opendir(ruta);
  if (d == NULL) {
    perror(ruta);
    return;
  }
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
      continue;

    char hijo[1024];
    snprintf(hijo, sizeof hijo, "%s/%s", ruta, e->d_name);

    struct stat st;
    if (stat(hijo, &st) == -1) {
      perror(hijo);
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      printf("%*s%s/\n", nivel * 2, "", e->d_name);
      recorrer(hijo, nivel + 1, total, archivos); // recursión sobre el subdirectorio
    } else {
      printf("%*s%s (%lld bytes)\n", nivel * 2, "", e->d_name, (long long)st.st_size);
      *total += st.st_size;
      (*archivos)++;
    }
  }
  closedir(d); // se cierra antes de volver al nivel anterior
}

int main(int argc, char *argv[]) {
  const char *inicio = argc > 1 ? argv[1] : ".";
  long long total = 0;
  int archivos = 0;
  recorrer(inicio, 0, &total, &archivos);
  printf("\n%d archivos, %lld bytes en total\n", archivos, total);
  return 0;
}
