// udp_cli.c: cliente UDP que espera la respuesta como máximo 2 segundos
// Uso: udp_cli mensaje
#include "net.h"

int main(int argc, char *argv[]) {
  const char *msg = argc > 1 ? argv[1] : "hola";
  net_iniciar();
  sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in srv;
  memset(&srv, 0, sizeof srv);
  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = inet_addr("127.0.0.1");
  srv.sin_port = htons(7777);

  sendto(s, msg, (int)strlen(msg), 0, (struct sockaddr *)&srv, sizeof srv);

  struct pollfd p = {s, POLLIN, 0};
  if (poll(&p, 1, 2000) <= 0) { // UDP no garantiza entrega: siempre hay que usar un tiempo límite
    printf("sin respuesta (el datagrama pudo perderse)\n");
    return 1;
  }
  char buf[512];
  int n = recvfrom(s, buf, sizeof buf - 1, 0, NULL, NULL);
  if (n >= 0) {
    buf[n] = '\0';
    printf("respuesta: %s\n", buf);
  }
  cerrar_socket(s);
  return 0;
}
