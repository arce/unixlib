# Multiplexación de E/S

Un **pool de conexiones** es un patrón fundamental en el desarrollo de aplicaciones de red en Linux que consiste en mantener un conjunto pre-establecido de conexiones de socket activas y reutilizables. En lugar de abrir y cerrar conexiones para cada operación individual, el pool administra un grupo de conexiones persistentes que están disponibles inmediatamente cuando se necesitan. Este enfoque es particularmente importante en entornos de alto rendimiento donde la sobrecarga de establecer nuevas conexiones TCP repetidamente  puede convertirse en un cuello de botella significativo para la aplicación.

La implementación de un pool de conexiones en Linux typically utiliza mecanismos de multiplexación I/O como **epoll** o **poll** para gestionar múltiples sockets simultáneamente de manera eficiente. Estos sistemas permiten que un solo hilo monitorice decenas de miles de conexiones, notificando solo cuando hay actividad real en algún socket, lo que resulta en un uso óptimo de recursos. El pool no solo maneja la reutilización de conexiones, sino que también implementa mecanismos para verificar el estado de las mismas mediante **keep-alive** o pruebas periódicas, asegurando que las conexiones estén activas antes de ser entregadas para su uso.

El formato de la llamada **pool** se muestra a continuación:

```c
#include <poll.h>

struct pollfd {
    int   fd;         // descriptor de archivo
    short events;     eventos solicitados
    short revents;    // eventos retornados
};

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

## Programa servidor

Este siguiente ejemplo implementa un **servidor de eco concurrente** en Linux utilizando la llamada al sistema `poll()` para manejar múltiples conexiones de clientes de manera eficiente. El servidor crea un socket que escucha en el puerto 8080 y utiliza la multiplexación I/O para monitorizar tanto el socket del servidor como hasta 10 conexiones de clientes simultáneamente. La estructura `pollfd` permite al programa esperar eventos de lectura en todos los descriptores de archivo de manera síncrona pero sin bloqueo activo.

El núcleo de la aplicación reside en el bucle principal que llama periódicamente a `poll()` con un timeout de 5 segundos. Cuando `poll()` retorna, el servidor verifica primero si hay nuevas conexiones entrantes en el socket del servidor (indicado por `POLLIN` en `fds[0].revents`), y en caso afirmativo, acepta la conexión y busca una posición libre en el array para almacenar el nuevo descriptor de socket del cliente. Posteriormente, itera sobre todos los sockets de clientes existentes para comprobar cuáles tienen datos listos para leer, leyendo el mensaje entrante y enviando el mismo contenido de vuelta al cliente en un patrón de eco, mientras maneja adecuadamente los casos de desconexión o error.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    struct pollfd fds[MAX_CLIENTS + 1];
    int timeout = 5000; // 5 segundos
    char buffer[BUFFER_SIZE];
    
    // Crear socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Configurar dirección del servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Servidor escuchando en puerto %d...\n", PORT);
    
    // Inicializar array de pollfd
    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    
    fds[0].fd = server_fd;
    
    while (1) {
        int ret = poll(fds, MAX_CLIENTS + 1, timeout);
        
        if (ret < 0) {
            perror("poll failed");
            break;
        } else if (ret == 0) {
            // Timeout - continuar
            continue;
        }
        
        // Verificar nuevo cliente
        if (fds[0].revents & POLLIN) {
            socklen_t addrlen = sizeof(address);
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            
            if (new_socket >= 0) {
                printf("Nueva conexión aceptada: socket %d\n", new_socket);
                
                // Buscar slot libre
                int slot = -1;
                for (int i = 1; i <= MAX_CLIENTS && slot == -1; i++) {
                    if (fds[i].fd == -1) {
                        slot = i;
                        fds[i].fd = new_socket;
                        fds[i].events = POLLIN;
                    }
                }
                
                if (slot == -1) {
                    printf("No hay slots disponibles\n");
                    close(new_socket);
                }
            }
        }
        
        // Verificar datos de clientes existentes
        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                int bytes_read = read(fds[i].fd, buffer, BUFFER_SIZE - 1);
                
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Recibido de cliente %d: %s", fds[i].fd, buffer);
                    
                    // Echo
                    write(fds[i].fd, buffer, bytes_read);
                } else if (bytes_read == 0) {
                    // Cliente desconectado
                    printf("Cliente %d desconectado\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    perror("read failed");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }
    }
    
    close(server_fd);
    return 0;
}
```

