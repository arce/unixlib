# Asignación de memoria

Cada vez que un programa en C llama a `malloc`, alguien tiene que decidir *qué* trozo de memoria entregar y recordar cuáles trozos ya están ocupados. En este tutorial se construyen, paso a paso, dos asignadores de memoria pequeños pero completos:

1. un **asignador de pila** (`arena_alloc` / `arena_free`), el más simple posible;
2. un **asignador con lista libre** (`mm_malloc` / `mm_free`), que funciona en cualquier orden, fusiona bloques vecinos y pide memoria al sistema operativo con `mmap`.

## Qué hace un asignador de memoria

Un proceso tiene distintas regiones de memoria. Las variables locales viven en la pila y las globales en la zona de datos, pero la memoria que se pide en tiempo de ejecución con `malloc` sale del *heap*:

``` txt
dirección alta
+-------------------------+
|          pila           |  variables locales, crece hacia abajo
+-------------------------+
|            |            |
|            v            |
|                         |
|            ^            |
|            |            |
+-------------------------+
|   regiones mmap         |  bibliotecas, bloques grandes
+-------------------------+
|          heap           |  lo que administra malloc
+-------------------------+
|     datos globales      |
+-------------------------+
|        código           |
+-------------------------+
dirección baja
```

El sistema operativo entrega memoria en trozos grandes (páginas, normalmente de 4 KiB o 16 KiB). El asignador se encarga de:

* **repartir** esos trozos en bloques del tamaño que pide el programa;
* **recordar** qué bloques están libres para poder reutilizarlos;
* **evitar la fragmentación**, es decir, que la memoria libre quede repartida en huecos tan pequeños que no sirvan para nada.

## Asignador de pila

El asignador más simple posible reserva memoria de un arreglo grande y fijo. Tiene dos operaciones: `arena_alloc(n)` devuelve un puntero a `n` bytes consecutivos, y `arena_free(p)` los devuelve. Las llamadas a `arena_free` deben hacerse en el orden opuesto a las de `arena_alloc`: el almacenamiento se comporta como una **pila** (último en entrar, primero en salir). Liberar `p` libera también todo lo que se reservó después de `p`.

``` txt
   pool                                              pool + POOL_SIZE
   +--------+-----------+-------------+--------------------------+
   |   a    |     b     |      c      |          libre           |
   +--------+-----------+-------------+--------------------------+
   ^                                  ^
   inicio                             top (siguiente byte libre)

   arena_free(b)  ->  top se mueve hasta el inicio de b (libera b y c)
```

La interfaz:

``` c
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

// Reserva n bytes de la pila. Devuelve NULL si no hay espacio.
void *arena_alloc(size_t n);

// Libera p y TODO lo que se reservó después de p.
void arena_free(void *p);

// Cantidad de bytes actualmente en uso (útil para depurar).
size_t arena_used(void);

#endif
```

La implementación completa:

``` c
// arena.c: asignador de memoria de tipo pila
#include "arena.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define POOL_SIZE 10000                // tamaño del espacio disponible
#define ALIGNMENT alignof(max_align_t) // todo puntero devuelto queda alineado

static alignas(max_align_t) unsigned char pool[POOL_SIZE]; // el espacio en sí
static unsigned char *top = pool; // primer byte libre

void *arena_alloc(size_t n) {
  if (n == 0 || n > POOL_SIZE)
    return NULL;

  n = (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1); // redondea hacia arriba

  size_t available = (size_t)(pool + POOL_SIZE - top);
  if (n > available)
    return NULL;

  void *p = top;
  top += n;
  return p;
}

void arena_free(void *p) {
  uintptr_t addr = (uintptr_t)p;
  if (addr >= (uintptr_t)pool && addr < (uintptr_t)top)
    top = p; // "rebobina" la pila hasta p
}

size_t arena_used(void) { return (size_t)(top - pool); }
```

Algunos detalles que vale la pena notar:

