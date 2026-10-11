// mm.h: asignador de memoria con lista libre (header-only, un solo include)
#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h> // VirtualAlloc
#else
#include <sys/mman.h> // mmap
#include <unistd.h>   // sysconf

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON // macOS
#endif
#endif

typedef enum {
  MM_FIRST_FIT, // primer ajuste: el primer bloque libre que sirva
  MM_BEST_FIT   // mejor ajuste: el bloque libre más pequeño que sirva
} mm_strategy_t;

#define ALIGNMENT 16            // todo bloque es múltiplo de 16 bytes
#define CHUNK_SIZE (64 * 1024)  // cuánto pedimos al sistema cada vez
#define MAX_CHUNKS 64

// Encabezado que precede a cada bloque, libre o usado.
typedef struct block {
  size_t size;         // tamaño total del bloque en bytes (con encabezado)
  struct block *next;  // siguiente bloque libre (solo si el bloque está libre)
} block_t;

#define HEADER_SIZE sizeof(block_t)
_Static_assert(sizeof(block_t) == ALIGNMENT, "se asume un sistema de 64 bits");

// Marca que se guarda en `next` mientras el bloque está en uso. Sirve para
// detectar un free() inválido o repetido.
#define ALLOC_MAGIC ((block_t *)(uintptr_t)0xA110C8ED)

static block_t *free_list = NULL; // bloques libres, ordenados por dirección
static mm_strategy_t strategy = MM_FIRST_FIT;

// Registro de las regiones pedidas al sistema (solo para mm_dump).
static struct {
  char *base;
  size_t size;
} chunks[MAX_CHUNKS];
static size_t nchunks = 0;
static size_t heap_bytes = 0;

static size_t align_up(size_t n, size_t alignment) {
  return (n + alignment - 1) & ~(alignment - 1);
}

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

static void *mm_malloc(size_t n) {
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

static void mm_free(void *ptr) {
  if (ptr == NULL)
    return;

  block_t *b = (block_t *)ptr - 1; // retrocede hasta el encabezado
  if (b->next != ALLOC_MAGIC) {
    fprintf(stderr, "mm_free: puntero inválido o doble free (%p)\n", ptr);
    abort();
  }
  insert_free(b);
}

static void mm_set_strategy(mm_strategy_t s) { strategy = s; }

static size_t mm_free_blocks(void) {
  size_t count = 0;
  for (block_t *b = free_list; b != NULL; b = b->next)
    count++;
  return count;
}

static size_t mm_free_bytes(void) {
  size_t total = 0;
  for (block_t *b = free_list; b != NULL; b = b->next)
    total += b->size;
  return total;
}

static size_t mm_heap_bytes(void) { return heap_bytes; }

// Imprime, para cada región, los bloques libres y usados en orden de
// dirección. Los tamaños incluyen el encabezado de 16 bytes.
static void mm_dump(void) {
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

#endif // MM_H