// a_contador.c: varios procesos incrementan un contador guardado en un archivo
// Uso: a_contador init          (crea el archivo con el valor 0)
//      a_contador               (suma 1000 veces; ejecute 3 copias a la vez)
#include "cerrojo.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

int main(int argc, char *argv[]) {
  int fd = open("contador.dat", O_RDWR | O_CREAT | O_BINARY, 0644);
  if (fd < 0) {
    perror("contador.dat");
    return 1;
  }
  if (argc > 1 && strcmp(argv[1], "init") == 0) {
    int cero = 0;
    write(fd, &cero, sizeof cero);
    close(fd);
    printf("contador inicializado en 0\n");
    return 0;
  }

  for (int i = 0; i < 1000; i++) {
    int valor = 0;
    bloquear(fd, 0, sizeof valor); // sección crítica: leer, modificar y escribir
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &valor, sizeof valor) != sizeof valor)
      valor = 0;
    valor++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &valor, sizeof valor);
    desbloquear(fd, 0, sizeof valor);
  }

  int final = 0;
  lseek(fd, 0, SEEK_SET);
  read(fd, &final, sizeof final);
  printf("proceso terminó; el contador vale ahora %d\n", final);
  close(fd);
  return 0;
}
