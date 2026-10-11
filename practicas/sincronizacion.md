# Sincronización de hilos

Los hilos de un proceso comparten memoria. Cuando dos hilos leen y modifican el mismo dato "al mismo tiempo", el resultado depende del orden exacto en que se intercalan sus instrucciones, y ese orden cambia en cada ejecución. La **sincronización** consiste en coordinar los hilos para que no se estorben al usar datos compartidos (**exclusión mutua**) y para que puedan esperarse unos a otros (**espera de condiciones**).

## Condiciones de carrera

Una instrucción como `contador++` parece una sola operación, pero el procesador la ejecuta en tres pasos: **leer** el valor de la memoria, **sumarle 1** y **escribir** el resultado. Si dos hilos lo hacen a la vez, sus pasos se pueden intercalar así:

``` txt
contador vale 10

Hilo A: lee 10
Hilo B: lee 10
Hilo A: calcula 11 y escribe 11
Hilo B: calcula 11 y escribe 11      <- se perdió un incremento

contador vale 11 (debería valer 12)
```

A esto se le llama **condición de carrera** (*race condition*): el resultado depende del orden en que se ejecuten los hilos. Las secciones de código que acceden a datos compartidos, como `contador++`, se llaman **secciones críticas**, y deben ser ejecutadas por un solo hilo a la vez (**exclusión mutua**).

El siguiente programa provoca el problema: cuatro hilos incrementan un mismo contador 500 000 veces cada uno. Compílelo sin optimizar (sin `-O`).

``` c
// carrera.c: cuatro hilos incrementan el mismo contador
#include <pthread.h>
#include <stdio.h>

#define HILOS 4
#define VUELTAS 500000

long contador = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *incrementa(void *arg) {
  for (int i = 0; i < VUELTAS; i++) {
    // pthread_mutex_lock(&mutex);      // quite los // de estas dos líneas
    contador++;
    // pthread_mutex_unlock(&mutex);    // para proteger la sección crítica
  }
  return NULL;
}

int main() {
  pthread_t hilo[HILOS];

  for (int i = 0; i < HILOS; i++)
    pthread_create(&hilo[i], NULL, incrementa, NULL);
  for (int i = 0; i < HILOS; i++)
    pthread_join(hilo[i], NULL);

  printf("esperado: %d, obtenido: %ld\n", HILOS * VUELTAS, contador);
  return 0;
}
```

``` txt
esperado: 2000000, obtenido: 1514676
```

Ejecútelo varias veces: casi nunca se obtiene 2 000 000, y **el resultado es distinto en cada ejecución**. Ese es el rasgo típico de estos errores: no fallan siempre ni de la misma forma, por lo que son difíciles de encontrar. Ahora quite los `//` de las líneas `pthread_mutex_lock` y `pthread_mutex_unlock` y vuelva a compilar: el resultado siempre es correcto. Lo que se acaba de usar es un mutex.

## Mutex

Un **mutex** (de *mutual exclusion*) es un candado que solo puede tener un hilo a la vez. Un hilo que llama a `pthread_mutex_lock` sobre un mutex ocupado se **bloquea** (se duerme, sin gastar CPU) hasta que el dueño llame a `pthread_mutex_unlock`. Un mutex se representa con una variable de tipo `pthread_mutex_t`.

| Función | Qué hace |
| ------- | -------- |
| `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;` | crea un mutex global, listo para usar |
| `pthread_mutex_init(&m, NULL)` | crea un mutex en tiempo de ejecución (por ejemplo, dentro de un arreglo o una estructura) |
| `pthread_mutex_lock(&m)` | lo toma; si está ocupado, el hilo espera |
| `pthread_mutex_trylock(&m)` | intenta tomarlo; si está ocupado, devuelve `EBUSY` sin esperar |
| `pthread_mutex_unlock(&m)` | lo libera |
| `pthread_mutex_destroy(&m)` | destruye el mutex (solo si nadie lo tiene tomado); no destruye los datos que protegía |

Reglas prácticas:

* Un mutex protege **datos**, no código. Decida qué variables protege cada mutex y tómelo **siempre** que las use, tanto para leer como para escribir.
* Mantenga la sección crítica lo más corta posible: mientras un hilo tiene el mutex, los demás esperan.
* Todo `lock` necesita su `unlock` en **todos** los caminos de salida de la función (`return`, `break`, `continue`).
* Solo el hilo que tomó el mutex debe liberarlo.

