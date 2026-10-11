# Manejo de señales

> **Nota para Windows:** los ejemplos de este tutorial usan llamadas Unix (`signal`, `sigaction`, `kill`, `alarm`, `pause`). Puede ejecutarlos en Windows con TDM-GCC/MinGW-w64 usando la biblioteca **unixlib**: <https://github.com/arce/unixlib>. Basta compilar con `gcc -I<ruta>/include -I<ruta>/compat programa.c -o programa.exe`, sin modificar el código.

Una señal (signal, sigaction) es una notificación a un proceso que un evento ocurrió. Muchas veces a las señales se les conoce como interrupciones software porque ocurren por requerimiento de usuario o programador. Usualmente las señales son asincrónicas, lo que significa que un proceso no sabe con anticipación que un evento ocurrirá, pero también pueden ser generadas sincrónicamente mediante un error en la aplicación, es el caso de SIGFPE y SIGSEGV. Las señales pueden usarse para comunicación entre usuario y proceso, para terminar o cancelar la ejecución de un proceso, o entre procesos o bien entre sistema y proceso, ejecución de instrucciones ilegales, etc.

Las señales son mensajes enviados por el sistema operativo al proceso en eje- cución. También puede verse como una forma de atender eventos, es decir, permiten interrumpir la ejecución de un proceso para atender la ocurrencia de un evento. Un manejador de evento es el proceso que permite ejecutar el evento que interrumpe, es decir, atender al evento. Normalmente se usa un manejador de evento por defecto que atiende eventos. Por ejemplo, en Unix, cuando se presiona ctrl. C se envía una señal de interrupción al proceso en ejecución, resultando en que el proceso se termina abruptamente.

Algunas de las señales especificadas en el SO son las siguientes:

``` c
    SIGHUP 1 /* hangup */
    SIGINT 2 /* interrupt */
    SIGQUIT 3 /* quit */
    SIGILL 4 /* illegal instruction */
    SIGABRT 6 /* used by abort */
    SIGKILL 9 /* hard kill */
    SIGALRM 14 /* alarm clock */
    SIGCONT 19 /* continue a stopped process */
    SIGCHLD 20 /* to parent on child stop or exit */
    SIGUSR1 21 /* intended for use by user applications */
    SIGUSR2 22 /* intended for use by user applications */
```

Las señales son enumeradas desde el 0 al 31.
Envío de señales
Hay dos llamadas al sistema comunes para enviar señales:

* int kill(int pid, int signal): el proceso que se ejecuta envía una señal al proceso especificado con pid. Si pid = 0 la señal es enviada a todos los procesos, excepto los procesos de sistema Si el valor de retorno == 0 el llamado es exitoso sino se retorna el código de error en la variable errno.
* int raise(int sig) : envía señal sig al proceso en ejecución. Es similar a kill(getpid(), sig)
Manejo de señales
Un proceso puede especificar un manejador (handler) de señales para ser invo- cado cuando se recibe una señal. Cuando un manejador es invocado para recibir una señal, se dice que atiende la señal. Un proceso puede realizar las siguientes acciones cuando recibe la señal:
* Realizar acción por defecto
* Bloquear la señal (algunas pueden ignorarse)
* Atender la señal con un manejador

Los manejadores (handlers) usualmente se ejecutan en la pila del proceso, lo cual implica que el manejador retorna al punto en que la ejecución fue interrumpida.
La función sigaction instala los manejadores de señales de unr proceso. La información sobre el manejador está contenida en una estructura de datos del tipo struct sigaction.

* sigaction(int signum, struct sigaction act, struct sigaction oldact): La llamada del sistema sigaction tiene tres parámetros: el número de señal, un puntero a la estructura sigaction del nuevo manejador y un puntero (también del tipo sigaction) a la estructura del manejador anterior.
La estructura sigaction cuenta con los siguientes miembros:

