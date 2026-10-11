// mux_chat.c: servidor de chat con poll(); todo lo que escribe un cliente lo reciben los demás
#include "net.h"

#define MAX 16

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(8080);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  printf("Chat en el puerto 8080\n");

  struct pollfd fds[MAX + 1];
  int n = 1; // fds[0] es el socket de escucha; fds[1..n-1] son los clientes
  fds[0].fd = srv;
  fds[0].events = POLLIN;

  for (;;) {
    if (poll(fds, n, 5000) < 0) {
      net_error("poll");
      break;
    }

    if (fds[0].revents & POLLIN) { // conexión nueva
      sock_t nuevo = accept(srv, NULL, NULL);
      if (n <= MAX) {
        fds[n].fd = nuevo;
        fds[n].events = POLLIN;
        fds[n].revents = 0;
        n++;
        printf("cliente %d conectado\n", n - 1);
      } else {
        enviar_todo(nuevo, "SERVIDOR LLENO\n", 15);
        cerrar_socket(nuevo);
      }
    }

    for (int i = 1; i < n; i++) {
      if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
        continue;
      char buf[512];
      int r = recv(fds[i].fd, buf, sizeof buf, 0);
      if (r <= 0) { // el cliente se fue: se quita del arreglo
        printf("cliente desconectado\n");
        cerrar_socket(fds[i].fd);
        fds[i] = fds[n - 1];
        n--;
        i--; // revisar la posición que acabamos de rellenar
        continue;
      }
      for (int j = 1; j < n; j++) // retransmitir a todos menos al que escribió
        if (j != i)
          enviar_todo(fds[j].fd, buf, r);
    }
  }
  return 0;
}
