// a_copia.c: copiar un archivo con open/read/write y un buffer de 4096 bytes
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0 // solo existe en Windows: evita traducir saltos de línea
#endif

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Uso: %s origen destino\n", argv[0]);
    return 1;
  }
  int in = open(argv[1], O_RDONLY | O_BINARY);
  if (in < 0) {
    perror(argv[1]);
    return 1;
  }
  int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
  if (out < 0) {
    perror(argv[2]);
    return 1;
  }

  char buffer[4096];
  long long total = 0;
  ssize_t n;
  clock_t t0 = clock();
  while ((n = read(in, buffer, sizeof buffer)) > 0) { // read puede devolver menos de lo pedido
    if (write(out, buffer, n) != n) {
      perror("write");
      return 1;
    }
    total += n;
  }
  close(in);
  close(out);
  printf("%lld bytes copiados en %.3f s\n", total, (double)(clock() - t0) / CLOCKS_PER_SEC);
  return 0;
}
