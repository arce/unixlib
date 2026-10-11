# Tuberías (pipes) en Unix

> **Nota para Windows:** los ejemplos de este tutorial usan llamadas Unix (`fork`, `pipe`, `wait`). Puede ejecutarlos en Windows con TDM-GCC/MinGW-w64 usando la biblioteca **unixlib**: <https://github.com/arce/unixlib>. Basta compilar con `gcc -I<ruta>/include -I<ruta>/compat programa.c -o programa.exe`, sin modificar el código.

En los sistemas operativos Unix y similares, las tuberías o **pipes** son un mecanismo fundamental de comunicación entre procesos. Su función principal es conectar la salida estándar (stdout) de un proceso con la entrada estándar (stdin) de otro, creando así una "cadena de procesamiento" de datos. Este diseño permite que programas simples y especializados, que realizan una única tarea de manera eficiente, puedan combinarse para resolver problemas complejos. Filosóficamente, esto encarna el principio Unix de "hacer una sola cosa y hacerla bien", donde la complejidad surge de la composición de herramientas simples.

Desde una perspectiva técnica, las tuberías crean un bsuffer en el kernel del sistema operativo que actúa como un canal unidireccional de datos. Este mecanismo es más eficiente que usar archivos temporales, ya que la comunicación ocurre en memoria y los procesos se ejecutan de forma concurrente, no secuencial.

En el lenguaje de programación C, el uso de tuberías (pipes) se realiza mediante llamadas al sistema que permiten la comunicación entre procesos relacionados, típicamente entre un proceso padre y sus hijos. La función fundamental es `pipe()`, que recibe como argumento un array de dos enteros (`int fd[2]`) donde se almacenan dos descriptores de archivo: `fd[0]` para el extremo de lectura y `fd[1]` para el extremo de escritura. Una vez creada la tubería, el proceso puede utilizar las operaciones estándar de lectura (`read`) y escritura (`write`) para transferir datos a través de este canal. Es importante recordar que la tubería existe únicamente en memoria y no como un archivo en el sistema de archivos.

## Ejemplo sencillo

La implementación típica involucra la creación de procesos hijos mediante `fork()`. Después de llamar a `pipe()`, el proceso se divide en padre e hijo, y ambos heredan los descriptores de archivo abiertos. Para establecer una comunicación unidireccional, uno de los procesos (por ejemplo, el hijo) cierra el extremo que no usará (`fd[0]` si va a escribir) y el otro proceso cierra el extremo contrario (`fd[1]` si va a leer). Esta coordinación es crucial para evitar condiciones de carrera y asegurar que los datos fluyan en la dirección deseada. El proceso escritor envía datos mediante `write(fd[1], buffer, tamaño)` y el lector los recibe con `read(fd[0], buffer, tamaño)`.

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];  // Array para los descriptores de archivo
    pid_t pid;
    char buffer[100];
    char mensaje[] = "Hola desde el proceso padre!";
    
    // Crear la tubería
    if (pipe(fd) == -1) {
        perror("Error al crear la tubería");
        return 1;
    }
    
    // Crear proceso hijo
    pid = fork();
    
    if (pid == -1) {
        perror("Error en fork");
        return 1;
    }
    
    if (pid == 0) {
        // Código del proceso hijo
        close(fd[1]);  // Cerrar extremo de escritura (no lo usa el hijo)
        
        // Leer mensaje del padre
        int bytes_leidos = read(fd[0], buffer, sizeof(buffer));
        if (bytes_leidos > 0) {
            printf("Proceso hijo recibió: %s\n", buffer);
        }
        
        close(fd[0]);  // Cerrar extremo de lectura
    } else {
        // Código del proceso padre
        close(fd[0]);  // Cerrar extremo de lectura (no lo usa el padre)
        
        // Escribir mensaje al hijo
        printf("Proceso padre enviando: %s\n", mensaje);
        write(fd[1], mensaje, strlen(mensaje) + 1);
        
        close(fd[1]);  // Cerrar extremo de escritura
        
        // Esperar a que el hijo termine
        wait(NULL);
    }
    
    return 0;
}
```



## Ejemplo de redirección

Un patrón avanzado pero común es la redirección de los flujos estándar (stdin, stdout) hacia las tuberías. Esto se logra utilizando `dup2()` para duplicar los descriptores de la tubería sobre los descriptores estándar (0 para stdin, 1 para stdout). Por ejemplo, `dup2(fd[1], STDOUT_FILENO)` redirige la salida estándar del proceso hacia la tubería. Este técnica es fundamental para implementar el equivalente programático del operador `|` del shell, permitiendo que un proceso hijo escriba su salida directamente en la tubería mientras otro proceso padre (o hijo) lee desde ella como si fuera entrada estándar.

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void escritor() {
    // Escribe varias líneas a stdout
    char *mensajes[] = {
        "Manzana", "Banana", "Cereza", "Dátil", "Uva",
        "Fruta del dragón", "Higo", "Kiwi", "Limón", "Mango", NULL
    };
    
    for (int i = 0; mensajes[i] != NULL; i++) {
        printf("%s\n", mensajes[i]);
    }
}

void lector() {
    char linea[100];
    int contador = 0;
    
    // Lee líneas de stdin y las procesa
    while (fgets(linea, sizeof(linea), stdin) != NULL) {
        // Eliminar salto de línea
        linea[strcspn(linea, "\n")] = 0;
        
        // Procesamiento simple: convertir a mayúsculas
        for (int i = 0; linea[i]; i++) {
            if (linea[i] >= 'a' && linea[i] <= 'z') {
                linea[i] = linea[i] - 32;
            }
        }
        
        printf("PROCESADO: %s\n", linea);
        contador++;
    }
    
    printf("Total procesado: %d elementos\n", contador);
}

int main() {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // Hijo 1 - Escritor
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        escritor();
        return 0;
    }

    if (fork() == 0) {
        // Hijo 2 - Lector
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        lector();
        return 0;
    }

    // Padre
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    
    printf("Pipeline finalizado: escritor | lector\n");
    return 0;
}
```