``` c
    struct sigaction {
       void     (*sa_handler)(int);
       void     (*sa_sigaction)(int, siginfo_t *, void *);
       sigset_t   sa_mask;
       int        sa_flags;
       void     (*sa_restorer)(void);
};
```

## Ejemplo 1

El siguiente ejemplo define un manejador myhandler para tratar la señal. El programa principal establece dicho manejador en la señal SIGUSR1 y posteri- ormente la llamada a raise invoca al manejador.

``` c
#include <signal.h>
#include <stdio.h>

    void myhandler(int a) { printf("In myhandler\n"); }
    int main(void) {
       struct sigaction info, handler;
       handler.sa_handler = &myhandler;
       if (sigaction(SIGUSR1,&handler,&info) != -1)
           printf("New handler set.\n");
       raise(SIGUSR1);
}
```

## Espera de señales

Una de las razones para el empleo de señales es evitar el problema de la espera ocupada. El problema de la espera ocupada implica emplear ciclos de CPU en espera de que ocurra un evento. Lo común es que el programa haga esto examinando el valor de una variable dentro de un ciclo. Un enfoque más eficiente es suspender el proceso hasta que se presente el evento esperado, de modo que los demás procesos puedan hacer uso de la CPU de forma productiva. UNIX proporciona dos funciones que permiten suspender los procesos hasta que ocurra una señal: pause y sigsuspend.

La llamada a pause suspende el proceso que la invoca hasta que sea depositada en éste una señal que no es ignorada por él. Si el proceso atrapa la señal, pause regresa después de que el manejador de señal retorna.

## Ejemplo 2

El siguiente código muestra como un proceso que se alista para recibir una señal de un proceso externo y se queda dormido esperando.

``` c
    #include <signal.h>
    #include <stdio.h>
    #include <unistd.h>
    void myhandler(int a) { printf("Signal Received\n"); }
    int main(void) {
       struct sigaction info, handler;
       handler.sa_handler = &myhandler;
       if (sigaction(SIGINT,&handler,&info) != -1)
           printf("New handler set.\n");
       while (1)
pause(); }
```

A continuación se presenta el código del proceso que envía la señal al proceso que espera. Note que dicha señal es enviada a todos los otros procesos del usuario.

``` c
    #include <signal.h>
    #include <stdio.h>
    int main(void) {
        kill(0,SIGUSR1);
        return 0;
}
```

## Atrapar `SIGINT` (Ctrl+C)

La señal `SIGINT` se genera cuando el usuario presiona Ctrl+C. Por defecto termina el programa, pero se puede instalar un manejador para decidir qué hacer. Un manejador se ejecuta de forma **asíncrona**, interrumpiendo al programa principal en cualquier punto, por lo que se debe mantener lo más simple posible: lo habitual es que solo modifique una variable global y deje el trabajo real al programa principal. Esa variable debe declararse `volatile sig_atomic_t`: `volatile` obliga al compilador a leerla de la memoria cada vez (en lugar de guardarla en un registro) y `sig_atomic_t` garantiza que leerla o escribirla es una operación indivisible. Dentro de un manejador no se deben llamar funciones como `printf` o `malloc`, que no son seguras para señales.

## Ejemplo 3

El siguiente programa cuenta cuántas veces el usuario presiona Ctrl+C y termina a la tercera. El programa principal espera con `pause()`, que suspende el proceso hasta que llegue una señal, en lugar de gastar CPU en una espera ocupada.

``` c
// s_cuenta.c: contar las veces que el usuario presiona Ctrl+C
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t cuenta = 0; // única forma segura de compartir datos con un manejador

static void manejador(int sig) {
  (void)sig;
  cuenta++;
}

int main(void) {
  struct sigaction sa;
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  printf("Presione Ctrl+C tres veces para terminar.\n");
  int mostrada = 0;
  while (cuenta < 3) {
    pause(); // espera una señal sin gastar CPU
    if (cuenta != mostrada) {
      mostrada = cuenta;
      printf("Ctrl+C recibido %d vez/veces\n", mostrada);
      fflush(stdout);
    }
  }
  printf("Total: %d. Adiós.\n", (int)cuenta);
  return 0;
}
```