* **Alineamiento.** Los procesadores exigen (o prefieren) que un `int` o un `double` estén en direcciones múltiplos de su tamaño. Por eso cada solicitud se redondea hacia arriba a un múltiplo de `alignof(max_align_t)` (16 bytes en sistemas de 64 bits), de modo que el puntero devuelto sirve para cualquier tipo.
* **Tipos.** El tamaño es `size_t`, el tipo que usa la biblioteca estándar, y el retorno es `void *`, así que el llamador no necesita casts.
* **Liberación segura.** `arena_free` ignora punteros que no estén dentro de la parte usada de la pila.

### Ejemplo

``` c
// demo_arena.c
#include <stdio.h>

#include "arena.h"

int main(void) {
  int *a = arena_alloc(10 * sizeof(int));
  if (a == NULL) {
    fprintf(stderr, "sin memoria\n");
    return 1;
  }
  for (int i = 0; i < 10; i++)
    a[i] = 10 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", a[i]);
  printf("\nusado: %zu bytes\n", arena_used());

  arena_free(a); // la pila queda vacía otra vez
  printf("usado: %zu bytes\n", arena_used());

  a = arena_alloc(10 * sizeof(int)); // reutiliza el mismo espacio
  for (int i = 0; i < 10; i++)
    a[i] = 20 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", a[i]);
  printf("\n");

  arena_free(a);
  return 0;
}
```

``` bash
gcc arena.c demo_arena.c -o demo_arena && ./demo_arena
```

``` txt
0 10 20 30 40 50 60 70 80 90 
usado: 48 bytes
usado: 0 bytes
0 20 40 60 80 100 120 140 160 180 
```

Se reservaron 40 bytes (10 enteros), pero se usan 48: el redondeo a múltiplos de 16 añade 8 bytes de relleno.

## Asignador con lista libre

El asignador de pila es rápido y sencillo, pero obliga a liberar en orden inverso. El asignador de esta sección, `mm_malloc` / `mm_free`, no tiene esa restricción: las llamadas pueden ocurrir en cualquier orden. Además, en vez de usar un arreglo de tamaño fijo, **pide memoria al sistema operativo** cuando la necesita.

### La idea

Toda la memoria del asignador se divide en **bloques**. Cada bloque, libre o usado, empieza con un pequeño **encabezado** que dice cuánto mide. Los bloques libres se enlazan en una **lista libre**, ordenada por dirección de memoria:

``` txt
dirección baja                                                   dirección alta
+-------+--------+-------+--------+-------+---------------------------+
| USADO | LIBRE  | USADO | LIBRE  | USADO |           LIBRE           |
+-------+--------+-------+--------+-------+---------------------------+
           ^                 ^                          ^
           |                 |                          |
   free_list -------------> ...  ------------------->  ... -> NULL
```

Cada bloque tiene esta forma:

``` txt
              +---------------------+
   encabezado | size | next         |   16 bytes
              +---------------------+
   zona útil  |                     |   <- el usuario recibe este puntero
              |   (n bytes)         |
              +---------------------+
```

Con esta estructura, las tres operaciones funcionan así:

* **`mm_malloc(n)`** recorre la lista libre buscando un bloque suficientemente grande (esta es la *estrategia de ajuste*). Si el bloque sobra, se **divide** en dos: una parte se entrega y el resto sigue libre. Si no hay ningún bloque que sirva, se pide una región nueva al sistema operativo.
* **`mm_free(p)`** retrocede desde `p` hasta el encabezado del bloque y lo inserta en la lista libre, en el lugar que le corresponde por dirección.
* Al insertar, si el bloque liberado queda **pegado** a un bloque libre (antes, después o ambos), se **fusionan** en un solo bloque más grande. Esto evita que la memoria se fragmente en huecos diminutos. Como la lista está ordenada por dirección, saber quiénes son los vecinos es trivial.

### Constantes y encabezado

