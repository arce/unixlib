// cli_lineas.c: cliente de prueba para srv_lineas
#include "net.h"

int main(void) {
  net_iniciar();
  sock_t s = cliente_tcp("127.0.0.1", 8888);
  if (s == SOCK_INVALIDO) {
    net_error("no se pudo conectar");
    return 1;
  }
  char entrada[512], respuesta[512];
  printf("Escriba líneas (QUIT para terminar):\n");
  while (fgets(entrada, sizeof entrada, stdin) != NULL) {
    if (enviar_todo(s, entrada, (int)strlen(entrada)) < 0)
      break;
    if (strncmp(entrada, "QUIT", 4) == 0)
      break;
    if (recibir_linea(s, respuesta, sizeof respuesta) < 0)
      break;
    printf("servidor: %s\n", respuesta);
  }
  cerrar_socket(s);
  return 0;
}