## Comparación de enfoques

Las tuberías pueden ser vistas como un mecanismo adicional de IPC (comunicación entre procesos), sin embargo no se pueden utilizar bajo las mismas condiciones que otros mecanismos como la memoria compartida o las colas de mensajes. A continuación se muestra un resumen de cada uno de estos métodos y sus características más importantes:

**Las Tuberías (Pipes)** ofrecen una comunicación secuencial y ordenada ideal para flujos de datos continuos. Su principal ventaja es la simplicidad de uso y la sincronización automática, donde los procesos se bloquean naturalmente cuando la tubería está llena o vacía. Sin embargo, son inherentemente unidireccionales y el alcance está limitado a procesos con parentesco, lo que restringe su uso en aplicaciones que requieren comunicación bidireccional o entre procesos no relacionados. Además, al estar basadas en flujos de bytes, los programas deben implementar sus propios protocolos para delimitar mensajes individuales.

**La Memoria Compartida** proporciona el mecanismo más rápido para comunicación interprocesos al eliminar la copia de datos entre espacios de direcciones. Permiendo que múltiples procesos accedan directamente a una región común de memoria, es ideal para aplicaciones que requieren alto rendimiento como procesamiento científico o bases de datos. No obstante, esta velocidad tiene un costo: los programadores deben implementar mecanismos explícitos de sincronización usando semáforos o mutex para evitar condiciones de carrera, y existe riesgo de corrupción de datos si la sincronización falla, haciendo el código más complejo y propenso a errores.

**Las Colas de Mensajes** (System V o POSIX) combinan aspectos de ambos enfoques, proporcionando comunicación estructurada con delimitación clara de mensajes y soporte bidireccional natural. Ofrecen ventajas en aplicaciones de arquitectura cliente-servidor donde los mensajes discretos deben mantenerse separados y priorizarse, además de permitir comunicación entre procesos no relacionados. Sin embargo, introducen sobrecarga adicional por la gestión de colas y la copia múltiple de datos, y su implementación puede ser más compleja que las tuberías simples, especialmente al gestionar permisos y persistencia de las colas en el sistema.

## Una cadena de procesos con dos pipes

Un pipe conecta a dos procesos; para formar una cadena (*pipeline*), como `generador | filtro | consumidor`, se necesita un pipe por cada par de procesos consecutivos. Lo delicado es cerrar los extremos que cada proceso no usa: un lector solo detecta el fin de archivo cuando **todos** los extremos de escritura del pipe están cerrados, incluidos los que quedaron abiertos en el padre y en los demás hijos. Una regla útil es que cada proceso empiece cerrando todos los descriptores que no va a utilizar.