### `pthread_mutex_trylock`

`pthread_mutex_lock` es lo que se usa casi siempre. `pthread_mutex_trylock` sirve cuando el hilo tiene **otra cosa útil que hacer** si el mutex está ocupado. En el ejemplo, `main` toma el mutex antes de crear el hilo, así que el primer intento del hilo falla; después `main` lo libera y el segundo intento tiene éxito.

``` c
// trylock.c: intentar tomar un mutex sin quedarse esperando
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *hilo(void *arg) {
  if (pthread_mutex_trylock(&mutex) == EBUSY) {
    printf("hilo: el mutex está ocupado, hago otra cosa\n");
  } else {
    printf("hilo: obtuve el mutex\n");
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main() {
  pthread_t t;

  pthread_mutex_lock(&mutex);          // main tiene el mutex...
  pthread_create(&t, NULL, hilo, NULL);
  pthread_join(t, NULL);               // ...y el hilo lo encuentra ocupado
  pthread_mutex_unlock(&mutex);

  pthread_create(&t, NULL, hilo, NULL);
  pthread_join(t, NULL);               // ahora el mutex está libre
  return 0;
}
```

``` txt
hilo: el mutex está ocupado, hago otra cosa
hilo: obtuve el mutex
```

No use `trylock` en un ciclo vacío hasta que funcione: eso es **espera activa** (*busy waiting*), que gasta CPU sin hacer nada útil y es peor que dejar que `lock` duerma al hilo.

## Interbloqueo

Los mutex resuelven las condiciones de carrera, pero introducen otro problema: dos hilos pueden esperarse **para siempre**. Es un **interbloqueo** (*deadlock*), y ocurre cuando cada hilo tiene un mutex y necesita el que tiene el otro:

``` txt
Hilo 1: tiene A, pide B  ----+  cada uno espera
Hilo 2: tiene B, pide A  ----+  lo que tiene el otro
```

En el programa, el `sleep(1)` da tiempo a que cada hilo tome su primer mutex antes de que alguno pida el segundo, así el interbloqueo ocurre siempre. En un programa real depende de la suerte, y por eso puede pasar inadvertido durante meses.

``` c
// interbloqueo.c: dos hilos que toman dos mutex en orden inverso
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;

void *hilo1(void *arg) {
  pthread_mutex_lock(&a);
  printf("hilo 1: tiene A, pide B\n");
  sleep(1);                     // da tiempo a que el hilo 2 tome B
  pthread_mutex_lock(&b);
  printf("hilo 1: tiene A y B\n");
  pthread_mutex_unlock(&b);
  pthread_mutex_unlock(&a);
  return NULL;
}

void *hilo2(void *arg) {
  pthread_mutex_lock(&b);
  printf("hilo 2: tiene B, pide A\n");
  sleep(1);
  pthread_mutex_lock(&a);
  printf("hilo 2: tiene A y B\n");
  pthread_mutex_unlock(&a);
  pthread_mutex_unlock(&b);
  return NULL;
}

int main() {
  pthread_t t1, t2;

  pthread_create(&t1, NULL, hilo1, NULL);
  pthread_create(&t2, NULL, hilo2, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  printf("terminaron\n");       // nunca se llega aquí
  return 0;
}
```

``` txt
hilo 1: tiene A, pide B
hilo 2: tiene B, pide A
```

El programa se queda "congelado" sin ningún mensaje de error; presione `Ctrl+C` para terminarlo.

Un interbloqueo necesita que se cumplan **cuatro condiciones** a la vez (condiciones de Coffman): exclusión mutua, retener un recurso mientras se espera otro, no poder quitarle el recurso a quien lo tiene, y **espera circular**. Basta con eliminar una para evitarlo. La técnica más práctica es eliminar la espera circular con un **orden global de locks**: todos los hilos toman los mutex siempre en el mismo orden. Aquí, si `hilo2` también toma primero A y luego B, el problema desaparece:

``` c
void *hilo2(void *arg) {
  pthread_mutex_lock(&a);               // mismo orden que el hilo 1: A y luego B
  printf("hilo 2: tiene A, pide B\n");
  sleep(1);
  pthread_mutex_lock(&b);
  printf("hilo 2: tiene A y B\n");
  pthread_mutex_unlock(&b);
  pthread_mutex_unlock(&a);
  return NULL;
}
```