## Bloquear señales: `sigprocmask` y `sigpending`

Un proceso puede **bloquear** una señal: mientras está bloqueada no se entrega, sino que queda **pendiente** y se entrega apenas se desbloquea. Esto sirve para proteger una sección crítica del programa que no debe ser interrumpida. Las señales se agrupan en conjuntos (`sigset_t`) que se manipulan con `sigemptyset`, `sigaddset` y `sigismember`; `sigprocmask(SIG_BLOCK, &conjunto, NULL)` bloquea las señales del conjunto, `SIG_UNBLOCK` las desbloquea y `sigpending` informa cuáles están pendientes. Bloquear una señal es distinto de ignorarla: una señal ignorada se descarta; una bloqueada se conserva (aunque una señal estándar pendiente no se acumula: si llegan cinco `SIGINT` mientras está bloqueada, se entrega una sola).

## Ejemplo 4

El programa bloquea `SIGINT` durante 5 segundos. Si el usuario presiona Ctrl+C en ese lapso, el programa lo detecta con `sigpending` y el manejador se ejecuta recién cuando se desbloquea.

``` c
// s_mascara.c: bloquear SIGINT con sigprocmask y detectarla con sigpending
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static void manejador(int sig) {
  (void)sig;
  printf("\n[manejador] SIGINT entregada al desbloquearla\n");
}

int main(void) {
  struct sigaction sa;
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  sigset_t bloqueo, pendientes;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGINT);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);

  printf("SIGINT bloqueada durante 5 segundos. Presione Ctrl+C si quiere.\n");
  for (int i = 0; i < 5; i++) {
    printf(".");
    fflush(stdout);
    sleep(1);
  }

  sigpending(&pendientes);
  if (sigismember(&pendientes, SIGINT))
    printf("\nHay una SIGINT pendiente.\n");
  else
    printf("\nNo llegó ninguna SIGINT.\n");

  sigprocmask(SIG_UNBLOCK, &bloqueo, NULL); // si había una pendiente, se entrega aquí
  printf("SIGINT desbloqueada. Fin.\n");
  return 0;
}
```

## Esperar una señal sin perderla: `sigsuspend`

Hay un error clásico al esperar una señal: el programa revisa una bandera (`while (!recibida) pause();`) y la señal llega **justo después** de revisarla y **antes** de ejecutar `pause()`; entonces `pause` duerme para siempre porque la señal ya pasó. La solución es bloquear la señal antes de revisar la bandera y usar `sigsuspend(&mascara_vacia)`, que **desbloquea y duerme en un solo paso indivisible**: si la señal ya estaba pendiente, se entrega de inmediato y la espera termina.

## Ejemplo 5

Este programa une varias ideas: dos procesos juegan ping-pong con `SIGUSR1`, cada uno esperando su turno con `sigsuspend`. La señal se bloquea **antes** de crear al hijo, de modo que ninguna se pierde aunque el hijo todavía no esté listo cuando el padre envía la primera.

``` c
// s_pingpong.c: padre e hijo se turnan con SIGUSR1 (sin perder señales)
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TURNOS 5

static volatile sig_atomic_t recibida = 0;

static void manejador(int sig) {
  (void)sig;
  recibida = 1;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = manejador;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  // Se bloquea SIGUSR1 desde antes de crear al hijo: así ninguna señal se pierde
  // y solo se entrega dentro de sigsuspend().
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGUSR1);
  sigemptyset(&vacio);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);

  pid_t hijo = fork();
  if (hijo == 0) {
    for (int i = 1; i <= TURNOS; i++) {
      while (!recibida)
        sigsuspend(&vacio);
      recibida = 0;
      printf("  hijo : pong %d\n", i);
      fflush(stdout);
      kill(getppid(), SIGUSR1);
    }
    return 0;
  }

  for (int i = 1; i <= TURNOS; i++) {
    printf("padre: ping %d\n", i);
    fflush(stdout);
    kill(hijo, SIGUSR1);
    while (!recibida)
      sigsuspend(&vacio);
    recibida = 0;
  }
  wait(NULL);
  printf("Fin del juego.\n");
  return 0;
}
```

