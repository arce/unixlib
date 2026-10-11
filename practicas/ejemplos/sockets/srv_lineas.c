// srv_lineas.c: servidor que responde cada línea en mayúsculas (protocolo orientado a líneas)
#include "net.h"
#include <ctype.h>

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(8888);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  printf("Servidor escuchando en el puerto 8888\n");

  for (;;) {
    sock_t cli = accept(srv, NULL, NULL);
    if (cli == SOCK_INVALIDO) {
      net_error("accept");
      continue;
    }
    printf("cliente conectado\n");

    char linea[512];
    while (recibir_linea(cli, linea, sizeof linea) >= 0) { // una línea = un mensaje completo
      if (strcmp(linea, "QUIT") == 0)
        break;
      for (char *p = linea; *p; p++)
        *p = (char)toupper((unsigned char)*p);
      strcat(linea, "\n");
      if (enviar_todo(cli, linea, (int)strlen(linea)) < 0)
        break;
    }
    cerrar_socket(cli);
    printf("cliente desconectado\n");
  }
}