``` txt
hilo 1: tiene A, pide B
hilo 1: tiene A y B
hilo 2: tiene A, pide B
hilo 2: tiene A y B
terminaron
```

## Variables de condición

Un mutex evita que los hilos se estorben, pero no permite que un hilo **espere a que ocurra algo**: que otro hilo prepare un dato, que un contador llegue a cierto valor, que un recurso quede disponible. Revisar una bandera en un ciclo (*espera activa*) desperdicia CPU y obliga a tomar el mutex constantemente solo para mirarla.

Las **variables de condición** (`pthread_cond_t`) resuelven esto. Se usan siempre **junto con un mutex**: el mutex protege el dato compartido que forma parte de la condición, y la variable de condición permite que un hilo se duerma hasta que otro le avise que la condición pudo haber cambiado.

| Función | Qué hace |
| ------- | -------- |
| `pthread_cond_t c = PTHREAD_COND_INITIALIZER;` | crea una variable de condición global (o `pthread_cond_init` / `pthread_cond_destroy`) |
| `pthread_cond_wait(&c, &m)` | **libera el mutex y duerme** al hilo, en un solo paso atómico; al despertar, vuelve a tomar el mutex antes de retornar |
| `pthread_cond_signal(&c)` | despierta a **un** hilo que esté esperando (si hay alguno) |
| `pthread_cond_broadcast(&c)` | despierta a **todos** los hilos que esperan |
| `pthread_cond_timedwait(&c, &m, &limite)` | como `wait`, pero se rinde al llegar la hora `limite` y devuelve `ETIMEDOUT` |

El esquema de uso es siempre el mismo:

``` c
// el hilo que espera             // el hilo que avisa
pthread_mutex_lock(&m);           pthread_mutex_lock(&m);
while (!condicion)                /* cambiar el estado */
  pthread_cond_wait(&c, &m);      pthread_cond_signal(&c);
/* usar el estado */              pthread_mutex_unlock(&m);
pthread_mutex_unlock(&m);
```

Tres reglas que hay que grabarse:

1. **`wait` siempre dentro de un `while`, nunca de un `if`.** Un hilo puede despertar sin que la condición se cumpla (despertares espurios), o puede que otro hilo se le adelante y consuma el recurso antes de que él recupere el mutex. Con `while`, el hilo revisa de nuevo la condición y, si no se cumple, vuelve a dormir.
2. **La condición es un estado, no un evento.** Una variable de condición no recuerda nada: si se hace `signal` cuando nadie espera, el aviso se pierde. Lo que importa es la variable compartida (`listo`, `cuenta`...) que se revisa en el `while`.
3. **Primero cambie el estado y después avise**, todo con el mutex tomado.

En el ejemplo, un hilo espera a que otro prepare un dato:

``` c
// condicion.c: un hilo espera a que otro prepare un dato
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int listo = 0;                  // la condición que se espera
int dato = 0;                   // el dato que protege la condición
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *espera(void *arg) {
  pthread_mutex_lock(&mutex);
  while (!listo)                // siempre en un while, nunca en un if
    pthread_cond_wait(&cond, &mutex);
  printf("el dato listo es %d\n", dato);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

void *avisa(void *arg) {
  sleep(1);                     // simula preparar el dato
  pthread_mutex_lock(&mutex);
  dato = 42;
  listo = 1;                    // primero se cambia el estado...
  pthread_cond_signal(&cond);   // ...y después se avisa
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main() {
  pthread_t t1, t2;

  pthread_create(&t1, NULL, espera, NULL);
  pthread_create(&t2, NULL, avisa, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
```

``` txt
el dato listo es 42
```

Si `avisa` se ejecutara antes que `espera`, `listo` ya valdría 1 cuando `espera` llegue, el `while` no se ejecutaría y no haría falta dormir. Por eso lo que se revisa es el estado (`listo`) y no el aviso.

### `signal` y `broadcast`

