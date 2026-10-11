# Servidor HTTP en C

En su esencia, HTTP es un protocolo basado en texto que se asienta sobre TCP, es el protocolo elegido cuando se requieren garantías de entrega. El protocolo se basa en peticiones, generadas por un cliente para iniciar una conversación HTTP, así como en respuestas, que son devueltas por un servidor al cliente.

HTTP define un conjunto de métodos de petición para indicar la acción que se desea realizar para un recurso determinado. Aunque también pueden ser sustantivos, estos métodos de petición se denominan a veces verbos HTTP.

El siguiente programa implementa un servidor web básico en C que maneja el protocolo HTTP de manera directa, demostrando los fundamentos del protocolo HTTP a bajo nivel. La primera sección consiste es el manejo de compatibilidad multiplataforma entre Windows y sistemas Unix/Linux. A través de directivas de preprocesador, el código detecta el sistema operativo y carga las bibliotecas apropiadas de sockets - Winsock2 en Windows y las estándar de POSIX en otros sistemas. Esta abstracción permite que el mismo código funcione en diferentes entornos, definiendo incluso la macro `close(x)` para traducir la función de cierre de sockets entre Windows (`closesocket`) y Unix (`close`).

La inicialización del servidor sigue el flujo clásico de programación con sockets: creación, configuración, enlace y escucha. Primero, se crea un socket con `socket(AF_INET, SOCK_STREAM, 0)`, especificando comunicación IPv4 y protocolo TCP. Luego, se configura la estructura `sockaddr_in` con el puerto 8080 y la dirección INADDR_ANY, permitiendo conexiones desde cualquier interfaz de red. El enlace con `bind()` asocia el socket a esta dirección, mientras que `listen()` lo marca como pasivo, capaz de aceptar conexiones entrantes con un backlog de SOMAXCONN (el máximo permitido por el sistema).

El núcleo del servidor es un bucle infinito que maneja solicitudes de manera secuencial. Para cada conexión, `accept()` bloquea hasta que un cliente se conecta, devolviendo un nuevo descriptor de socket específico para esa comunicación. El programa entonces lee la solicitud HTTP con `recv()`, parsea los componentes básicos (método, URI, versión) usando `sscanf()`, y registra esta información junto con la IP y puerto del cliente. Aunque el código no analiza ni procesa realmente la solicitud, este registro demuestra cómo un servidor real podría enrutar peticiones basándose en estos parámetros.

Finalmente, el servidor responde enviando siempre la misma respuesta HTTP predefinida, que incluye headers válidos y un cuerpo HTML simple. Después de cada respuesta, cierra el socket específico del cliente con `close(newsockfd)` mientras mantiene el socket principal abierto para nuevas conexiones. Este diseño, aunque básico, ilustra el patrón fundamental de todos los servidores web: aceptar conexiones, leer solicitudes, procesarlas y enviar respuestas. La limpieza final con `WSACleanup()` en Windows asegura la liberación adecuada de recursos del subsistema de red, completando el ciclo de vida del servidor.

``` c
// webserver.c
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

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
#if defined _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif

  char buffer[BUFFER_SIZE];
  char resp[] = "HTTP/1.0 200 OK\r\n"
                "Server: webserver-c\r\n"
                "Content-type: text/html\r\n\r\n"
                "<html>hello, world</html>\r\n";

  // Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("webserver (socket)");
    return 1;
  }
  printf("socket created successfully\n");

  // Create the address to bind the socket to
  struct sockaddr_in host_addr;
  int host_addrlen = sizeof(host_addr);

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Create client address
  struct sockaddr_in client_addr;
  int client_addrlen = sizeof(client_addr);

  // Bind the socket to the address
  if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
    perror("webserver (bind)");
    return 1;
  }
  printf("socket successfully bound to address\n");

  // Listen for incoming connections
  if (listen(sockfd, SOMAXCONN) != 0) {
    perror("webserver (listen)");
    return 1;
  }
  printf("server listening for connections\n");

  for (;;) {
    // Accept incoming connections
    int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                           (socklen_t *)&host_addrlen);
    if (newsockfd < 0) {
      perror("webserver (accept)");
      continue;
    }
    printf("connection accepted\n");

    // Get client address
    int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr,
                            (socklen_t *)&client_addrlen);
    if (sockn < 0) {
      perror("webserver (getsockname)");
      continue;
    }

    // Read from the socket
    int valread = recv(newsockfd, buffer, BUFFER_SIZE,0);
    if (valread < 0) {
      perror("webserver (read)");
      continue;
    }

    // Read the request
    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), method, version, uri);

    // Write to the socket
    int valwrite = send(newsockfd, resp, strlen(resp),0);
    if (valwrite < 0) {
      perror("webserver (write)");
      continue;
    }

    close(newsockfd);
  }
	
	#if defined _WIN32
	  WSACleanup();
	#endif

  return 0;
}
```

Para compilar este ejemplo bajo el ambiente Windows se debe ejecutar el siguiente comando de compilación en consola:

> gcc webserver.c -o webserver -lws2_32

Note que debe tener derechos de supervisor para permitir que dicha aplicación abra el socket en la máquina.

## Anatomía de una solicitud y una respuesta

HTTP es un protocolo de texto: el cliente envía una **solicitud** y el servidor devuelve una **respuesta**, ambas con el mismo formato general (una primera línea, encabezados y, opcionalmente, un cuerpo). Una solicitud típica de un navegador es:

```
GET /saludo?nombre=Ana HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0
Accept: text/html
(línea vacía)
```