``` c
// mm.h: asignador de memoria con lista libre (interfaz pública)
#ifndef MM_H
#define MM_H

#include <stddef.h>

typedef enum {
  MM_FIRST_FIT, // primer ajuste: el primer bloque libre que sirva
  MM_BEST_FIT   // mejor ajuste: el bloque libre más pequeño que sirva
} mm_strategy_t;

void *mm_malloc(size_t n);         // reserva n bytes (NULL si falla)
void mm_free(void *ptr);           // libera un bloque obtenido con mm_malloc
void mm_set_strategy(mm_strategy_t s);

// Depuración y pruebas
void mm_dump(void);                // imprime el mapa de la memoria
size_t mm_free_blocks(void);       // cantidad de bloques en la lista libre
size_t mm_free_bytes(void);        // bytes libres (encabezados incluidos)
size_t mm_heap_bytes(void);        // bytes totales obtenidos del sistema

#endif
```

El encabezado mide exactamente 16 bytes, así que si todos los tamaños de bloque son múltiplos de 16, la zona útil que sigue al encabezado queda alineada automáticamente.

Mientras un bloque está en uso, su campo `next` no sirve para nada, así que se aprovecha para guardar un valor "mágico". `mm_free` lo verifica y así detecta un `free` doble o de un puntero que nunca salió de `mm_malloc`.

### Pedir memoria al sistema operativo

Cuando no hay ningún bloque libre suficientemente grande, `grow_heap` pide una región nueva con `mmap` y la agrega a la lista libre:

``` c
// mm.c (fragmento): grow_heap
// Tamaño de página del sistema (normalmente 4 KiB).
static size_t page_size(void) {
#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (size_t)info.dwPageSize;
#else
  return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

// Pide `size` bytes al sistema operativo (memoria en ceros). NULL si falla.
static void *os_alloc(size_t size) {
#ifdef _WIN32
  return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return mem == MAP_FAILED ? NULL : mem;
#endif
}

// Pide al sistema operativo una región nueva de al menos min_size bytes y la
// agrega a la lista libre. Devuelve 0 si tuvo éxito y -1 si falló.
static int grow_heap(size_t min_size) {
  if (nchunks == MAX_CHUNKS)
    return -1;

  size_t size = align_up(min_size + HEADER_SIZE, page_size());
  if (size < CHUNK_SIZE)
    size = CHUNK_SIZE;

  void *mem = os_alloc(size);
  if (mem == NULL)
    return -1;

  // Se deja libre un encabezado al final de la región como separador, para
  // que dos regiones contiguas nunca se fusionen en un solo bloque.
  block_t *b = mem;
  b->size = size - HEADER_SIZE;
  chunks[nchunks].base = mem;
  chunks[nchunks].size = size;
  nchunks++;
  heap_bytes += b->size;

  insert_free(b);
  return 0;
}
```

`MAP_ANONYMOUS` significa que la memoria no está respaldada por ningún archivo: el sistema operativo entrega páginas llenas de ceros.

Las dos funciones auxiliares `page_size` y `os_alloc` aíslan lo que depende del sistema operativo: en Linux y macOS se usa `sysconf` y `mmap`; en Windows, `GetSystemInfo` y `VirtualAlloc`, que hacen lo mismo. El resto del asignador no cambia, y es un buen ejemplo de cómo se aísla una dependencia de plataforma en un par de funciones pequeñas.

Una región se pide de al menos `CHUNK_SIZE` (64 KiB) para no llamar al sistema operativo por cada solicitud pequeña; si la solicitud es mayor, se redondea a un múltiplo del tamaño de página.

### Liberar y fusionar

`insert_free` es el corazón del asignador. Busca el lugar del bloque en la lista (ordenada por dirección) y revisa si puede fusionarse con su vecino de adelante y con el de atrás:

``` c
// mm.c (fragmento): insert_free
// Inserta b en la lista libre (ordenada por dirección) y lo fusiona con sus
// vecinos si son contiguos en memoria.
static void insert_free(block_t *b) {
  block_t *prev = NULL;
  block_t *next = free_list;
  while (next != NULL && (uintptr_t)next < (uintptr_t)b) {
    prev = next;
    next = next->next;
  }

  // b queda entre prev y next
  b->next = next;
  if (prev != NULL)
    prev->next = b;
  else
    free_list = b;

  // ¿b termina justo donde empieza next? Se fusionan.
  if (next != NULL && (char *)b + b->size == (char *)next) {
    b->size += next->size;
    b->next = next->next;
  }
  // ¿prev termina justo donde empieza b? Se fusionan.
  if (prev != NULL && (char *)prev + prev->size == (char *)b) {
    prev->size += b->size;
    prev->next = b->next;
  }
}
```

