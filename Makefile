# unixlib (header-only) - TDM-GCC: mingw32-make
#   mingw32-make          -> compila pruebas y ejemplos en build/
#   mingw32-make test     -> ejecuta build/test_all.exe y build/test_ipc.exe
CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=gnu11
CFLAGS  += -Iinclude -Icompat

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

TUT  := $(patsubst tests/tutorial/%.c,build/tut/%$(EXE),$(wildcard tests/tutorial/*.c))
BINS := $(TUT) build/test_all$(EXE) build/test_ipc$(EXE) build/test_multi$(EXE) \
        build/pipe_fork$(EXE) build/sigint$(EXE)

all: $(BINS)

build:
	mkdir -p build

build/test_all$(EXE): tests/test_all.c include/unixlib/*.h | build
	$(CC) $(CFLAGS) $< -o $@

build/test_ipc$(EXE): tests/test_ipc.c include/unixlib/*.h compat/sys/*.h | build
	$(CC) $(CFLAGS) $< -o $@

# Dos unidades de traduccion: comprueba que el estado compartido
# (UNIXLIB_SHARED) enlaza sin simbolos duplicados.
build/test_multi$(EXE): tests/test_multi_a.c tests/test_multi_b.c include/unixlib/*.h | build
	$(CC) $(CFLAGS) tests/test_multi_a.c tests/test_multi_b.c -o $@

# Codigo de los tutoriales, sin modificar: comprueba que compila con -Icompat
build/tut/%$(EXE): tests/tutorial/%.c include/unixlib/*.h compat/*.h compat/sys/*.h | build
	mkdir -p build/tut
	$(CC) $(CFLAGS) -w $< -o $@

build/%$(EXE): examples/%.c include/unixlib/*.h | build
	$(CC) $(CFLAGS) $< -o $@

test: build/test_all$(EXE) build/test_ipc$(EXE) build/test_multi$(EXE)
	./build/test_all$(EXE)
	./build/test_ipc$(EXE)
	./build/test_multi$(EXE)

clean:
	rm -rf build

.PHONY: all test clean
