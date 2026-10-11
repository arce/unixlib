// p_pipeline3.c: generador -> filtro -> padre, conectados por dos pipes
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  int a[2], b[2]; // a: generador -> filtro   b: filtro -> padre
  pipe(a);
  pipe(b);

  if (fork() == 0) { // proceso 1: genera los números 1..10
    close(a[0]);
    close(b[0]);
    close(b[1]);
    for (int i = 1; i <= 10; i++)
      write(a[1], &i, sizeof i);
    close(a[1]); // el filtro verá el fin de archivo
    return 0;
  }

  if (fork() == 0) { // proceso 2: eleva al cuadrado cada número
    close(a[1]);
    close(b[0]);
    int n;
    while (read(a[0], &n, sizeof n) == sizeof n) {
      int cuadrado = n * n;
      write(b[1], &cuadrado, sizeof cuadrado);
    }
    close(a[0]);
    close(b[1]);
    return 0;
  }

  // el padre solo lee de b: cierra todo lo demás
  close(a[0]);
  close(a[1]);
  close(b[1]);
  int v, suma = 0;
  while (read(b[0], &v, sizeof v) == sizeof v) {
    printf("recibido %d\n", v);
    suma += v;
  }
  close(b[0]);
  wait(NULL);
  wait(NULL);
  printf("suma de cuadrados = %d\n", suma);
  return 0;
}