Los tres casos posibles al liberar un bloque `b` entre `prev` y `next`:

``` txt
Sin vecinos libres:     [prev] [USADO] [ b ] [USADO] [next]
                        -> b queda como un bloque libre más

Vecino de adelante:     [prev] [USADO] [ b ][next]
                        -> [prev] [USADO] [   b + next   ]

Vecinos en ambos lados: [prev][ b ][next]
                        -> [       prev + b + next       ]
```

### Buscar un bloque y dividirlo

La búsqueda está aislada en `find_fit`, que aplica la estrategia de ajuste elegida. Aquí aparecen dos: **primer ajuste** (`MM_FIRST_FIT`) devuelve el primer bloque que sirve; **mejor ajuste** (`MM_BEST_FIT`) recorre toda la lista y devuelve el más pequeño que sirve.

``` c
// mm.c (fragmento): find_fit
// Busca en la lista libre un bloque de al menos `need` bytes según la
// estrategia activa. Devuelve el bloque (o NULL) y, en *prev_out, su anterior.
static block_t *find_fit(size_t need, block_t **prev_out) {
  block_t *prev = NULL;
  block_t *best = NULL;
  block_t *best_prev = NULL;

  for (block_t *cur = free_list; cur != NULL; prev = cur, cur = cur->next) {
    if (cur->size < need)
      continue;

    if (strategy == MM_FIRST_FIT) {
      *prev_out = prev;
      return cur; // el primero que sirve
    }
    // MM_BEST_FIT: seguimos buscando uno más pequeño
    if (best == NULL || cur->size < best->size) {
      best = cur;
      best_prev = prev;
    }
  }

  *prev_out = best_prev;
  return best;
}
```

Con `find_fit` resuelto, `mm_malloc` queda corto:

``` c
// mm.c (fragmento): mm_malloc
void *mm_malloc(size_t n) {
  if (n == 0 || n > SIZE_MAX / 2)
    return NULL;

  size_t need = align_up(n, ALIGNMENT) + HEADER_SIZE;

  block_t *prev;
  block_t *b = find_fit(need, &prev);
  if (b == NULL) { // no hay bloque suficiente: pedir más al sistema
    if (grow_heap(need) < 0)
      return NULL;
    b = find_fit(need, &prev);
    if (b == NULL)
      return NULL;
  }

  // Si sobra espacio para otro bloque útil, se divide en dos.
  if (b->size >= need + 2 * HEADER_SIZE) {
    block_t *rest = (block_t *)((char *)b + need);
    rest->size = b->size - need;
    rest->next = b->next;
    b->size = need;
    b->next = rest;
  }

  // Se desliga b de la lista libre.
  if (prev != NULL)
    prev->next = b->next;
  else
    free_list = b->next;

  b->next = ALLOC_MAGIC;
  return b + 1; // el usuario recibe la zona justo después del encabezado
}
```

Puntos clave:

* `need` es el tamaño total que se busca: la solicitud redondeada a múltiplos de 16, más el encabezado.
* Un bloque solo se divide si lo que sobra alcanza para un encabezado más al menos 16 bytes útiles; si sobra menos, se entrega completo (un poco de desperdicio interno es mejor que un bloque inútil en la lista).
* `b + 1` es aritmética de punteros: avanza `sizeof(block_t)` bytes, es decir, salta el encabezado.
* Se comprueba `n > SIZE_MAX / 2` para que las sumas y redondeos internos nunca desborden.

### Liberar

``` c
// mm.c (fragmento): mm_free
void mm_free(void *ptr) {
  if (ptr == NULL)
    return;

  block_t *b = (block_t *)ptr - 1; // retrocede hasta el encabezado
  if (b->next != ALLOC_MAGIC) {
    fprintf(stderr, "mm_free: puntero inválido o doble free (%p)\n", ptr);
    abort();
  }
  insert_free(b);
}
```

