// web_post.c: formulario HTML y recepción de datos con POST
// Compilar en Windows: gcc web_post.c -o web_post -lws2_32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define MAX_PETICION 8192

static void responder(int s, int codigo, const char *motivo, const char *html) {
  char cab[200];
  int n = snprintf(cab, sizeof cab,
                   "HTTP/1.0 %d %s\r\nContent-Type: text/html; charset=utf-8\r\n"
                   "Content-Length: %d\r\nConnection: close\r\n\r\n",
                   codigo, motivo, (int)strlen(html));
  send(s, cab, n, 0);
  send(s, html, (int)strlen(html), 0);
}

// Lee la petición completa: encabezados y, si hay POST, tantos bytes de cuerpo como diga Content-Length.
static int leer_peticion(int s, char *buf, int max, char **cuerpo) {
  int total = 0;
  *cuerpo = NULL;
  while (total < max - 1) {
    int r = recv(s, buf + total, max - 1 - total, 0);
    if (r <= 0) break;
    total += r;
    buf[total] = '\0';
    char *fin = strstr(buf, "\r\n\r\n"); // los encabezados terminan en una línea vacía
    if (fin == NULL) continue;
    *cuerpo = fin + 4;
    char *cl = strstr(buf, "Content-Length:");
    int largo = cl ? atoi(cl + 15) : 0;
    if (total - (int)(*cuerpo - buf) >= largo) break; // el cuerpo ya llegó completo
  }
  return total;
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
  int visitas = 0;

  for (;;) {
    int s = (int)accept(srv, NULL, NULL);
    if (s < 0) continue;

    static char peticion[MAX_PETICION];
    char *cuerpo;
    leer_peticion(s, peticion, sizeof peticion, &cuerpo);

    char metodo[16], uri[256];
    sscanf(peticion, "%15s %255s", metodo, uri);

    if (strcmp(metodo, "GET") == 0 && strcmp(uri, "/") == 0) {
      char html[400];
      snprintf(html, sizeof html,
               "<h1>Formulario</h1>"
               "<form method=\"POST\" action=\"/enviar\">"
               "<input name=\"texto\"> <input type=\"submit\" value=\"Enviar\"></form>"
               "<p>Visitas: %d</p>",
               ++visitas);
      responder(s, 200, "OK", html);
    } else if (strcmp(metodo, "POST") == 0 && strcmp(uri, "/enviar") == 0 && cuerpo) {
      char html[MAX_PETICION + 100], limpio[MAX_PETICION];
      int j = 0; // escapar < y > antes de mostrar lo recibido
      for (char *p = cuerpo; *p && j < (int)sizeof limpio - 6; p++) {
        if (*p == '<') { memcpy(limpio + j, "&lt;", 4); j += 4; }
        else if (*p == '>') { memcpy(limpio + j, "&gt;", 4); j += 4; }
        else limpio[j++] = *p;
      }
      limpio[j] = '\0';
      snprintf(html, sizeof html, "<h1>Datos recibidos</h1><pre>%s</pre><a href=\"/\">Volver</a>", limpio);
      responder(s, 200, "OK", html);
    } else {
      responder(s, 404, "Not Found", "<h1>404</h1>");
    }
    printf("%s %s\n", metodo, uri);
    close(s);
  }
}
