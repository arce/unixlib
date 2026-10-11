# Programación con sockets 

## Creación de sockets

La función socket(domain, type, protocol) crea un punto final de comunicación y retorna un descriptor de archivo que referencia al punto final. El argumento domain especifica un dominio de comunicación, este selecciona la familia de protocolos que serán usados para la comunicación. Estas familias están definidas en la librería . Para el caso particular de Internet se utiliza el dominio AF_INET (IPv4) ó AF_INET6 (IPv6). El argumento type indica la semántica de comuni- cación, que generalmente será SOCK_STREAM que es un protocolo confiable en dos sentidos (TCP). El argumento protocol especifica un protocolo particular a se usado con el socket. Generalmente solo existe un protocolo por lo que normalmente se escribe un 0.

Los sockets SOCK_STREAM son flujos de bytes en dos sentidos. Ellos no preservan las fronteras de registros. Este tipo de sockets debe estar en estado conectado antes de recibir o enviar cualquier dato. Una conexión a otro socket es creada con la función connect(). Una vez conectado, se pueden transferir datos usando las funciones read() y write(), o alguna variante de las funciones send() y recv(). Cuando una sesión ha sido completada se debe ejecutar un close.

La función connect(socket,addr,addrlen) conecta el socket a la dirección especifi- cada por addr. El argumento addrlen especifica el tamaño de addr. El formato de la dirección addr es determinado por el espacio de dirección del socket.

La función send(socket,buffer,size,flags) es usada para transmitir un mensaje (al- macenado en buffer) a otro socket. El send() puede ser usado solo cuando el socket está en estado conectado. Por su parte la función recv(socket,buffer,size,flags) es utilizado para recibir mensajes desde el socket.

``` c
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP. */
#endif
#include <Ws2tcpip.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#if defined _WIN32
#define close(x) closesocket(x)
#endif
int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in server;
  char message[1000], server_reply[2000];
#if defined _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  // Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    printf("Could not create socket");
  }
  puts("Socket created");
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8888);
  // Connect to remote server
  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect failed. Error");
    return 1;
  }
  puts("Connected\n");
  // keep communicating with server
  while (1) {
    printf("Enter message : ");
    scanf("%s", message);
    // Send some data
    if (send(sock, message, strlen(message), 0) < 0) {
      puts("Send failed");
      return 1;
    }
    // Receive a reply from the server

    if (recv(sock, server_reply, 2000, 0) < 0) {
      puts("recv failed");
      break;
    }
    puts("Server reply :");
    puts(server_reply);
  }
  close(sock);
#if defined _WIN32
  WSACleanup();
#endif
  return 0;
}
```

El anterior ejemplo se compila en windows con:

> gcc socket_client.c -lWs2_32 -o socket_client

## Aceptando conexiones

Cuando un socket es creado, este existe en un espacio de nombres (familia de direcciones) pero no tiene una dirección asignada a él. La función bind(socket,addr,addrlen) asigna la dirección especificada por addr al socket. El argumento addrlen especifica el tamaño, en bytes, de la estructura de la dirección. Generalmente, es necesario asignar una dirección local usando bind() antes de que un socket SOCK_STREAM pueda recibir conexiones.
La función listen(socket,backlog) pone a socket en un estado de escucha de conexiones entrantes. El argumento backlog define la cantidad de conexiones pendientes que permitirá el socket.

``` c
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP. */
#endif
#include <Ws2tcpip.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#if defined _WIN32
#define close(x) closesocket(x)
#endif
int main(int argc, char *argv[]) {
  int socket_desc, client_sock, c, read_size;
  struct sockaddr_in server, client;
  char client_message[2000];
#if defined _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
  }
  puts("Socket created");
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);
  // Bind
  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    // print the error message
    perror("bind failed. Error");
    return 1;
  }
  puts("bind done");
  // Listen
  listen(socket_desc, 3);
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);

  // accept connection from an incoming client
  client_sock =
      accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  if (client_sock < 0) {
    perror("accept failed");
    return 1;
  }
  puts("Connection accepted");
  // Receive a message from client
  while ((read_size = recv(client_sock, client_message, 2000, 0)) > 0) {
    // Send the message back to client
    send(client_sock, client_message, strlen(client_message), 0);
  }
  if (read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }
#if defined _WIN32
  WSACleanup();
#endif
  return 0;
}
```

## Sockets con hilos

Para manejar las diferentes conexiones que atiende el proceso servidor se pueden utilizar hilos. Para ello la función pthread_create() debe recibir como parámetro el identificador del socket recién generado.

