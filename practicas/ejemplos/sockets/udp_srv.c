// udp_srv.c: servidor UDP de eco (sin conexión: cada datagrama es independiente)
#include "net.h"

int main(void) {
  net_iniciar();
  sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in dir;
  memset(&dir, 0, sizeof dir);
  dir.sin_family = AF_INET;
  dir.sin_addr.s_addr = htonl(INADDR_ANY);
  dir.sin_port = htons(7777);
  if (s == SOCK_INVALIDO || bind(s, (struct sockaddr *)&dir, sizeof dir) != 0) {
    net_error("bind");
    return 1;
  }
  printf("Servidor UDP en el puerto 7777\n");

  for (;;) {
    char buf[512];
    struct sockaddr_in origen;
    socklen_t largo = sizeof origen;
    int n = recvfrom(s, buf, sizeof buf - 1, 0, (struct sockaddr *)&origen, &largo);
    if (n < 0)
      continue;
    buf[n] = '\0';
    printf("datagrama de %s:%d: %s\n", inet_ntoa(origen.sin_addr), ntohs(origen.sin_port), buf);
    sendto(s, buf, n, 0, (struct sockaddr *)&origen, largo); // responde al remitente
  }
}
