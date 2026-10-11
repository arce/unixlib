# Temporizadores

> **Nota para Windows:** estos ejemplos usan `alarm`, `sigsuspend`, `fork` y `kill`. Puede ejecutarlos en Windows con TDM-GCC/MinGW-w64 usando la biblioteca **unixlib**: <https://github.com/arce/unixlib>. Basta compilar con `gcc -I<ruta>/include -I<ruta>/compat programa.c -o programa.exe`, sin modificar el código. Los programas que solo miden tiempo o duermen (`t_medir.c` y `t_dormir.c`) compilan sin unixlib.

Casi todo programa necesita medir el tiempo (cuánto tarda una tarea), esperar un tiempo (dormir) o reaccionar cuando un plazo se cumple (un temporizador). Este tutorial presenta las tres cosas con las funciones estándar de C y de Unix: `time`, `clock`, `gettimeofday`, `sleep`, `usleep`, `alarm` y las señales.

## Medir el tiempo

Hay varias funciones, con distinta resolución:

| Función | Encabezado | Resolución | Qué mide |
| ------- | ---------- | ---------- | -------- |
| `time(NULL)` | `<time.h>` | 1 segundo | segundos desde el 1 de enero de 1970 (hora de reloj) |
| `gettimeofday(&tv, NULL)` | `<sys/time.h>` | microsegundos | hora de reloj con fracción de segundo |
| `clock()` | `<time.h>` | `CLOCKS_PER_SEC` | tiempo de **CPU** usado por el programa |

Conviene distinguir el **tiempo real** (el que pasa en el reloj de pared) del **tiempo de CPU** (el que el programa realmente estuvo ejecutándose). Un programa que duerme consume tiempo real pero casi nada de CPU. Para medir cuánto tarda una tarea se toma la hora antes y después, y se resta:

``` c
// t_medir.c: medir el tiempo de una tarea con gettimeofday() y clock()
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static double ahora(void) { // segundos (con fracción) desde un instante fijo
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  double inicio = ahora();
  clock_t cpu0 = clock();

  volatile double suma = 0; // la tarea a medir
  for (long i = 1; i <= 50000000L; i++)
    suma += 1.0 / i;

  double real = ahora() - inicio;
  double cpu = (double)(clock() - cpu0) / CLOCKS_PER_SEC;
  printf("suma = %.6f\n", suma);
  printf("tiempo real: %.3f s, tiempo de CPU: %.3f s\n", real, cpu);
  return 0;
}
```

El ciclo hace mucho trabajo, por lo que el tiempo real y el de CPU deberían ser parecidos. Pruebe cambiar el ciclo por una llamada a `sleep(2)` y compare los dos valores.

## Dormir

`sleep(n)` suspende el proceso `n` segundos y `usleep(u)` lo suspende `u` microsegundos. Mientras duerme, el proceso no consume CPU. Ninguna de las dos garantiza la duración exacta: el sistema operativo puede tardar un poco más en devolver el control, sobre todo para esperas muy cortas. El siguiente programa lo comprueba:

``` c
// t_dormir.c: sleep() y usleep(), y cuánto duermen realmente
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

static double ahora(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  double t0 = ahora();
  sleep(1);
  printf("sleep(1)        tardó %.3f s\n", ahora() - t0);

  t0 = ahora();
  usleep(250000); // 250 000 microsegundos = 250 ms
  printf("usleep(250000)  tardó %.3f s\n", ahora() - t0);

  t0 = ahora();
  for (int i = 0; i < 10; i++)
    usleep(1000); // 10 veces 1 ms: ¿cuánto tarda en realidad?
  printf("10 x usleep(1000) tardó %.3f s (ideal: 0.010)\n", ahora() - t0);
  return 0;
}
```

En Windows las esperas cortas suelen tardar más de lo pedido (la resolución típica del reloj es de unos 15 ms), así que 10 llamadas a `usleep(1000)` pueden tardar bastante más de 10 ms.

## Alarmas: `alarm` y `SIGALRM`