La implementación anterior muestra un patrón clásico de servidor de red donde se evita el bloqueo en operaciones I/O individuales gracias a la multiplexación, permitiendo que un único hilo de ejecución gestione docenas de clientes concurrentes, en lugar de utilizar múltiples hilos.

## Programa cliente

El siguiente programa implementa un cliente en C que se conecta al servidor de eco utilizando sockets TCP. La aplicación comienza configurando la conexión de red mediante la creación de un socket y estableciendo una conexión con el servidor especificado, que por defecto es localhost en el puerto 8080, aunque permite personalizar estos parámetros mediante argumentos de línea de comandos. Una vez establecida la conexión, el cliente proporciona una interfaz interactiva donde el usuario puede enviar mensajes al servidor y recibir respuestas, implementando un patrón de comunicación bidireccional.

Este cliente usa **multiplexación E/S con `poll()`** pero para manejar simultáneamente la entrada del usuario desde la terminal (`STDIN_FILENO`) y los datos entrantes desde el servidor a través del socket. Este método permite que el programa responda inmediatamente tanto a la entrada del usuario como a los mensajes recibidos del servidor sin bloquearse en ninguna operación individual, proporcionando una interacción fluida. El ciclo principal monitoriza constantemente ambos descriptores de archivo, procesando los mensajes de salida del usuario y mostrando los ecos recibidos del servidor en tiempo real.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    struct pollfd fds[2];
    const char *server_ip = SERVER_IP;
    int server_port = SERVER_PORT;
    
    // Permitir especificar IP y puerto por línea de comandos
    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        server_port = atoi(argv[2]);
    }
    
    // Crear socket del cliente
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Conectar al servidor
    printf("Conectando al servidor %s:%d...\n", server_ip, server_port);
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Conexión establecida exitosamente!\n");
    printf("Comandos disponibles:\n");
    printf("  - Escribe cualquier mensaje y recibirás el eco\n");
    printf("  - 'quit' para salir\n");
    printf("  - 'test' para enviar un mensaje de prueba\n");
    printf("----------------------------------------\n");
    
    // Configurar poll para monitorear stdin y el socket
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = client_fd;
    fds[1].events = POLLIN;
    
    while (1) {
        int ret = poll(fds, 2, -1); // Espera indefinida
        
        if (ret < 0) {
            perror("poll failed");
            break;
        }
        
        // Verificar entrada del usuario
        if (fds[0].revents & POLLIN) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }
            
            // Eliminar el salto de línea
            buffer[strcspn(buffer, "\n")] = 0;
            
            if (strcmp(buffer, "quit") == 0) {
                printf("Saliendo...\n");
                break;
            }
            
            // Agregar salto de línea para el servidor
            strcat(buffer, "\n");
            
            // Enviar mensaje al servidor
            int bytes_sent = send(client_fd, buffer, strlen(buffer), 0);
            if (bytes_sent < 0) {
                perror("send failed");
                break;
            }
            
            printf("[Enviado] %d bytes\n", bytes_sent);
        }
        
        // Verificar datos del servidor
        if (fds[1].revents & POLLIN) {
            int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_received < 0) {
                perror("recv failed");
                break;
            } else if (bytes_received == 0) {
                printf("Servidor desconectado\n");
                break;
            }
            
            buffer[bytes_received] = '\0';
            printf("[Eco del servidor]: %s", buffer);
        }
        
        // Verificar errores en el socket
        if (fds[1].revents & (POLLERR | POLLHUP)) {
            printf("Error en la conexión con el servidor\n");
            break;
        }
    }
    
    close(client_fd);
    printf("Conexión cerrada. Hasta pronto!\n");
    return 0;
}
```

## `select`, `poll` y `epoll`

Un servidor con un solo hilo que atiende a varios clientes no puede llamar a `recv` sobre cada socket por turnos, porque `recv` se bloquea hasta que llegue algo y los demás clientes quedarían sin atención. La **multiplexación de E/S** resuelve esto: el programa entrega al sistema operativo la lista de descriptores que le interesan y se bloquea en **una sola llamada** hasta que alguno esté listo para leer. Hay tres interfaces:

| Interfaz | Cómo se indica qué vigilar | Límite | Disponibilidad |
| -------- | -------------------------- | ------ | -------------- |
| `select` | tres conjuntos `fd_set` que se deben reconstruir en cada llamada | `FD_SETSIZE` (normalmente 1024; 64 en Windows) | Unix y Windows |
| `poll` | un arreglo de `struct pollfd` con los eventos pedidos y los obtenidos (`revents`) | sin límite fijo | Unix y Windows (`WSAPoll`) |
| `epoll` | el sistema recuerda la lista; solo devuelve los descriptores listos | sin límite fijo, muy eficiente con miles de conexiones | solo Linux |

El servidor del ejemplo anterior usa directamente `<sys/socket.h>` y `<poll.h>`, por lo que solo compila en Unix; los ejemplos de esta sección son su versión portable. Con pocas conexiones las tres rinden igual; `epoll` solo se justifica con miles de clientes simultáneos. Los ejemplos siguientes usan el encabezado `net.h` del tutorial de sockets, que en Windows traduce `poll` a `WSAPoll` y `close` a `closesocket`, de modo que **el mismo código funciona en Unix y en Windows sin unixlib** (se compila con `-lws2_32`).

## Un servidor de chat con `poll`

El servidor mantiene un arreglo de `struct pollfd`: la posición 0 es el socket de escucha y las demás son los clientes. Cuando `poll` devuelve, hay que revisar **cada** posición: si el socket de escucha tiene datos hay una conexión nueva (se acepta y se agrega al arreglo); si un cliente tiene datos, se leen y se retransmiten a los demás; si `recv` devuelve 0 o un error, el cliente se fue y se quita del arreglo (se copia el último elemento en su lugar). El tiempo límite de `poll` (5 segundos) permite que el servidor haga otras tareas aunque no haya actividad.

``` c
// mux_chat.c: servidor de chat con poll(); todo lo que escribe un cliente lo reciben los demás
#include "net.h"