``` c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP. */
#endif
#include <Ws2tcpip.h>
#include <winsock2.h>

#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#if defined _WIN32
#define close(x) closesocket(x)
#endif
#include <pthread.h>
// the thread function
void *connection_handler(void *);
int main(int argc, char *argv[]) {
  int socket_desc, client_sock, c, *new_sock;
  struct sockaddr_in server, client;
#if defined _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
  }
  puts("Socket created");
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);
  // Bind
  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    // print the error message
    perror("bind failed. Error");
    return 1;
  }
  puts("bind done");

  // Listen
  listen(socket_desc, 3);
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  while ((client_sock = accept(socket_desc, (struct sockaddr *)&client,
                               (socklen_t *)&c))) {
    puts("Connection accepted");
    pthread_t sniffer_thread;
    new_sock = malloc(1);
    *new_sock = client_sock;
    if (pthread_create(&sniffer_thread, NULL, connection_handler,
                       (void *)new_sock) < 0) {
      perror("could not create thread");
      return 1;
    }
    // Now join the thread , so that we dont terminate before the thread
    // pthread_join( sniffer_thread , NULL);
    puts("Handler assigned");
  }
  if (client_sock < 0) {
    perror("accept failed");
    return 1;
  }
#if defined _WIN32
  WSACleanup();
#endif
  return 0;
}
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
  // Get the socket descriptor
  int sock = *(int *)socket_desc;

  int read_size;
  char *message, client_message[2000];
  // Send some messages to the client
  message = "Greetings! I am your connection handler\n";
  send(sock, message, strlen(message), 0);
  message = "Now type something and i shall repeat what you type \n";
  send(sock, message, strlen(message), 0);
  // Receive a message from client
  while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
    // Send the message back to client
    send(sock, client_message, strlen(client_message), 0);
  }
  if (read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }
  // Free the socket pointer
  free(socket_desc);
  return 0;
}
```

## Encabezado de portabilidad `net.h`

Los programas de red de este tutorial se pueden compilar tanto en Unix como en Windows (Winsock). Para no repetir en cada programa los bloques `#ifdef _WIN32`, se agrupan en un encabezado `net.h`. Además de la portabilidad, define cuatro funciones que se usan en todos los ejemplos siguientes: `servidor_tcp` (crea un socket de escucha: `socket`, `SO_REUSEADDR`, `bind` y `listen`), `cliente_tcp` (`socket` y `connect`), `enviar_todo` y `recibir_linea`. El mismo archivo se usa en el tutorial de multiplexación de E/S.

``` c
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
```

Los dos detalles importantes de este encabezado se explican a continuación.

**`send` puede enviar menos de lo pedido.** TCP es un flujo de bytes: `send(s, datos, 1000, 0)` puede devolver 600. `enviar_todo` repite la llamada hasta enviar todos los bytes. De la misma forma, `recv` devuelve lo que haya llegado, que puede ser parte de un mensaje o varios mensajes juntos.

**TCP no conserva los límites de los mensajes.** Si el cliente envía "hola" y luego "mundo", el servidor puede recibir "holamundo" en un solo `recv`. Por eso todo protocolo sobre TCP necesita una forma de delimitar los mensajes: un carácter terminador (como el `\n` de `recibir_linea`), un largo antes de los datos, o ambos.

En Windows, los programas se compilan con `gcc programa.c -o programa -lws2_32`. Por usar Winsock directamente, estos programas **no necesitan unixlib**.

## Un protocolo orientado a líneas

El primer ejemplo es un servidor que recibe líneas de texto y devuelve cada línea en mayúsculas, hasta que el cliente envía `QUIT`. Atiende a un cliente a la vez (para atender a varios simultáneamente se usan hilos, como en la sección anterior, o multiplexación de E/S).

``` c
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
```

El cliente de prueba lee líneas del teclado, las envía y muestra la respuesta:

``` c
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
```

## Transferencia de un archivo

Para enviar un archivo hay que decirle al receptor cuántos bytes esperar, porque el archivo puede contener cualquier byte (incluido `\n`) y no se puede usar un terminador. El protocolo del ejemplo es: el cliente envía la línea `GET nombre`; el servidor responde con la línea `OK <tamaño>` seguida exactamente de ese número de bytes, o con una línea `ERR <motivo>`. Los archivos se abren en modo binario (`"rb"` y `"wb"`), y el servidor rechaza los nombres con `..` o separadores de ruta para no permitir leer archivos fuera de su directorio.

``` c
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
```

El cliente lee la cabecera con `recibir_linea` y luego repite `recv` hasta completar el tamaño anunciado, porque los datos llegan en trozos:

``` c
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
```