`alarm(n)` le pide al sistema que envíe la señal `SIGALRM` al proceso dentro de `n` segundos, y regresa de inmediato. La alarma **dispara una sola vez**; para repetirla hay que volver a programarla, normalmente desde el propio manejador. `alarm(0)` cancela la alarma pendiente.

Mientras la alarma corre, el programa puede hacer otras tareas o, como en el ejemplo, quedarse dormido en `sigsuspend`, que desbloquea la señal y duerme **de forma atómica**. Así no se puede perder una alarma que llegue justo antes de dormir (un error clásico si se usa `pause()` directamente). La señal se bloquea primero con `sigprocmask`, de modo que solo pueda llegar dentro de `sigsuspend`.

``` c
// t_alarma.c: cuenta regresiva con alarm() y SIGALRM
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t restante = 5;

static void tic(int sig) {
  (void)sig;
  restante--;
  if (restante > 0)
    alarm(1); // reprogramar: alarm() solo dispara una vez
}

int main(void) {
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGALRM);
  sigemptyset(&vacio);

  signal(SIGALRM, tic);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL); // SIGALRM solo llega dentro de sigsuspend
  alarm(1);
  while (restante > 0) {
    printf("faltan %d segundos\n", restante);
    fflush(stdout);
    sigsuspend(&vacio); // desbloquea SIGALRM y duerme, de forma atómica
  }
  printf("¡tiempo!\n");
  return 0;
}
```

## Tareas periódicas

Una tarea periódica es una alarma que se reprograma cada vez. Conviene que el manejador haga lo mínimo (activar una bandera y reprogramar) y que el trabajo real se haga fuera de él, en el programa principal, porque dentro de un manejador solo es seguro llamar a unas pocas funciones (`printf`, por ejemplo, no lo es en general).

``` c
// t_periodico.c: tarea periódica cada 2 segundos con alarm()
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define PERIODO 2
#define VECES 5

static volatile sig_atomic_t pendiente = 0;

static void tic(int sig) {
  (void)sig;
  pendiente = 1; // el manejador solo avisa; el trabajo se hace afuera
  alarm(PERIODO);
}

int main(void) {
  sigset_t bloqueo, vacio;
  sigemptyset(&bloqueo);
  sigaddset(&bloqueo, SIGALRM);
  sigemptyset(&vacio);

  signal(SIGALRM, tic);
  sigprocmask(SIG_BLOCK, &bloqueo, NULL);
  alarm(PERIODO);
  for (int n = 0; n < VECES;) {
    sigsuspend(&vacio); // espera la señal sin perder ninguna
    if (pendiente) {
      pendiente = 0;
      time_t t = time(NULL);
      char hora[32];
      strftime(hora, sizeof hora, "%H:%M:%S", localtime(&t));
      printf("tarea %d ejecutada a las %s\n", ++n, hora);
      fflush(stdout);
    }
  }
  alarm(0); // cancelar la alarma pendiente
  return 0;
}
```

> `setitimer` permite temporizadores con fracciones de segundo, pero solo existe en Unix. Con `alarm` la resolución mínima es de un segundo; para periodos menores use `usleep` dentro de un ciclo y mida con `gettimeofday` el tiempo real transcurrido.

## Esperar con un tiempo límite

Una necesidad frecuente es esperar un evento, pero no más de cierto tiempo (un *timeout*). El siguiente programa crea un hijo que tarda 10 segundos y lo espera como máximo 3: cada 100 ms revisa con `waitpid(..., WNOHANG)` si ya terminó y, si se agota el plazo, lo cancela con `kill`.