#define MAX 16

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(8080);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  printf("Chat en el puerto 8080\n");

  struct pollfd fds[MAX + 1];
  int n = 1; // fds[0] es el socket de escucha; fds[1..n-1] son los clientes
  fds[0].fd = srv;
  fds[0].events = POLLIN;

  for (;;) {
    if (poll(fds, n, 5000) < 0) {
      net_error("poll");
      break;
    }

    if (fds[0].revents & POLLIN) { // conexión nueva
      sock_t nuevo = accept(srv, NULL, NULL);
      if (n <= MAX) {
        fds[n].fd = nuevo;
        fds[n].events = POLLIN;
        fds[n].revents = 0;
        n++;
        printf("cliente %d conectado\n", n - 1);
      } else {
        enviar_todo(nuevo, "SERVIDOR LLENO\n", 15);
        cerrar_socket(nuevo);
      }
    }

    for (int i = 1; i < n; i++) {
      if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
        continue;
      char buf[512];
      int r = recv(fds[i].fd, buf, sizeof buf, 0);
      if (r <= 0) { // el cliente se fue: se quita del arreglo
        printf("cliente desconectado\n");
        cerrar_socket(fds[i].fd);
        fds[i] = fds[n - 1];
        n--;
        i--; // revisar la posición que acabamos de rellenar
        continue;
      }
      for (int j = 1; j < n; j++) // retransmitir a todos menos al que escribió
        if (j != i)
          enviar_todo(fds[j].fd, buf, r);
    }
  }
  return 0;
}
```

Para probarlo, ejecute el servidor y conecte dos o tres clientes (con el programa de la siguiente sección, o con `telnet localhost 8080`).

## Un cliente que espera teclado y socket a la vez

Un cliente de chat tiene el mismo problema que el servidor: debe reaccionar tanto a lo que el usuario escribe como a lo que llega del servidor, sin saber cuál ocurrirá primero. En Unix, el teclado es el descriptor 0 y se vigila con `poll` junto con el socket. En Windows, `WSAPoll` solo funciona con sockets, así que el ejemplo consulta el teclado aparte con `_kbhit()`, usando un tiempo límite corto de 100 ms para el `poll` (en ese caso, al detectar una tecla, `fgets` espera a que el usuario termine la línea con Enter).

``` c
// mux_cliente.c: cliente de chat que espera a la vez el teclado y el socket
#include "net.h"
#ifdef _WIN32
#include <conio.h>
#endif

