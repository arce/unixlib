# Linux, macOS y Windows (MSYS2/MinGW-w64): use "make" o "mingw32-make"
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g
SAN     = -fsanitize=address,undefined -fno-omit-frame-pointer

ifeq ($(OS),Windows_NT)
EXE = .exe
CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
endif

all: demo_arena$(EXE) demo_mm$(EXE)

demo_arena$(EXE): demo_arena.c arena.c arena.h
	$(CC) $(CFLAGS) demo_arena.c arena.c -o $@

demo_mm$(EXE): demo_mm.c mm.c mm.h
	$(CC) $(CFLAGS) demo_mm.c mm.c -o $@

test_arena$(EXE): test_arena.c arena.c arena.h
	$(CC) $(CFLAGS) test_arena.c arena.c -o $@

test_mm$(EXE): test_mm.c mm.c mm.h
	$(CC) $(CFLAGS) test_mm.c mm.c -o $@

test: test_arena$(EXE) test_mm$(EXE)
	./test_arena$(EXE)
	./test_mm$(EXE)

# Programa con errores, compilado con los sanitizers de gcc/clang
# (solo Linux y macOS: MinGW no los incluye).
errores: errores.c
	$(CC) $(CFLAGS) $(SAN) errores.c -o $@

clean:
	rm -f demo_arena demo_mm test_arena test_mm errores *.exe
	rm -rf *.dSYM

.PHONY: all test clean
