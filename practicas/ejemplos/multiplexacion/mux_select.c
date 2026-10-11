// mux_select.c: servidor de eco con select() y un tiempo límite de 5 segundos
#include "net.h"

#define MAX 16

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(8081);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  sock_t clientes[MAX];
  int n = 0;
  printf("Servidor select() en el puerto 8081\n");

  for (;;) {
    fd_set lectura; // select modifica el conjunto: hay que reconstruirlo en cada vuelta
    FD_ZERO(&lectura);
    FD_SET(srv, &lectura);
    sock_t mayor = srv;
    for (int i = 0; i < n; i++) {
      FD_SET(clientes[i], &lectura);
      if (clientes[i] > mayor)
        mayor = clientes[i];
    }

    struct timeval limite = {5, 0}; // también se reconstruye en cada vuelta
    int listos = select((int)mayor + 1, &lectura, NULL, NULL, &limite);
    if (listos < 0) {
      net_error("select");
      break;
    }
    if (listos == 0) {
      printf("sin actividad en 5 segundos (%d clientes)\n", n);
      continue;
    }

    if (FD_ISSET(srv, &lectura)) {
      sock_t nuevo = accept(srv, NULL, NULL);
      if (n < MAX)
        clientes[n++] = nuevo;
      else
        cerrar_socket(nuevo);
    }
    for (int i = 0; i < n; i++) {
      if (!FD_ISSET(clientes[i], &lectura))
        continue;
      char buf[512];
      int r = recv(clientes[i], buf, sizeof buf, 0);
      if (r <= 0) {
        cerrar_socket(clientes[i]);
        clientes[i] = clientes[--n];
        i--;
      } else {
        enviar_todo(clientes[i], buf, r);
      }
    }
  }
  return 0;
}