La señal `SIGUSR1` no tiene significado predefinido (está reservada para el uso de las aplicaciones), así que es la más cómoda para este tipo de coordinación entre procesos. Una señal no transporta datos, solo avisa que algo ocurrió; para enviar información además se usa otro mecanismo (un archivo, un pipe o memoria compartida).

## Ejercicios


1. Escriba un programa que instale un manejador para `SIGINT` y cuente cuántas veces el usuario presiona Ctrl+C. Al tercer Ctrl+C debe imprimir el total y terminar. (Use una variable `volatile sig_atomic_t`).
2. Escriba un programa que imprima una cuenta regresiva de 10 a 0 usando `SIGALRM`: el manejador reprograma `alarm(1)` en cada ocurrencia, y el programa principal espera con `pause()`.
3. Implemente su propia función `mi_sleep(n)` usando `alarm()` y `pause()`. Pruébela con un programa que imprima un mensaje cada 2 segundos, 5 veces.
4. Desarrolle dos procesos que jueguen "ping-pong" usando señales. Cuando A recibe `SIGUSR1`, imprime "Ping" y envía `SIGUSR2` a B; cuando B recibe `SIGUSR2`, imprime "Pong" y envía `SIGUSR1` a A. Deben realizar 10 intercambios.
5. Escriba un programa que cree dos procesos hijos que impriman en forma alternada los números pares e impares respectivamente (del 1 al 20). Ambos procesos deben coordinarse utilizando señales. (Recuerde que los diferentes hijos de un proceso no comparten variables de memoria).
6. Cree un programa con un proceso hijo que funcione como reloj, imprimiendo la hora cada segundo con `SIGALRM`. El padre pausa el reloj enviando `SIGUSR1`, lo reanuda con `SIGUSR2` y lo termina con `SIGTERM`.
7. Escriba un programa donde un proceso hijo ejecute un ciclo de trabajo que ignora las primeras 3 señales `SIGTERM` (imprimiendo "intento n ignorado") y termina limpiamente en la cuarta. El padre envía una señal cada 2 segundos.
8. Implemente un productor y un consumidor sincronizados con señales. El productor escribe un número aleatorio en un archivo y envía `SIGUSR1` al consumidor; el consumidor lee el número, lo imprime y responde con `SIGUSR2`, que le indica al productor que puede generar el siguiente. Repita 10 veces.
9. Escriba un programa con tres hijos que imprimen, coordinados con señales y en secuencia, un número, una letra y un símbolo (1, A, #, 2, B, $, ...) hasta completar 5 vueltas.
10. Escriba un programa con un coordinador y 3 trabajadores. Cada trabajador "trabaja" un tiempo aleatorio (1 a 4 segundos) y envía `SIGUSR1` al coordinador. Cuando el coordinador ha recibido las 3 señales, envía `SIGUSR2` a todos los trabajadores para que impriman "fase 2 completada" y terminen.
11. Escriba un programa que bloquee `SIGINT` con `sigprocmask` durante 5 segundos (imprimiendo un punto por segundo), luego lo desbloquee y detecte con `sigpending` si el usuario presionó Ctrl+C durante ese tiempo, imprimiendo el resultado.
12. Escriba una carrera con un juez y 3 corredores (procesos hijos). El juez envía `SIGUSR1` a un corredor elegido al azar cada medio segundo; cada corredor imprime su avance al recibir la señal. El primer corredor que acumule 10 señales gana: el juez lo detecta (el corredor avisa con `SIGUSR2`), anuncia el ganador y termina a los demás.