`pthread_cond_signal` despierta a **un solo** hilo; úselo cuando el cambio de estado le sirve a uno (por ejemplo, se liberó una sola unidad de un recurso). `pthread_cond_broadcast` despierta a **todos** los que esperan; úselo cuando el cambio puede servirle a varios, o cuando en la misma variable de condición esperan hilos con condiciones distintas. En caso de duda, `broadcast` es más seguro: los hilos que no puedan continuar volverán a dormir gracias al `while`, y lo único que se pierde es algo de rendimiento.

En el ejemplo, cuatro corredores esperan la misma señal de salida:

``` c
// largada.c: varios hilos esperan una señal de salida (pthread_cond_broadcast)
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define CORREDORES 4

int salida = 0;                 // la condición: "se dio la salida"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *corredor(void *arg) {
  int id = *(int *)arg;

  pthread_mutex_lock(&mutex);
  while (!salida)
    pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);

  printf("corredor %d sale\n", id);
  return NULL;
}

int main() {
  pthread_t hilo[CORREDORES];
  int id[CORREDORES];

  for (int i = 0; i < CORREDORES; i++) {
    id[i] = i;
    pthread_create(&hilo[i], NULL, corredor, &id[i]);
  }

  sleep(1);                     // los corredores se preparan
  pthread_mutex_lock(&mutex);
  salida = 1;
  pthread_cond_broadcast(&cond);   // despierta a TODOS (signal solo a uno)
  pthread_mutex_unlock(&mutex);

  for (int i = 0; i < CORREDORES; i++)
    pthread_join(hilo[i], NULL);
  return 0;
}
```

``` txt
corredor 1 sale
corredor 0 sale
corredor 3 sale
corredor 2 sale
```

(el orden puede variar). Con `pthread_cond_signal` en lugar de `broadcast`, saldría un solo corredor y el programa quedaría colgado.

### `pthread_cond_timedwait`

A veces un hilo espera una condición, pero no puede esperar para siempre. `pthread_cond_timedwait` recibe además una **hora límite absoluta** (no una duración) y retorna `ETIMEDOUT` si llega esa hora sin que se haya avisado. En el ejemplo el plazo es de 2 segundos (`time(NULL) + 2`), pero el aviso tarda 4:

``` c
// timedwait.c: esperar una condición, pero solo hasta un plazo
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int listo = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *avisa(void *arg) {
  sleep(4);                     // tarda más que el plazo; pruebe con sleep(1)
  pthread_mutex_lock(&mutex);
  listo = 1;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main() {
  pthread_t t;
  struct timespec limite;
  int rc = 0;

  pthread_create(&t, NULL, avisa, NULL);

  limite.tv_sec = time(NULL) + 2;   // plazo: una hora ABSOLUTA (ahora + 2 s)
  limite.tv_nsec = 0;

  pthread_mutex_lock(&mutex);
  while (!listo && rc != ETIMEDOUT)
    rc = pthread_cond_timedwait(&cond, &mutex, &limite);
  if (listo)
    printf("la condición se cumplió\n");
  else
    printf("se acabó el plazo, sigo sin esperar más\n");
  pthread_mutex_unlock(&mutex);
  return 0;
}
```

``` txt
se acabó el plazo, sigo sin esperar más
```

Cambie `sleep(4)` por `sleep(1)` y el programa imprimirá `la condición se cumplió`. Observe que el ciclo termina cuando se cumple la condición **o** vence el plazo, y que el plazo se calcula una sola vez, antes del ciclo.

## Semáforos

Un **semáforo** es un contador entero que nunca es negativo, con dos operaciones:

* `wait`: si el contador es mayor que 0, lo decrementa y continúa; si es 0, el hilo se **bloquea** hasta que otro hilo haga `post`.
* `post`: incrementa el contador y, si hay hilos esperando, despierta a uno.

Los semáforos POSIX se definen en `<semaphore.h>` mediante variables de tipo `sem_t`:

| Función | Qué hace |
| ------- | -------- |
| `sem_init(&s, 0, valor)` | crea el semáforo con `valor` como valor inicial del contador (el `0` indica que se comparte entre los hilos de un mismo proceso) |
| `sem_wait(&s)` | decrementa el contador, esperando si vale 0 |
| `sem_trywait(&s)` | igual, pero retorna con error de inmediato en vez de esperar |
| `sem_post(&s)` | incrementa el contador |
| `sem_destroy(&s)` | destruye el semáforo |

Un semáforo se usa de dos formas principales:

* **Limitar el acceso a un recurso.** Si el contador empieza en *n*, hasta *n* hilos pueden estar a la vez en una sección de código: cada hilo hace `wait` al entrar y `post` al salir.
* **Avisar que ocurrió algo.** Si el contador empieza en 0, un hilo hace `wait` y se duerme hasta que otro hilo haga `post`.

El siguiente ejemplo limita a 3 la cantidad de hilos en una sección, aunque haya 8 hilos:

``` c
// semaforo_limite.c: un semáforo limita a N los hilos en una sección
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#define N 3                     // máximo de hilos a la vez en la sección
#define HILOS 8

sem_t sem;

void *func(void *arg) {
  int id = *(int *)arg;

  sem_wait(&sem);               // decrementa el contador (espera si es 0)
  printf("hilo %d entra\n", id);
  sleep(1);                     // trabajo dentro de la sección limitada
  printf("hilo %d sale\n", id);
  sem_post(&sem);               // incrementa el contador
  return NULL;
}

int main() {
  pthread_t hilo[HILOS];
  int id[HILOS];

  sem_init(&sem, 0, N);         // el contador empieza en N
  for (int i = 0; i < HILOS; i++) {
    id[i] = i;
    pthread_create(&hilo[i], NULL, func, &id[i]);
  }
  for (int i = 0; i < HILOS; i++)
    pthread_join(hilo[i], NULL);
  sem_destroy(&sem);
  return 0;
}
```

``` txt
hilo 2 entra
hilo 0 entra
hilo 1 entra
hilo 2 sale
hilo 3 entra
hilo 1 sale
hilo 0 sale
hilo 4 entra
...
```

Nunca hay más de 3 hilos entre un `entra` y su `sale`; cada vez que uno sale, entra el siguiente. El segundo ejemplo usa un semáforo que empieza en 0 como aviso: `main` espera a que el trabajador termine.

``` c
// semaforo_evento.c: un semáforo en 0 sirve para avisar que algo ocurrió
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

sem_t listo;                    // empieza en 0: todavía no ha ocurrido nada

void *trabajador(void *arg) {
  sleep(1);                     // simula un trabajo
  printf("trabajador: terminé\n");
  sem_post(&listo);             // avisa (incrementa el contador a 1)
  return NULL;
}

int main() {
  pthread_t t;

  sem_init(&listo, 0, 0);
  pthread_create(&t, NULL, trabajador, NULL);

  printf("main: espero al trabajador\n");
  sem_wait(&listo);             // se duerme hasta que el contador sea > 0
  printf("main: el trabajador terminó\n");

  pthread_join(t, NULL);
  sem_destroy(&listo);
  return 0;
}
```

``` txt
main: espero al trabajador
trabajador: terminé
main: el trabajador terminó
```

Diferencias con las otras herramientas:

* Un semáforo **no tiene dueño**: cualquier hilo puede hacer `post`, no solo el que hizo `wait` (un mutex solo lo libera quien lo tomó). Un semáforo que empieza en 1 se comporta como un mutex.
* Un semáforo **recuerda** los `post`, aunque nadie estuviera esperando (una variable de condición no recuerda nada).
* Las variables de condición permiten esperar **cualquier condición** sobre el estado (`cuenta == 0`, `a > b`), pero el semáforo solo expresa "el contador es mayor que cero". Los semáforos son cómodos para contar recursos iguales; las variables de condición, para condiciones más elaboradas.

> Los semáforos sin nombre (`sem_init`) no están implementados en macOS. En ese sistema hay que usar semáforos con nombre (`sem_open`, `sem_close`, `sem_unlink`) o construir el semáforo con un mutex y una variable de condición.

## Inanición

Además del interbloqueo, hay otro problema de sincronización: la **inanición** (*starvation*). Un hilo sufre inanición cuando **no está bloqueado del todo, pero nunca logra avanzar** porque otros hilos siempre pasan antes. Por ejemplo, si al despertar hilos en espera se elige siempre a los de "mayor prioridad" y estos llegan sin parar, los de menor prioridad esperan indefinidamente. Se evita con políticas justas, como atender a los hilos en orden de llegada.

## Referencias

* Butenhof, D. R. *Programming with POSIX Threads*, Addison-Wesley, 1997.

## Ejercicios