### Ver la memoria con `mm_dump`

Un asignador es difícil de entender "a ciegas". Por eso `mm.c` incluye `mm_dump`, que imprime el mapa de cada región: qué bloques están libres y cuáles usados (los huecos entre bloques libres son bloques usados). Los tamaños incluyen el encabezado de 16 bytes.

``` c
// mm.c (fragmento): mm_dump
// Imprime, para cada región, los bloques libres y usados en orden de
// dirección. Los tamaños incluyen el encabezado de 16 bytes.
void mm_dump(void) {
  for (size_t i = 0; i < nchunks; i++) {
    char *cur = chunks[i].base;
    char *end = cur + chunks[i].size;
    printf("región %zu (%zu bytes):\n  ", i, chunks[i].size);

    for (block_t *f = free_list; f != NULL; f = f->next) {
      if ((char *)f < cur || (char *)f >= end)
        continue; // pertenece a otra región
      if ((char *)f > cur)
        printf("[USADO %zu] ", (size_t)((char *)f - cur));
      printf("[LIBRE %zu] ", f->size);
      cur = (char *)f + f->size;
    }
    if (cur < end)
      printf("[USADO %zu]", (size_t)(end - cur));
    printf("\n");
  }
}
```

### Ejemplo

``` c
// demo_mm.c
#include <stdio.h>

#include "mm.h"

int main(void) {
  int *arreglo = mm_malloc(10 * sizeof(int));
  char *texto = mm_malloc(200);
  double *valores = mm_malloc(5 * sizeof(double));
  if (arreglo == NULL || texto == NULL || valores == NULL)
    return 1;

  for (int i = 0; i < 10; i++)
    arreglo[i] = 10 * i;
  for (int i = 0; i < 10; i++)
    printf("%d ", arreglo[i]);
  printf("\n\n");

  printf("Después de tres mm_malloc:\n");
  mm_dump();

  mm_free(texto); // libera el bloque del medio
  printf("\nDespués de liberar texto:\n");
  mm_dump();

  mm_free(arreglo); // se fusiona con el hueco vecino
  printf("\nDespués de liberar arreglo (se fusiona con el hueco):\n");
  mm_dump();

  mm_free(valores); // todo vuelve a ser un único bloque libre
  printf("\nDespués de liberar valores:\n");
  mm_dump();
  return 0;
}
```

``` bash
gcc mm.c demo_mm.c -o demo_mm && ./demo_mm
```

``` txt
0 10 20 30 40 50 60 70 80 90 

Después de tres mm_malloc:
región 0 (65536 bytes):
  [USADO 352] [LIBRE 65168] [USADO 16]

Después de liberar texto:
región 0 (65536 bytes):
  [USADO 64] [LIBRE 224] [USADO 64] [LIBRE 65168] [USADO 16]

Después de liberar arreglo (se fusiona con el hueco):
región 0 (65536 bytes):
  [LIBRE 288] [USADO 64] [LIBRE 65168] [USADO 16]

Después de liberar valores:
región 0 (65536 bytes):
  [LIBRE 65520] [USADO 16]
```

Conviene seguir la salida paso a paso:

1. Los tres bloques usados miden 64 (`10 * 4 = 40` → 48 + 16), 224 (`200` → 208 + 16) y 64 (`5 * 8 = 40` → 48 + 16) bytes: 352 en total. El resto de la región queda como un solo bloque libre. Los 16 bytes finales son el separador de la región.
2. Al liberar `texto` (el bloque del medio) aparece un hueco de 224 bytes entre dos bloques usados.
3. Al liberar `arreglo`, su bloque es vecino del hueco, así que **se fusionan**: un solo bloque libre de 64 + 224 = 288.
4. Al liberar `valores`, se fusiona con ambos lados y toda la región vuelve a ser un solo bloque libre.

## Estrategias de ajuste

`find_fit` es el único lugar donde el asignador decide *qué bloque libre usar*. Cambiar esa decisión cambia el rendimiento y la fragmentación. Las estrategias clásicas son:

| Estrategia | Qué bloque elige | Ventajas | Desventajas |
| ---------- | ---------------- | -------- | ----------- |
| Primer ajuste (*first fit*) | el primero que sirve | rápido; deja huecos grandes al final | acumula trocitos pequeños al inicio de la lista |
| Siguiente ajuste (*next fit*) | el primero que sirve, buscando desde donde terminó la búsqueda anterior | evita recorrer siempre los mismos bloques del inicio | en la práctica fragmenta más que el primer ajuste |
| Mejor ajuste (*best fit*) | el más pequeño que sirve | desperdicia poco en cada asignación | recorre toda la lista; deja muchos huecos diminutos |
| Peor ajuste (*worst fit*) | el más grande | el sobrante suele ser aún utilizable | destruye los bloques grandes que harán falta después |

El código ya incluye primer y mejor ajuste. Se cambia la estrategia en tiempo de ejecución:

``` c
mm_set_strategy(MM_BEST_FIT);
```

## Referencias

* Kernighan, B. W. y Ritchie, D. M. *El lenguaje de programación C*, 2.ª ed., Prentice Hall, 1991.

## Ejercicios


1. Modifique `arena_alloc(n)` a `arena_alloc(n, i)` para que permita dos áreas de asignación independientes, identificadas por `i`, pero que compartan la misma pila. Un área asigna de abajo hacia arriba y la otra de arriba hacia abajo. Cuando ambas se unen es porque la pila está llena.
2. Implemente `void *mm_calloc(size_t count, size_t size)`, que reserva `count * size` bytes inicializados en cero. **Cuidado:** debe detectar el desbordamiento de la multiplicación y devolver `NULL`.
3. Implemente `void *mm_realloc(void *ptr, size_t n)` de la forma más sencilla: reserve un bloque nuevo, copie los datos, libere el anterior y maneje los casos `ptr == NULL` y `n == 0`.
4. Mejore `mm_dump` para que muestre también la dirección de cada bloque y, al final, el porcentaje de memoria libre y el número de bloques libres.
5. Agregue la estrategia del **siguiente ajuste** (*next fit*): la búsqueda no vuelve al inicio de la lista, sino que continúa desde donde terminó la anterior. Incluya una prueba que muestre la diferencia con el primer ajuste.
6. Agregue la estrategia del **peor ajuste** (*worst fit*), que busca el bloque más grande. Incluya una prueba.
7. Agregue la estrategia de la **mitad** (*half fit*), que busca bloques en un rango del 50 % alrededor del tamaño solicitado. Incluya una prueba. Referencia: <https://ieeexplore.ieee.org/document/528746>
8. Implemente `double mm_fragmentation(void)`, que calcule la **fragmentación externa** como `1 - (bloque libre más grande / total libre)`. Escriba un programa que reserve y libere bloques de tamaños variables e imprima la fragmentación después de cada operación.
9. Modifique el asignador para crear y asignar **bloques de tamaño fijo** (todas las solicitudes reciben un bloque del mismo tamaño).
10. Modifique el asignador para manejar **dos tamaños fijos** de bloque (grandes y pequeños): cada solicitud recibe un bloque del tamaño más pequeño que le sirva.
11. Escriba `int mm_check(void)`, un verificador de integridad que recorra la lista de bloques y compruebe que los tamaños son válidos, que ningún bloque se solapa con otro y que los bloques libres adyacentes están fusionados. Debe imprimir el primer problema que encuentre y retornar 0 si todo es correcto.
12. Escriba un programa de *benchmark* que ejecute la misma secuencia aleatoria de 1000 solicitudes y liberaciones con cada estrategia (primer, mejor, siguiente y peor ajuste) y compare el tiempo total y el número de bloques libres al final. ¿Cuál estrategia es mejor y en qué condiciones?
13. Agregue las operaciones `mm_save(ptr, archivo)` y `mm_load(ptr, archivo)`, que escriben y leen el contenido de un bloque desde un archivo. Escriba un programa de ejemplo: asigne un arreglo de enteros, escriba valores en él, guárdelo, modifique el arreglo, recupérelo del archivo e imprímalo.