int main(void) {
  net_iniciar();
  sock_t s = cliente_tcp("127.0.0.1", 8080);
  if (s == SOCK_INVALIDO) {
    net_error("no se pudo conectar");
    return 1;
  }
  printf("Conectado. Escriba mensajes y presione Enter.\n");

  for (;;) {
    struct pollfd f[2];
    int cuantos = 1;
    f[0].fd = s;
    f[0].events = POLLIN;
#ifndef _WIN32
    f[1].fd = 0; // en Unix el teclado (stdin) también se puede vigilar con poll
    f[1].events = POLLIN;
    cuantos = 2;
#endif
    if (poll(f, cuantos, 100) < 0)
      break;

    if (f[0].revents & (POLLIN | POLLHUP | POLLERR)) { // llegó algo del servidor
      char buf[512];
      int r = recv(s, buf, sizeof buf, 0);
      if (r <= 0) {
        printf("conexión cerrada por el servidor\n");
        break;
      }
      fwrite(buf, 1, (size_t)r, stdout);
      fflush(stdout);
    }

#ifdef _WIN32
    int hay_teclado = _kbhit(); // en Windows poll() solo vigila sockets: se consulta el teclado aparte
#else
    int hay_teclado = f[1].revents & POLLIN;
#endif
    if (hay_teclado) {
      char linea[512];
      if (fgets(linea, sizeof linea, stdin) == NULL)
        break;
      if (enviar_todo(s, linea, (int)strlen(linea)) < 0)
        break;
    }
  }
  cerrar_socket(s);
  return 0;
}
```

## El mismo servidor con `select`

`select` hace lo mismo con otra interfaz. Los conjuntos `fd_set` se manipulan con `FD_ZERO`, `FD_SET` y `FD_ISSET`, y la llamada **los modifica**: por eso se reconstruyen en cada vuelta del ciclo, igual que la estructura `timeval` del tiempo límite. El primer argumento es el descriptor más grande más uno (Windows lo ignora). Si `select` devuelve 0, el tiempo límite se agotó sin actividad.

``` c
// mux_select.c: servidor de eco con select() y un tiempo límite de 5 segundos
#include "net.h"

#define MAX 16

