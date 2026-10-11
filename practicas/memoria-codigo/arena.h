// arena.h: asignador de memoria de tipo pila (header-only, un solo include)
#ifndef ARENA_H
#define ARENA_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define POOL_SIZE 10000                // tamaño del espacio disponible
#define ARENA_ALIGNMENT alignof(max_align_t) // todo puntero devuelto queda alineado

static alignas(max_align_t) unsigned char pool[POOL_SIZE]; // el espacio en sí
static unsigned char *top = pool; // primer byte libre

// Reserva n bytes de la pila. Devuelve NULL si no hay espacio.
static void *arena_alloc(size_t n) {
  if (n == 0 || n > POOL_SIZE)
    return NULL;

  n = (n + ARENA_ALIGNMENT - 1) & ~(ARENA_ALIGNMENT - 1); // redondea hacia arriba

  size_t available = (size_t)(pool + POOL_SIZE - top);
  if (n > available)
    return NULL;

  void *p = top;
  top += n;
  return p;
}

// Libera p y TODO lo que se reservó después de p.
static void arena_free(void *p) {
  uintptr_t addr = (uintptr_t)p;
  if (addr >= (uintptr_t)pool && addr < (uintptr_t)top)
    top = p; // "rebobina" la pila hasta p
}

// Cantidad de bytes actualmente en uso (útil para depurar).
static size_t arena_used(void) { return (size_t)(top - pool); }

#endif // ARENA_H