El siguiente programa crea un generador de números, un filtro que eleva cada número al cuadrado, y el padre, que consume los resultados. Los datos se envían como enteros binarios (`sizeof(int)` bytes por número) y el fin de archivo se detecta cuando `read` devuelve menos bytes de los pedidos.

``` c
// p_pipeline3.c: generador -> filtro -> padre, conectados por dos pipes
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  int a[2], b[2]; // a: generador -> filtro   b: filtro -> padre
  pipe(a);
  pipe(b);

  if (fork() == 0) { // proceso 1: genera los números 1..10
    close(a[0]);
    close(b[0]);
    close(b[1]);
    for (int i = 1; i <= 10; i++)
      write(a[1], &i, sizeof i);
    close(a[1]); // el filtro verá el fin de archivo
    return 0;
  }

  if (fork() == 0) { // proceso 2: eleva al cuadrado cada número
    close(a[1]);
    close(b[0]);
    int n;
    while (read(a[0], &n, sizeof n) == sizeof n) {
      int cuadrado = n * n;
      write(b[1], &cuadrado, sizeof cuadrado);
    }
    close(a[0]);
    close(b[1]);
    return 0;
  }

  // el padre solo lee de b: cierra todo lo demás
  close(a[0]);
  close(a[1]);
  close(b[1]);
  int v, suma = 0;
  while (read(b[0], &v, sizeof v) == sizeof v) {
    printf("recibido %d\n", v);
    suma += v;
  }
  close(b[0]);
  wait(NULL);
  wait(NULL);
  printf("suma de cuadrados = %d\n", suma);
  return 0;
}
```

## Enviar estructuras por un pipe

Un pipe transporta bytes sin ninguna estructura, por lo que escritor y lector deben acordar el formato. La forma más sencilla de enviar registros es escribir la estructura completa con una sola llamada a `write` y leerla con una sola llamada a `read` del mismo tamaño. Esto funciona entre procesos que corren en la misma máquina y fueron compilados con la misma definición de la estructura (no sería correcto entre máquinas con distinta representación de los datos).

``` c
// p_estructura.c: enviar estructuras completas por un pipe
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct alumno {
  int id;
  char nombre[20];
  float nota;
};

int main(void) {
  int fd[2];
  pipe(fd);

  if (fork() == 0) { // hijo: lector
    close(fd[1]);
    struct alumno a;
    float suma = 0;
    int n = 0;
    while (read(fd[0], &a, sizeof a) == sizeof a) { // una estructura por lectura
      printf("hijo recibió: %d %-8s %.1f\n", a.id, a.nombre, a.nota);
      suma += a.nota;
      n++;
    }
    close(fd[0]);
    if (n > 0)
      printf("promedio del grupo: %.2f\n", suma / n);
    return 0;
  }

  close(fd[0]); // padre: escritor
  struct alumno grupo[3] = {{1, "Ana", 9.0f}, {2, "Luis", 7.5f}, {3, "Marta", 8.5f}};
  for (int i = 0; i < 3; i++)
    write(fd[1], &grupo[i], sizeof grupo[i]);
  close(fd[1]);
  wait(NULL);
  return 0;
}
```

## Ejecutar un comando y leer su salida: `popen`

Crear un pipe, un proceso hijo, redirigir su salida con `dup2` y ejecutar un programa es tan frecuente que la biblioteca estándar lo ofrece como una sola función: `popen(comando, "r")` ejecuta el comando y devuelve un `FILE *` desde el cual se lee su salida (con `"w"`, lo que se escribe llega a la entrada estándar del comando). Se cierra con `pclose`, que además devuelve el estado del comando.

``` c
// p_popen.c: ejecutar un comando y leer su salida con popen()
#include <stdio.h>

#ifdef _WIN32
#define COMANDO "dir /b"
#else
#define COMANDO "ls"
#endif

int main(void) {
  FILE *p = popen(COMANDO, "r"); // equivale a: pipe + fork + exec + dup2
  if (p == NULL) {
    perror("popen");
    return 1;
  }
  char linea[256];
  int n = 0;
  while (fgets(linea, sizeof linea, p) != NULL) {
    printf("%3d: %s", ++n, linea);
  }
  int estado = pclose(p);
  printf("El comando listó %d entradas (estado %d)\n", n, estado);
  return 0;
}
```