Para probarlo, ejecute `srv_archivo` en una consola, y en otra `cli_archivo nombre_de_un_archivo_del_directorio_del_servidor`. Pruebe con un archivo de texto y con una imagen, y compare los resultados.

## Sockets UDP

UDP (`SOCK_DGRAM`) es un protocolo **sin conexión**: no hay `listen`, `accept` ni `connect`; cada mensaje es un *datagrama* independiente que se envía con `sendto` (indicando el destinatario) y se recibe con `recvfrom` (que además informa quién lo envió). A diferencia de TCP, UDP **conserva los límites de los mensajes** (un `recvfrom` recibe un datagrama completo), pero **no garantiza** la entrega, el orden ni la ausencia de duplicados. Es adecuado para mensajes pequeños e independientes (consultas DNS, juegos, mediciones).

``` c
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
```

Como un datagrama puede perderse, el cliente nunca debe esperar la respuesta indefinidamente; el siguiente cliente usa `poll` con un tiempo límite de 2 segundos:

``` c
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
```

## Errores frecuentes

- **`bind` falla con "Address already in use":** el puerto sigue ocupado por una ejecución anterior. Se evita con la opción `SO_REUSEADDR` (ya incluida en `servidor_tcp`).
- **Suponer que un `recv` recibe un mensaje completo:** puede recibir solo una parte; hay que delimitar los mensajes (línea o largo) y repetir la lectura.
- **Olvidar `WSAStartup` en Windows:** cualquier función de sockets falla. Se resuelve con `net_iniciar()`.
- **Usar `perror` en Windows:** no muestra los errores de Winsock; use `WSAGetLastError()` (lo hace `net_error`).
- **Olvidar cerrar los sockets:** cada conexión abierta consume un descriptor; ciérrela con `cerrar_socket` apenas termine de usarla.

## Ejercicios


1. Escriba una aplicación cliente/servidor en C donde el servidor elija un número secreto entre 1 y 100 y el cliente intente adivinarlo. El servidor responde "mayor", "menor" o "acertó", y cuenta los intentos.
2. Escriba una aplicación cliente/servidor en C donde el cliente envíe documentos de texto al servidor y este devolverá un conteo de líneas, palabras y letras.
3. Escriba una aplicación cliente/servidor en C que permita votar por 4 candidatos. El servidor debe mantener las opciones seleccionadas en un archivo. Un segundo programa cliente debe imprimir el resultado de las votaciones.
4. Escriba una aplicación cliente/servidor en C que permita adquirir boletos para un concierto, la cantidad de boletos es limitada (20 boletos). El servidor debe mantener las opciones seleccionadas en un archivo y controlar que no se asignen boletos repetidos. Un segundo programa cliente debe imprimir el número de boleto asignado y el nombre de la persona que lo adquirió.
5. Escriba una aplicación cliente/servidor en C que permita que varias personas se pongan de acuerdo en la hora para realizar una reunión. El servidor debe enviar los posibles horarios disponibles junto con los usuarios que seleccionaron cada horario, y los nuevos clientes podrán seleccionar alguno de los horarios disponibles.
6. Escriba un servidor de eco con un hilo por cliente. Cada hilo cuenta los mensajes de su cliente y el servidor mantiene (con un mutex) el número de clientes conectados, que se informa a cada cliente cuando se conecta.
7. Escriba un servidor de "diccionario" que atienda a cada cliente en un hilo y mantenga una tabla de pares clave/valor en memoria, compartida y protegida con un mutex. Los comandos son "SET clave valor", "GET clave" y "DEL clave".
8. Escriba una aplicación de chat en tiempo real con un hilo por cliente. Todos los mensajes enviados por un cliente son retransmitidos por el servidor a los demás clientes conectados.
9. Escriba una aplicación cliente/servidor en C donde el cliente solicite un archivo por nombre ("GET nombre") y el servidor lo envíe. El cliente lo guarda en disco y el servidor responde con un mensaje de error si el archivo no existe.
10. Escriba una aplicación cliente/servidor en C donde el cliente suba un archivo al servidor ("PUT nombre"). El servidor lo guarda en un directorio propio y mantiene un listado de los archivos recibidos, que envía al cliente con el comando "LIST".
11. Escriba un servidor que lleve un contador de visitas persistente: cada vez que un cliente se conecta le responde "Usted es el visitante número N" y guarda el nuevo valor de N en un archivo, de modo que se conserve al reiniciar el servidor.
12. Escriba un programa cliente de prueba que lance 20 hilos que se conectan simultáneamente a un servidor, envíen un mensaje y midan cuánto tarda cada uno en recibir la respuesta. Al final debe imprimir el tiempo mínimo, máximo y promedio.