``` c
// t_timeout.c: esperar a un proceso hijo como máximo 3 segundos
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define LIMITE_MS 3000

int main(void) {
  pid_t pid = fork();
  if (pid == 0) { // el hijo hace una tarea que tarda 10 segundos
    printf("hijo %d: trabajando 10 segundos...\n", (int)getpid());
    fflush(stdout);
    sleep(10);
    printf("hijo: terminé\n");
    return 0;
  }

  int estado, transcurrido = 0;
  pid_t r;
  while ((r = waitpid(pid, &estado, WNOHANG)) == 0 && transcurrido < LIMITE_MS) {
    usleep(100000); // revisar cada 100 ms
    transcurrido += 100;
  }
  if (r == 0) { // se agotó el tiempo y el hijo sigue vivo
    printf("padre: tiempo agotado (%d ms), cancelo al hijo\n", transcurrido);
    kill(pid, SIGTERM);
    waitpid(pid, &estado, 0);
    if (WIFSIGNALED(estado))
      printf("padre: el hijo terminó por la señal %d\n", WTERMSIG(estado));
  } else {
    printf("padre: el hijo terminó a tiempo\n");
  }
  return 0;
}
```

## Un cronómetro

Combinando una señal con la medición de tiempo se obtiene un cronómetro: corre hasta que el usuario presiona Ctrl+C, que el manejador convierte en una bandera.

``` c
// t_cronometro.c: cronómetro que se detiene con Ctrl+C
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

static volatile sig_atomic_t detener = 0;

static void al_interrumpir(int sig) {
  (void)sig;
  detener = 1;
}

static double ahora(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(void) {
  signal(SIGINT, al_interrumpir);
  printf("Cronómetro en marcha. Presione Ctrl+C para detenerlo.\n");
  double inicio = ahora();
  while (!detener) {
    printf("\r%7.1f s", ahora() - inicio);
    fflush(stdout);
    usleep(100000);
  }
  printf("\nTiempo total: %.2f segundos\n", ahora() - inicio);
  return 0;
}
```

## Ejercicios

1. Escriba un programa que mida e imprima, en segundos y milisegundos, cuánto tarda en ejecutarse un ciclo que suma los números del 1 al 100 millones, usando `gettimeofday`.
2. Escriba un cronómetro de teclado: al presionar Enter comienza a contar, al presionar Enter otra vez se detiene e imprime el tiempo transcurrido con dos decimales.
3. Escriba un programa que reciba por la línea de comandos un número *N* y realice una cuenta regresiva desde *N* hasta 0 usando `alarm` y `SIGALRM`.
4. Escriba un reloj que imprima la hora actual (`time`, `localtime`, `strftime`) cada segundo durante 10 segundos, usando `alarm`.
5. Escriba un programa que cree un hijo que duerma un tiempo aleatorio entre 1 y 8 segundos. El padre lo espera como máximo 4 segundos; si el hijo no termina a tiempo, el padre lo cancela con `kill` e imprime cuánto esperó.
6. Escriba un programa que llame 1000 veces a `usleep(1000)` y calcule el tiempo promedio real de cada llamada. Compare el resultado con el valor esperado (1 ms) y explique la diferencia.
7. Escriba un programa que cree 4 hijos que duran 1, 2, 3 y 4 segundos. El padre mide con `gettimeofday` cuánto tardó cada hijo en terminar (desde que lo creó) y el tiempo total de todo el programa.
8. Escriba un programa de recordatorios: lee líneas con el formato "segundos mensaje" hasta que el usuario escriba "fin". Por cada línea crea un proceso hijo que duerme esa cantidad de segundos e imprime el mensaje.
9. Escriba un programa con un "latido" (*heartbeat*): el hijo envía `SIGUSR1` al padre cada segundo durante 5 latidos y luego termina. El padre reinicia una alarma de 3 segundos con cada latido; si la alarma se agota, imprime "hijo caído".
10. Escriba un programa que muestre en pantalla las líneas de un archivo a razón de *N* líneas por segundo (*N* es un argumento) y que al final imprima la velocidad real medida.
11. Escriba un cronómetro con vueltas: cada vez que el usuario presiona Ctrl+C se registra una vuelta (el manejador solo activa una bandera). A la tercera vuelta el programa termina e imprime los tiempos parcial y total de cada vuelta.
12. Escriba un programa que haga una pregunta aritmética sencilla y le dé 5 segundos al usuario para responder. Si se agota el tiempo, un manejador de `SIGALRM` imprime "tiempo agotado" y termina el programa.
