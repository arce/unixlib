// web_rutas.c: servidor HTTP con rutas, parámetros en la URL, códigos de estado y Content-Type
// Compilar en Windows: gcc web_rutas.c -o web_rutas -lws2_32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(x) closesocket(x)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define PUERTO 8080

// Envía una respuesta HTTP completa: línea de estado, encabezados y cuerpo.
static void responder(int s, int codigo, const char *motivo, const char *tipo, const char *cuerpo, int largo) {
  char cab[256];
  int n = snprintf(cab, sizeof cab,
                   "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
                   "Connection: close\r\n\r\n",
                   codigo, motivo, tipo, largo);
  send(s, cab, n, 0);
  if (largo > 0)
    send(s, cuerpo, largo, 0);
}

static void responder_texto(int s, int codigo, const char *motivo, const char *html) {
  responder(s, codigo, motivo, "text/html; charset=utf-8", html, (int)strlen(html));
}

// Devuelve el Content-Type según la extensión del archivo.
static const char *tipo_mime(const char *nombre) {
  const char *punto = strrchr(nombre, '.');
  if (punto == NULL) return "application/octet-stream";
  if (!strcmp(punto, ".html")) return "text/html; charset=utf-8";
  if (!strcmp(punto, ".css")) return "text/css";
  if (!strcmp(punto, ".txt")) return "text/plain; charset=utf-8";
  if (!strcmp(punto, ".png")) return "image/png";
  if (!strcmp(punto, ".jpg")) return "image/jpeg";
  return "application/octet-stream";
}

// Copia el valor del parámetro 'clave' de la cadena de consulta ("a=1&b=2") en 'valor'.
static int parametro(const char *consulta, const char *clave, char *valor, int max) {
  char buscado[64];
  snprintf(buscado, sizeof buscado, "%s=", clave);
  const char *p = consulta;
  while (p && *p) {
    if (strncmp(p, buscado, strlen(buscado)) == 0) {
      p += strlen(buscado);
      int n = 0;
      while (*p && *p != '&' && n < max - 1) {
        char c = *p++;
        // escapar < > & para que el valor no se interprete como HTML
        if (c == '<' || c == '>' || c == '&') c = '_';
        valor[n++] = c == '+' ? ' ' : c;
      }
      valor[n] = '\0';
      return 1;
    }
    p = strchr(p, '&');
    if (p) p++;
  }
  return 0;
}

static void servir_archivo(int s, const char *nombre) {
  char ruta[300];
  if (strstr(nombre, "..") || strchr(nombre, '\\') || strchr(nombre, '/')) {
    responder_texto(s, 404, "Not Found", "<h1>404</h1><p>Archivo no permitido</p>");
    return;
  }
  snprintf(ruta, sizeof ruta, "www/%s", nombre);
  FILE *f = fopen(ruta, "rb"); // "rb": en Windows evita alterar los bytes del archivo
  if (f == NULL) {
    responder_texto(s, 404, "Not Found", "<h1>404</h1><p>El archivo no existe</p>");
    return;
  }
  fseek(f, 0, SEEK_END);
  long tam = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *datos = malloc((size_t)tam + 1);
  size_t leidos = fread(datos, 1, (size_t)tam, f);
  fclose(f);
  responder(s, 200, "OK", tipo_mime(nombre), datos, (int)leidos);
  free(datos);
}

int main(void) {
#ifdef _WIN32
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
  int srv = (int)socket(AF_INET, SOCK_STREAM, 0);
  int si = 1;
  setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&si, sizeof si);
  struct sockaddr_in dir;
  memset(&dir, 0, sizeof dir);
  dir.sin_family = AF_INET;
  dir.sin_addr.s_addr = htonl(INADDR_ANY);
  dir.sin_port = htons(PUERTO);
  if (bind(srv, (struct sockaddr *)&dir, sizeof dir) != 0 || listen(srv, 8) != 0) {
    perror("bind/listen");
    return 1;
  }
  printf("Servidor en http://localhost:%d/\n", PUERTO);

  for (;;) {
    struct sockaddr_in cli;
    socklen_t largo = sizeof cli;
    int s = (int)accept(srv, (struct sockaddr *)&cli, &largo);
    if (s < 0) continue;

    char peticion[4096];
    int n = recv(s, peticion, sizeof peticion - 1, 0);
    if (n <= 0) {
      close(s);
      continue;
    }
    peticion[n] = '\0';

    char metodo[16], uri[1024];
    if (sscanf(peticion, "%15s %1023s", metodo, uri) != 2) {
      responder_texto(s, 400, "Bad Request", "<h1>400</h1>");
      close(s);
      continue;
    }

    char *consulta = strchr(uri, '?'); // separar "/ruta" de "?a=1&b=2"
    if (consulta) *consulta++ = '\0';

    int codigo = 200;
    if (strcmp(metodo, "GET") != 0) {
      responder_texto(s, 405, "Method Not Allowed", "<h1>405</h1><p>Solo se permite GET</p>");
      codigo = 405;
    } else if (strcmp(uri, "/") == 0) {
      responder_texto(s, 200, "OK", "<h1>Bienvenido</h1><p><a href=\"/hora\">Hora</a> | "
                                    "<a href=\"/saludo?nombre=Ana\">Saludo</a></p>");
    } else if (strcmp(uri, "/hora") == 0) {
      time_t t = time(NULL);
      char hora[64], html[160];
      strftime(hora, sizeof hora, "%d/%m/%Y %H:%M:%S", localtime(&t));
      snprintf(html, sizeof html, "<h1>Hora del servidor</h1><p>%s</p>", hora);
      responder_texto(s, 200, "OK", html);
    } else if (strcmp(uri, "/saludo") == 0) {
      char nombre[64] = "desconocido", html[200];
      if (consulta) parametro(consulta, "nombre", nombre, sizeof nombre);
      snprintf(html, sizeof html, "<h1>Hola, %s</h1>", nombre);
      responder_texto(s, 200, "OK", html);
    } else if (strncmp(uri, "/archivo/", 9) == 0) {
      servir_archivo(s, uri + 9);
    } else {
      responder_texto(s, 404, "Not Found", "<h1>404</h1><p>Ruta inexistente</p>");
      codigo = 404;
    }
    printf("%s %s -> %d\n", metodo, uri, codigo); // registro de la solicitud
    close(s);
  }
}
