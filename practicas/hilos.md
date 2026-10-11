# Administración de hilos

## Creación de hilos

Inicialmente, la función *main()* del programa provee un único hilo por omisión. Todos los otros hilos deben ser explícitamente creados por el programador.

La función *thread\_create(thread,attr,start_routine,arg)* crea un nuevo hilo y lo hace ejecutable. Esta rutina puede ser llamada muchas veces desde cualquier parte del código. Entre los argumentos de dicha función se encuentran:

* *thread*: un identificador opaco único, para el nuevo hilo retornado por la rutina.
* *attr*: un objeto opaco de atributos que puede ser usado para establecer los atributos del hilo. Se puede especificar este objeto de atributos del hilo, ó NULL para los valores por omisión. 
* *start\_routine*: la rutina en C que el hilo ejecutará una vez que sea creado.
* *arg*: un argumento simple que puede ser pasado a la rutina de inicio. Este debe ser pasado por referencia como un puntero de tipo void. Se puede usar NULL si ningún argumento es requerido.

El número máximo de hilos que pueden ser creados por un proceso es dependiente de la implementación. Los programas que exceden el límite puede fallar o producir resultados erróneos.

Una vez creado los hilos pueden crear otros hilos. No existe ninguna jerarquía o dependencia entre hilos.

``` c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS     5

void *PrintHello(void *threadid) {
   long did;
   did = (long)threadid;
   printf("Hello World! It's me, thread #%ld!\n", did);
   pthread_exit(NULL);
}

int main (int argh, char *argv[]) {
   pthread_t threads[NUM_THREADS];
   int rc;
   long t;
   for(t=0; t<NUM_THREADS; t++) {
      printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
      if (rc) {
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      }
   }

   /* Last thing that main() should do */
   pthread_exit(NULL);
}
```

Note que para compilar programas que utilicen hilos se requiere incluir la librería de hilos POSIX al invocar el compilar, por ejemplo:

> gcc ejemplo.c -o ejemplo -lpthread

### Paso de parámetros

La rutina *pthread\_create* permite que el programador pase un argumento a la rutina que ejecuta el hilo. Para casos donde múltiples hilos deben ser pasados, esta limitación es fácilmente subsanada mediante la creación de una estructura que contiene todos los argumentos, y luego pasando un puntero a la estructura en la rutina *pthread\_create*. Todos los argumentos deben ser pasados por referencia y forzados a (void *).

``` c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS 5

struct thread_data {
  int thread_id;
  int sum;
  char *message;
};

struct thread_data thread_data_array[NUM_THREADS];

void *PrintHello(void *threadarg) {
  struct thread_data *my_data;

  int taskid;
  int sum;
  char *hello_msg;

  my_data = (struct thread_data *)threadarg;
  taskid = my_data->thread_id;
  sum = my_data->sum;
  hello_msg = my_data->message;

  printf("It's me, thread #%d %s %d\n", taskid, hello_msg, sum);
  pthread_exit(NULL);
}

int main(int argh, char *argv[]) {
  pthread_t threads[NUM_THREADS];

  char **messages = (char *[]){"First", "Second", "Third", "Fourth", "Fifth"};

  int rc;
  long t;
  long sum;
  for (t = 0, sum = 0; t < NUM_THREADS; t++, sum += 10) {
    thread_data_array[t].thread_id = t;
    thread_data_array[t].sum = sum;
    thread_data_array[t].message = messages[t];
    rc = pthread_create(&threads[t], NULL, PrintHello,
                        (void *)&thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  /* Last thing that main() should do */
  pthread_exit(NULL);
}
```

## Uniendo y separando hilos

La unión o acople (joining) es una forma de lograr la sincronización entre hilos. La función *pthread\_join(threadid,status)* bloquea el hilo llamador hasta que el hilo especificado por *threadid* termina. El programador es capaz de obtener el estado retornado por el hilo terminado siempre y cuando dicho estado sea retornado en la llamada *pthread\_exit()*.

Cuando un hilo es creado, uno de sus atributos define si este será *acoplable* (joinable) ó *separado* (detached). Solo un hilo declarado *acoplable* puede ser acoplado. Si un hilo es creado como *separado* este no puede ser acoplado.

La rutina *pthread\_detach()* se utiliza para explícitamente desacoplar un hilo inclusive si fue creado como *acoplable* (joinable).

