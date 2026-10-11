// cli_archivo.c: descarga un archivo del servidor srv_archivo
// Uso: cli_archivo nombre
#include "net.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s nombre\n", argv[0]);
    return 1;
  }
  net_iniciar();
  sock_t s = cliente_tcp("127.0.0.1", 9000);
  if (s == SOCK_INVALIDO) {
    net_error("no se pudo conectar");
    return 1;
  }

  char orden[600], cabecera[128];
  snprintf(orden, sizeof orden, "GET %s\n", argv[1]);
  enviar_todo(s, orden, (int)strlen(orden));

  if (recibir_linea(s, cabecera, sizeof cabecera) < 0 || strncmp(cabecera, "OK ", 3) != 0) {
    printf("el servidor respondió: %s\n", cabecera);
    cerrar_socket(s);
    return 1;
  }
  long faltan = atol(cabecera + 3), total = faltan;

  FILE *f = fopen(argv[1], "wb");
  if (f == NULL) {
    perror(argv[1]);
    return 1;
  }
  char buf[4096];
  while (faltan > 0) { // recv devuelve lo que haya llegado: hay que repetir hasta completar
    int pedir = faltan < (long)sizeof buf ? (int)faltan : (int)sizeof buf;
    int r = recv(s, buf, pedir, 0);
    if (r <= 0)
      break;
    fwrite(buf, 1, (size_t)r, f);
    faltan -= r;
  }
  fclose(f);
  cerrar_socket(s);
  printf("%s: recibidos %ld de %ld bytes\n", argv[1], total - faltan, total);
  return faltan == 0 ? 0 : 1;
}
