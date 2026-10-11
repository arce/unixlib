// mux_cliente.c: cliente de chat que espera a la vez el teclado y el socket
#include "net.h"
#ifdef _WIN32
#include <conio.h>
#endif

int main(void) {
  net_iniciar();
  sock_t s = cliente_tcp("127.0.0.1", 8080);
  if (s == SOCK_INVALIDO) {
    net_error("no se pudo conectar");
    return 1;
  }
  printf("Conectado. Escriba mensajes y presione Enter.\n");

  for (;;) {
    struct pollfd f[2];
    int cuantos = 1;
    f[0].fd = s;
    f[0].events = POLLIN;
#ifndef _WIN32
    f[1].fd = 0; // en Unix el teclado (stdin) también se puede vigilar con poll
    f[1].events = POLLIN;
    cuantos = 2;
#endif
    if (poll(f, cuantos, 100) < 0)
      break;

    if (f[0].revents & (POLLIN | POLLHUP | POLLERR)) { // llegó algo del servidor
      char buf[512];
      int r = recv(s, buf, sizeof buf, 0);
      if (r <= 0) {
        printf("conexión cerrada por el servidor\n");
        break;
      }
      fwrite(buf, 1, (size_t)r, stdout);
      fflush(stdout);
    }

#ifdef _WIN32
    int hay_teclado = _kbhit(); // en Windows poll() solo vigila sockets: se consulta el teclado aparte
#else
    int hay_teclado = f[1].revents & POLLIN;
#endif
    if (hay_teclado) {
      char linea[512];
      if (fgets(linea, sizeof linea, stdin) == NULL)
        break;
      if (enviar_todo(s, linea, (int)strlen(linea)) < 0)
        break;
    }
  }
  cerrar_socket(s);
  return 0;
}