- La **línea de solicitud** tiene tres partes: el *método* (`GET`, `POST`...), el *URI* (la ruta y, después de `?`, los parámetros) y la versión del protocolo.
- Los **encabezados** tienen el formato `Nombre: valor`. Terminan con una línea vacía (`\r\n\r\n`).
- En un `POST` los datos viajan en el **cuerpo**, después de la línea vacía; el encabezado `Content-Length` dice cuántos bytes tiene.

Y una respuesta:

```
HTTP/1.0 200 OK
Content-Type: text/html
Content-Length: 25
(línea vacía)
<h1>Hola, Ana</h1>
```

La primera línea de la respuesta contiene el **código de estado**. Los más usados son: `200 OK` (todo bien), `400 Bad Request` (solicitud mal formada), `404 Not Found` (la ruta o el archivo no existe) y `405 Method Not Allowed` (el método no está permitido para esa ruta). El encabezado `Content-Type` le dice al navegador cómo interpretar el cuerpo (`text/html`, `text/plain`, `image/png`...), y `Content-Length` cuántos bytes tiene.

## Ejemplo 2: rutas, parámetros y códigos de estado

El servidor anterior responde siempre lo mismo. El siguiente programa separa el método y el URI de la primera línea de la solicitud, separa la ruta de los parámetros (`/saludo?nombre=Ana`), decide la respuesta según la ruta y devuelve el código de estado adecuado. Además, sirve archivos del directorio `www/` enviando el `Content-Type` según la extensión. Se organiza en funciones pequeñas: `responder` arma y envía la respuesta, `tipo_mime` elige el tipo y `parametro` extrae un valor de la cadena de consulta.

``` c
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
```

Para probarlo, compile (en Windows `gcc web_rutas.c -o web_rutas -lws2_32`), cree el directorio `www` con un archivo `prueba.html` y visite `http://localhost:8080/`, `/hora`, `/saludo?nombre=Ana`, `/archivo/prueba.html` y una ruta inexistente. Note dos detalles importantes de seguridad: el nombre de archivo se rechaza si contiene `..` o separadores de ruta (para que nadie pueda leer archivos fuera de `www`), y el valor del parámetro se limpia de los caracteres `<`, `>` y `&` (para que no se interprete como HTML). Los archivos se abren en modo binario (`"rb"`) para que Windows no modifique los bytes de las imágenes.

## Ejemplo 3: formularios y el método POST

Un formulario HTML con `method="POST"` envía los datos en el cuerpo de la solicitud, con el formato `campo=valor`. Como TCP entrega bytes y no mensajes, una solicitud puede llegar partida en varios `recv`: el servidor debe seguir leyendo hasta tener los encabezados completos y tantos bytes de cuerpo como indique `Content-Length`. La función `leer_peticion` hace exactamente eso.

``` c
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
```

Pruébelo en `http://localhost:8080/`: al enviar el formulario, el navegador hace `POST /enviar` y el servidor muestra el cuerpo recibido (por ejemplo `texto=hola`). El contador de visitas se guarda en una variable en memoria, por lo que se pierde al reiniciar el servidor.

## Ejercicios


Realice los siguientes ejercicios únicamente modificando el código mostrando anteriormente (no utilice ninguna librería externa adicional):

1. Desarrollar un servidor que responda con una página HTML básica que muestre: 

   ```
   <h1>Mi Primera Página Web</h1>
   <p>Servidor escrito en C</p>
   <p>Fecha ##/##/## Hora: ##:##</p>". 
   ```

   Donde la fecha y hora deben ser obtenidas del servidor.

2. Implementar un servidor que diferencie entre solicitudes a "/" (respondiendo con una página de bienvenida) y "/about" (respondiendo con información "Acerca de este servidor").

3. Crear un servidor que pueda servir archivos HTML estáticos desde un directorio local, leyendo el contenido del archivo y enviándolo en la respuesta HTTP.

4. Crear un servidor que envíe un listado (en HTML) de todos los archivos del directorio local.

5. Desarrollar un servidor capaz de recibir datos mediante POST y mostrar el contenido recibido en la respuesta (sin procesamiento, solo eco).

6. Crear un servidor que mantenga un contador de visitas (en memoria) y muestre en la página principal: "Esta página ha sido visitada X veces".

7. Desarrollar un servidor que muestre un formulario HTML con un campo de texto y, al enviarlo, muestre los datos recibidos en una nueva página.

8. Implementar un servidor que lleve la cuenta de cuántas peticiones ha realizado cada dirección IP diferente. Mostrar en la respuesta: "Dirección IP: X.X.X.X - Peticiones: N".

9. Modificar el servidor para que procese parámetros en la URL: una solicitud a `/saludo?nombre=Ana` debe responder con una página que diga "Hola, Ana". Si no se envía el parámetro, debe responder "Hola, desconocido".

10. Hacer que el servidor devuelva los códigos de estado correctos: `200 OK` para las rutas conocidas, `404 Not Found` (con una página HTML propia) para rutas inexistentes y `405 Method Not Allowed` para métodos distintos de GET y POST.

11. Modificar el servidor de archivos estáticos para que envíe el encabezado `Content-Type` correcto según la extensión del archivo (`.html`, `.css`, `.txt`, `.png`, `.jpg`).

12. Hacer que el servidor registre cada solicitud en la consola y en un archivo `acceso.log`, con el formato: fecha y hora, dirección IP, método, ruta y código de estado devuelto.