int main(void) {
  net_iniciar();
  sock_t srv = servidor_tcp(8081);
  if (srv == SOCK_INVALIDO) {
    net_error("no se pudo crear el servidor");
    return 1;
  }
  sock_t clientes[MAX];
  int n = 0;
  printf("Servidor select() en el puerto 8081\n");

  for (;;) {
    fd_set lectura; // select modifica el conjunto: hay que reconstruirlo en cada vuelta
    FD_ZERO(&lectura);
    FD_SET(srv, &lectura);
    sock_t mayor = srv;
    for (int i = 0; i < n; i++) {
      FD_SET(clientes[i], &lectura);
      if (clientes[i] > mayor)
        mayor = clientes[i];
    }

    struct timeval limite = {5, 0}; // también se reconstruye en cada vuelta
    int listos = select((int)mayor + 1, &lectura, NULL, NULL, &limite);
    if (listos < 0) {
      net_error("select");
      break;
    }
    if (listos == 0) {
      printf("sin actividad en 5 segundos (%d clientes)\n", n);
      continue;
    }

    if (FD_ISSET(srv, &lectura)) {
      sock_t nuevo = accept(srv, NULL, NULL);
      if (n < MAX)
        clientes[n++] = nuevo;
      else
        cerrar_socket(nuevo);
    }
    for (int i = 0; i < n; i++) {
      if (!FD_ISSET(clientes[i], &lectura))
        continue;
      char buf[512];
      int r = recv(clientes[i], buf, sizeof buf, 0);
      if (r <= 0) {
        cerrar_socket(clientes[i]);
        clientes[i] = clientes[--n];
        i--;
      } else {
        enviar_todo(clientes[i], buf, r);
      }
    }
  }
  return 0;
}
```

## Ejercicios


1. Crear un servidor de eco que use `poll()` para atender a varios clientes. Cada línea recibida se devuelve precedida del número del cliente, y al desconectarse el cliente el servidor imprime cuántos mensajes envió.
2. Desarrollar un servidor que, al recibir el comando "TIME", devuelva la hora actual del sistema al cliente que lo solicitó. Implementar un timeout de 30 segundos en `poll()`: si no hay actividad, el servidor imprime "sin actividad".
3. Implementar un servidor que reciba operaciones matemáticas simples con el formato "a + b" (suma, resta, multiplicación, división) y devuelva el resultado; debe responder con un mensaje de error en caso de división por cero. Usar `poll()` para atender múltiples clientes.
4. Desarrollar un servidor que permita listar archivos del directorio actual (comando "LIST") y enviar archivos específicos (comando "GET filename"). Usar `poll()` para manejar múltiples clientes.
5. Crear un servidor donde dos clientes puedan jugar piedra, papel o tijera. Usar `poll()` para manejar ambos jugadores y determinar el ganador.
6. Desarrollar un cliente que se conecte a múltiples servidores simultáneamente y envíe el mismo mensaje a todos ellos. Usar `poll()` para manejar las respuestas de todos los servidores.
7. Implementar un servidor que requiera autenticación (usuario/contraseña) antes de permitir el acceso. Usar `poll()` con timeout para desconectar clientes que no se autentiquen en 60 segundos.
8. Crear un servidor de chat donde todos los mensajes recibidos de un cliente sean retransmitidos a todos los demás clientes conectados. Usar `poll()` para gestionar las múltiples conexiones.
9. Implementar un servidor donde múltiples clientes intenten adivinar un número aleatorio. El servidor debe anunciar al ganador y reiniciar el juego. Usar `poll()` para manejar todos los jugadores.
10. Implementar un servidor que mantenga un contador global compartido por todos los clientes. Los comandos "INC", "DEC" y "GET" modifican o devuelven el contador. Usar `poll()` para atender a todos los clientes.
11. Crear un servidor que acepte como máximo 5 clientes simultáneos. Si llega un sexto cliente, el servidor le envía el mensaje "SERVIDOR LLENO" y cierra su conexión. El comando "QUIEN" devuelve cuántos clientes hay conectados.
12. Desarrollar un cliente de chat interactivo que use `poll()` para esperar simultáneamente la entrada del teclado y los mensajes que llegan del servidor, de modo que pueda leer y escribir al mismo tiempo sin hilos.
