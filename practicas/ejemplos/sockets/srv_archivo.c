// srv_archivo.c: servidor de archivos. Protocolo:
//   cliente -> "GET nombre\n"
//   servidor -> "OK <tamaño>\n" + <tamaño> bytes     o     "ERR <motivo>\n"
#include "net.h"

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(9000);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  printf("Servidor de archivos en el puerto 9000 (directorio actual)\n");

  for (;;) {
    sock_t cli = accept(srv, NULL, NULL);
    if (cli == SOCK_INVALIDO)
      continue;

    char peticion[512], respuesta[128];
    if (recibir_linea(cli, peticion, sizeof peticion) >= 0 && strncmp(peticion, "GET ", 4) == 0) {
      const char *nombre = peticion + 4;
      // seguridad: no se permiten rutas, solo archivos del directorio actual
      if (strchr(nombre, '/') || strchr(nombre, '\\') || strstr(nombre, "..")) {
        enviar_todo(cli, "ERR nombre no permitido\n", 24);
      } else {
        FILE *f = fopen(nombre, "rb"); // "b": binario, necesario en Windows
        if (f == NULL) {
          enviar_todo(cli, "ERR no existe\n", 14);
        } else {
          fseek(f, 0, SEEK_END);
          long tam = ftell(f);
          fseek(f, 0, SEEK_SET);
          int n = snprintf(respuesta, sizeof respuesta, "OK %ld\n", tam);
          enviar_todo(cli, respuesta, n);
          char buf[4096];
          size_t leidos;
          while ((leidos = fread(buf, 1, sizeof buf, f)) > 0)
            if (enviar_todo(cli, buf, (int)leidos) < 0)
              break;
          fclose(f);
          printf("enviado %s (%ld bytes)\n", nombre, tam);
        }
      }
    }
    cerrar_socket(cli);
  }
}