El ejemplo cambia el comando según el sistema (`ls` en Unix, `dir /b` en Windows). `popen` es muy cómodo, pero ejecuta el comando a través del intérprete de comandos (*shell*), por lo que nunca se debe construir el comando con texto que escribió un usuario sin validarlo.

## Pipes bloqueantes y pipes con nombre

Un pipe tiene una capacidad limitada (típicamente 64 KB en Linux): si el escritor llena el pipe, `write` se bloquea hasta que el lector consuma datos; si el pipe está vacío, `read` se bloquea hasta que haya datos o se cierren todos los escritores. Esto sincroniza a los procesos automáticamente, pero también puede producir interbloqueos si dos procesos esperan mutuamente (por ejemplo, ambos escribiendo en un pipe lleno sin que nadie lea). Si se escribe en un pipe cuyo extremo de lectura ya se cerró, Unix envía la señal `SIGPIPE` (que termina el proceso, salvo que se atrape o ignore); por eso siempre se debe verificar el valor de retorno de `write`.

Los pipes de este tutorial son **anónimos**: solo pueden compartirse entre procesos emparentados (padre e hijo). Los **pipes con nombre** (*FIFO*, creados con `mkfifo`) aparecen como un archivo especial y permiten comunicar procesos sin parentesco, pero son específicos de Unix y no están disponibles en unixlib.

## Ejercicios


1. Escriba un programa donde el padre lea líneas desde el teclado y las envíe al hijo a través de un pipe. El hijo convierte cada línea a mayúsculas y la imprime. El programa termina cuando el usuario escribe "fin".
2. Cree un programa donde un proceso genere 10 líneas de texto y otro proceso, conectado por un pipe, cuente e imprima el número de líneas, palabras y caracteres recibidos (como el comando `wc`).
3. Establezca comunicación bidireccional entre padre e hijo usando dos pipes. El padre envía 5 números enteros, uno por uno; el hijo multiplica cada uno por 2 y lo devuelve, y el padre imprime cada resultado.
4. Desarrolle un programa donde un proceso genere los números del 1 al 20 y, a través de un pipe, otro proceso filtre y muestre solo los pares.
5. Escriba un programa donde el padre distribuya los números del 1 al 20 entre dos hijos (los pares a uno y los impares al otro) usando pipes separados. Cada hijo calcula la suma de sus números y la devuelve al padre por otro pipe; el padre imprime ambas sumas y el total.
6. Cree una cadena de tres procesos conectados por dos pipes: el primero genera los números del 1 al 10, el segundo calcula el cuadrado de cada uno y el tercero los imprime junto con su suma total.
7. Escriba un programa donde un arreglo de 100 números es dividido por el padre entre 4 hijos. Cada hijo calcula la suma parcial de su segmento y la envía al padre por un pipe compartido; el padre lee las 4 sumas parciales e imprime el total.
8. Escriba un programa donde el padre envíe 10 números enteros desordenados a un hijo por un pipe. El hijo los ordena y los devuelve ordenados por otro pipe, y el padre los imprime.
9. Defina una estructura `struct alumno { int id; char nombre[20]; float nota; }`. El padre envía 5 alumnos por un pipe (con una sola llamada a `write` por estructura) y el hijo los lee e imprime el nombre del alumno con la mejor nota y el promedio del grupo.
10. Escriba un programa en el que el padre escriba un mensaje largo (por ejemplo 1000 bytes) en un pipe y el hijo lo lea en bloques de 64 bytes hasta detectar el fin de archivo (`read` devuelve 0). Imprima el número total de bytes leídos. Cierre correctamente los extremos que no use cada proceso.
11. Use `dup2` para redirigir la salida estándar del hijo hacia un pipe. El hijo ejecuta varios `printf`; el padre lee todo lo que el hijo escribió y lo imprime antepuesto con el prefijo "[hijo] ".
12. Implemente una conversación por turnos entre padre e hijo usando dos pipes: el padre envía una pregunta ("¿Cómo estás?"), el hijo responde, y así sucesivamente durante 5 turnos, con frases distintas en cada turno.