En todos los ejercicios genere los hilos "cliente", "auto", etc. en forma continua (puede basarse en la plantilla `10_plantilla.c`), e imprima mensajes que permitan verificar que la solución es correcta.

1. Una cuenta bancaria tiene un saldo inicial de 100. Varios hilos realizan depósitos y otros realizan retiros de montos aleatorios. Un retiro que excede el saldo debe esperar hasta que otro hilo deposite lo suficiente. Escriba los procedimientos deposito() y retiro() con un mutex y una variable de condición, y compruebe que el saldo nunca es negativo.
2. En un edificio de estacionamiento hay n lugares disponibles. Los autos llegan y, si hay lugar, estacionan; si no, esperan. Cuando un auto se va, libera un lugar y permite que entre uno de los que esperan. Escriba el procedimiento auto() utilizando un semáforo.
3. Tres hilos forman una tubería (*pipeline*) conectada por buffers de una sola posición: el hilo A genera números, el hilo B calcula su cuadrado y el hilo C los imprime. Cada hilo debe esperar si el buffer que necesita está vacío o lleno. Use mutex y variables de condición (o semáforos).
4. En una barbería hay un solo barbero, una silla para el corte de pelo y una sala de espera con n sillas. Si no hay clientes, el barbero se sienta en su silla y se duerme. Cuando llega un cliente, si el barbero está durmiendo, lo despierta; si el barbero está ocupado, el cliente se sienta en una silla de la sala de espera (si hay alguna disponible). Si no hay sillas disponibles, el cliente se va. Escriba los procedimientos barbero() y cliente().
5. En un banco hay una sola fila y 3 cajeros. Cada cliente que llega espera su turno en la fila y es atendido por el primer cajero que quede libre; si no hay clientes, los cajeros esperan. Escriba los procedimientos cajero() y cliente(), imprimiendo qué cajero atiende a cada cliente.
6. En un camino existe un puente que cuenta con un sólo carril (y no tiene CEDA). Existen autos que quieren pasar de sur a norte y otros de norte a sur. El puente sólo puede ser atravesado por uno o más autos que vayan en un mismo sentido, es decir, el puente no puede ser atravesado simultáneamente por autos circulando en sentidos opuestos. Escriba los procedimientos AutoSurNorte() y AutoNorteSur(), que imprimen el número de auto que atraviesa el puente y el sentido.
7. En una atracción del Parque Jurásico existen n vehículos que llevan a k pasajeros a través del parque. Si no hay autos disponibles los pasajeros deben esperar en una única fila a que regrese uno de los carros. El auto se puede ir con menos pasajeros si no hay los suficientes en la fila, pero si no hay ningún pasajero el carro debe esperar. Escriba los procedimientos pasajero() y carro().
8. Implemente una **barrera reutilizable** usando un mutex y una variable de condición (sin `pthread_barrier_t`): *n* hilos ejecutan 3 fases de trabajo y ninguno puede comenzar la fase *k+1* hasta que todos hayan terminado la fase *k*. Cada hilo imprime cuándo llega y cuándo sale de cada fase.
9. Cuatro hilos "corredores" participan en una carrera de relevos: el corredor 1 corre, entrega el testigo al corredor 2, y así hasta el corredor 4, siempre en ese orden. Cada corredor imprime cuando recibe el testigo y cuando lo entrega. Use variables de condición o semáforos.
10. Existen varios hilos lectores y varios hilos escritores que comparten una base de datos. Los lectores pueden acceder simultáneamente, pero los escritores requieren acceso exclusivo. Modifique el ejemplo `08_lectores_escritores.c` para dar **prioridad a los escritores**: si hay un escritor esperando, no deben entrar nuevos lectores.
11. Varios clientes llegan a una ventanilla y esperan con un tiempo límite de 3 segundos a que un cajero los atienda. Si el tiempo se agota, el cliente se va molesto. Use `pthread_cond_timedwait` e imprima cuántos clientes fueron atendidos y cuántos se fueron.
12. Hay 4 cuentas bancarias, cada una con su propio mutex, y 6 hilos que realizan transferencias de montos aleatorios entre dos cuentas elegidas al azar. Implemente `transferir(origen, destino, monto)` sin riesgo de interbloqueo (por ejemplo, adquiriendo los mutex siempre en el mismo orden) y verifique que la suma total de los saldos no cambia.
