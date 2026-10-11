// p_estructura.c: enviar estructuras completas por un pipe
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct alumno {
  int id;
  char nombre[20];
  float nota;
};

int main(void) {
  int fd[2];
  pipe(fd);

  if (fork() == 0) { // hijo: lector
    close(fd[1]);
    struct alumno a;
    float suma = 0;
    int n = 0;
    while (read(fd[0], &a, sizeof a) == sizeof a) { // una estructura por lectura
      printf("hijo recibió: %d %-8s %.1f\n", a.id, a.nombre, a.nota);
      suma += a.nota;
      n++;
    }
    close(fd[0]);
    if (n > 0)
      printf("promedio del grupo: %.2f\n", suma / n);
    return 0;
  }

  close(fd[0]); // padre: escritor
  struct alumno grupo[3] = {{1, "Ana", 9.0f}, {2, "Luis", 7.5f}, {3, "Marta", 8.5f}};
  for (int i = 0; i < 3; i++)
    write(fd[1], &grupo[i], sizeof grupo[i]);
  close(fd[1]);
  wait(NULL);
  return 0;
}
