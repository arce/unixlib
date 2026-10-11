// net.h: portabilidad mínima Unix / Windows (Winsock) para los ejemplos de red
#ifndef NET_H
#define NET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 /* Windows Vista o superior: permite WSAPoll */
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define SOCK_INVALIDO INVALID_SOCKET
#define cerrar_socket closesocket
#define poll(fds, n, ms) WSAPoll((fds), (n), (ms)) // poll() de Unix = WSAPoll() de Windows
static inline void net_iniciar(void) {
  WSADATA w;
  if (WSAStartup(MAKEWORD(2, 2), &w) != 0) {
    fprintf(stderr, "WSAStartup falló\n");
    exit(1);
  }
}
static inline void net_error(const char *msg) {
  fprintf(stderr, "%s (error Winsock %d)\n", msg, WSAGetLastError());
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_t;
#define SOCK_INVALIDO (-1)
#define cerrar_socket close
static inline void net_iniciar(void) {}
static inline void net_error(const char *msg) { perror(msg); }
#endif

// Envía exactamente n bytes (send puede enviar menos de los pedidos).
static inline int enviar_todo(sock_t s, const char *datos, int n) {
  int enviados = 0;
  while (enviados < n) {
    int r = send(s, datos + enviados, n - enviados, 0);
    if (r <= 0)
      return -1;
    enviados += r;
  }
  return 0;
}

// Lee una línea (hasta '\n', que se descarta). Devuelve su largo, o -1 si la conexión se cerró.
static inline int recibir_linea(sock_t s, char *buf, int max) {
  int n = 0;
  while (n < max - 1) {
    char c;
    int r = recv(s, &c, 1, 0);
    if (r <= 0)
      return -1;
    if (c == '\n')
      break;
    if (c != '\r')
      buf[n++] = c;
  }
  buf[n] = '\0';
  return n;
}

// Crea un socket TCP de escucha en el puerto indicado.
static inline sock_t servidor_tcp(int puerto) {
  sock_t s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == SOCK_INVALIDO)
    return SOCK_INVALIDO;
  int si = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&si, sizeof si); // reusar el puerto al reiniciar
  struct sockaddr_in dir;
  memset(&dir, 0, sizeof dir);
  dir.sin_family = AF_INET;
  dir.sin_addr.s_addr = htonl(INADDR_ANY);
  dir.sin_port = htons((unsigned short)puerto);
  if (bind(s, (struct sockaddr *)&dir, sizeof dir) != 0 || listen(s, 8) != 0) {
    cerrar_socket(s);
    return SOCK_INVALIDO;
  }
  return s;
}

// Conecta con un servidor TCP en 127.0.0.1 (o la dirección indicada).
static inline sock_t cliente_tcp(const char *ip, int puerto) {
  sock_t s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == SOCK_INVALIDO)
    return SOCK_INVALIDO;
  struct sockaddr_in dir;
  memset(&dir, 0, sizeof dir);
  dir.sin_family = AF_INET;
  dir.sin_addr.s_addr = inet_addr(ip);
  dir.sin_port = htons((unsigned short)puerto);
  if (connect(s, (struct sockaddr *)&dir, sizeof dir) != 0) {
    cerrar_socket(s);
    return SOCK_INVALIDO;
  }
  return s;
}

#endif