``` c
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS 4

void *BusyWork(void *t) {
  int i;
  long tid;
  double result = 0.0;
  tid = (long)t;
  printf("Thread %ld starting...\n", tid);
  for (i = 0; i < 1000000; i++) {
    result = result + sin(i) * tan(i);
  }
  printf("Thread %ld done. Result = %e\n", tid, result);
  pthread_exit((void *)t);
}

int main(int argc, char *argv[]) {
  pthread_t thread[NUM_THREADS];
  pthread_attr_t attr;
  int rc;
  long t;
  void *status;

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (t = 0; t < NUM_THREADS; t++) {
    printf("Main: creating thread %ld\n", t);
    rc = pthread_create(&thread[t], &attr, BusyWork, (void *)t);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);
  for (t = 0; t < NUM_THREADS; t++) {
    rc = pthread_join(thread[t], &status);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
    printf("Main: completed join with thread %ld having a status of %ld\n", t,
           (long)status);
  }

  printf("Main: program completed. Exiting.\n");
  pthread_exit(NULL);
}
```

## Hilos asíncronos

El siguiente ejemplo muestra cómo crear múltiples hilos de forma asíncrona. Se utilizan cuatro llamadas de hilos:

*pthread\_create* crea un hilo ejecutando la rutina especificada en el tercer argumento (*alarm_thread*), retornando un identificador para el nuevo hilo y almacenándolo en una variable *thread*. *pthread\_detach* permite reclamar los recursos del hilo tan pronto como terminen. *pthread\_exit* termina el hilo, y *pthread\_self* que retorna el identificador del hilo invocador.

``` c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __unix__
#include <unistd.h>
#elif defined _WIN32
#include "windows.h"
#define sleep(x) Sleep(1000 * x)
#endif

typedef struct alarm_tag {
  int seconds;
  char message[64];
} alarm_t;

void *alarm_thread(void *arg) {
  alarm_t *alarm = (alarm_t *)arg;
  int status;

  status = pthread_detach(pthread_self());
  if (status != 0) {
    printf("ERROR; Detach thread, status %d\n", status);
    exit(-1);
  }
  sleep(alarm->seconds);
  printf("(%d) %s\n", alarm->seconds, alarm->message);
  free(alarm);
  return NULL;
}

int main(int argc, char *argv[]) {
  int status;
  char line[128];
  alarm_t *alarm;
  pthread_t thread;

  while (1) {
    printf("Alarm> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
      exit(0);
    if (strlen(line) <= 1)
      continue;
    alarm = (alarm_t *)malloc(sizeof(alarm_t));
    if (alarm == NULL) {
      printf("ERROR; Allocate alarm");
      exit(-1);
    }
    if (sscanf(line, "%d %64[^\n]", &alarm->seconds, alarm->message) < 2) {
      fprintf(stderr, "Bad command\n");
      free(alarm);
    } else {
      status = pthread_create(&thread, NULL, alarm_thread, alarm);
      if (status != 0) {
        printf("ERROR; create alarm thread, status %d\n", status);
        exit(-1);
      }
    }
  }
}
```

## Cancelación de hilos

La función *int pthread\_cancel(thread)* envía una solicitud de cancelación al hilo especificado. El hecho de que el hilo destino reaccione ó no a la solicitud depende de dos atributos que están bajo control del hilo: su estado de cancelabilidad y el tipo.

El estado de cancelabilidad es determinado por la función *int pthread\_setcancelstate(state, oldstate)* y debe ser: *PTHREAD_CANCEL_ENABLE* ó *PTHREAD_CANCEL_DISABLE*. El valor por omisión es que el hilo sea cancelable. Por su parte el estado de cancelabilidad es determinado por la función *int pthread\_setcanceltype(type, oldtype)* y el parámetro debe ser: *PTHREAD_CANCEL_DEFERRED* ó *PTHREAD_CANCEL_ASYNCHRONOUS*. El valor por omisión es que la cancelación sea diferida, es decir, que la cancelación se lleva a cabo hasta que se invoque una función del tipo *punto de cancelación* (cancellation point). En el otro caso la cancelación se aplica de inmediato.

### Punto de cancelación

La forma de definir un punto de cancelación es invocar la función *pthread\_testcancel()*. En este caso si existiera alguna solicitud de cancelación pendiente se producirá la cancelación del hilo, sino el llamado sería ignorado. Existen otros tipos de funciones que se consideran puntos de cancelación por sí mismas. En general estas funciones involucran acceso a archivos, terminales, sockets, señales, mensajes, y esperas.

