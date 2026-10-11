// a_bitacora.c: varios procesos agregan líneas a un mismo archivo de bitácora
#include "cerrojo.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <process.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

int main(void) {
  int fd = open("bitacora.log", O_WRONLY | O_CREAT | O_APPEND | O_BINARY, 0644);
  if (fd < 0) {
    perror("bitacora.log");
    return 1;
  }
  for (int i = 1; i <= 100; i++) {
    char linea[80];
    int n = snprintf(linea, sizeof linea, "proceso %d, línea %d\n", (int)getpid(), i);
    bloquear(fd, 0, 0); // en Windows, O_APPEND por sí solo no es atómico entre procesos
    write(fd, linea, n); // una sola llamada a write por línea
    desbloquear(fd, 0, 0);
  }
  close(fd);
  printf("proceso %d terminó\n", (int)getpid());
  return 0;
}