``` c
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __unix__
#include <unistd.h>
#elif defined _WIN32
#include "windows.h"
#define sleep(x) Sleep(1000 * x)
#endif

#define handle_error_en(en, msg)                                               \
  do {                                                                         \
    errno = en;                                                                \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

static void *thread_func(void *ignored_argument) {
  int s;

  /* Disable cancellation for a while, so that we don't
     immediately react to a cancellation request */

  s = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  if (s != 0)
    handle_error_en(s, "pthread_setcancelstate");

  printf("thread_func(): started; cancellation disabled\n");
  sleep(5);
  printf("thread_func(): about to enable cancellation\n");

  s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (s != 0)
    handle_error_en(s, "pthread_setcancelstate");

  while (1) {
    printf("thread_func(): not canceled, yet!\n");
    pthread_testcancel();
  }

  /* Should never get here */

  printf("thread_func(): not canceled, at all!\n");
  return NULL;
}

int main(void) {
  pthread_t thr;
  void *res;
  int s;

  /* Start a thread and then send it a cancellation request */

  s = pthread_create(&thr, NULL, &thread_func, NULL);
  if (s != 0)
    handle_error_en(s, "pthread_create");

  sleep(5); /* Give thread a chance to get started */

  printf("main(): sending cancellation request\n");
  s = pthread_cancel(thr);
  if (s != 0)
    handle_error_en(s, "pthread_cancel");

  /* Join with thread to see what its exit status was */

  s = pthread_join(thr, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join");

  if (res == PTHREAD_CANCELED)
    printf("main(): thread was canceled\n");
  else
    printf("main(): thread wasn't canceled (shouldn't happen!)\n");
  exit(EXIT_SUCCESS);
}
```

## Funciones utilitarias

Existen dos funciones utilitarias para hilos. La primera, nombrada anteriormente, es *pthread\_self()* que retorna el identificador interno del hilo invocador. La otra función es *pthread\_equal(thread1,thread2)* que compara dos identificadores de hilos, si los dos identificadores son diferentes la función retorna un 0, en caso contrario se retorna un valor no cero.

## Ejercicios


1. Cree una cadena de 5 hilos donde cada hilo padre creará un único hilo hijo y esperará a que termine (`pthread_join`). Cada hilo debe imprimir el índice asignado.

2. Cree un hilo padre que cree 3 hilos hijos, que a su vez crearán otros 3 cada uno (tres niveles: abuelo, padres, nietos). Cada hilo debe imprimir su nivel y el índice asignado, y esperar a sus hijos.

3. Cree un hilo padre que cree 10 hilos hijos a los cuáles les pasará por parámetro (mediante una estructura) dos números aleatorios que ellos deberán sumar e imprimir. Puede usar *r = rand() % n;* para generar un número random entre 0 y n.

4. Cree un hilo padre que cree a su vez 10 hilos hijos a los cuáles les pasará por parámetros un identificador y un número aleatorio de segundos (entre 1 y 5) que cada hilo hijo debe dormir (sleep). Al despertar cada hilo imprimirá su identificador y la cantidad de segundos que esperó.

5. Cree tres hilos A, B y C asíncronos de forma que el hilo A duerma 2 segundos y luego espere que termine el hilo B que dormirá 4 segundos y luego esperará a que termine el hilo C que dormirá 6 segundos. Los hilos deben imprimir cuando terminen de dormir y cuando terminen la espera del otro hilo.

6. Cree un hilo padre que cree 10 hilos hijos los cuáles deben generar un número aleatorio que imprimirán y luego retornarán al hilo padre como su estado de terminación. El padre también debe imprimir los números aleatorios que le retornaron sus hilos hijos al finalizar.

7. Declare un arreglo global de 10 enteros. Cree 10 hilos y haga que el hilo *i* guarde el cuadrado de *i* en la posición *i* del arreglo (cada hilo escribe en una posición distinta). Cuando todos terminen, el hilo principal imprime el arreglo.

8. Cree un arreglo de 1000 enteros y divídalo entre 4 hilos. Cada hilo calcula la suma de su segmento y la retorna al hilo principal (`pthread_exit` / `pthread_join`), que imprime las sumas parciales y la suma total.

9. Cada uno de 10 hilos debe calcular el factorial de un número entre 1 y 10 y retornar un puntero a una estructura (reservada con `malloc`) con el número y su factorial. El hilo principal recupera cada resultado con `pthread_join`, lo imprime y libera la memoria.

10. Cree 4 hilos que buscan un valor en las cuatro partes de un arreglo de 10000 enteros (con un valor repetido solo una vez). El hilo que lo encuentre imprime su posición y cancela a los demás con `pthread_cancel`; el hilo principal espera a todos con `pthread_join`.

11. Cree un proceso padre que cree 5 hilos hijos donde cada uno de ellos entrará en un ciclo infinito esperando ser cancelado, imprimiendo su identificador cada segundo. El padre enviará mensajes de cancelación a cada uno de los hijos con un intervalo de 2 segundos entre ellos.

12. Cree un programa que lea líneas desde el teclado con el formato "mensaje cantidad" (hasta que el usuario escriba "fin"). La impresión de cada mensaje se asigna a un hilo diferente, que imprime el mensaje tantas veces como indique su cantidad, con un intervalo de 2 segundos entre cada impresión.
