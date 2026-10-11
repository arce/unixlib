# Linux, macOS y Windows (MSYS2/MinGW-w64): use "make" o "mingw32-make"
CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -g
LDLIBS  = -pthread

ifeq ($(OS),Windows_NT)
EXE = .exe
endif

BASES = 01_carrera 02_trylock 03_interbloqueo 04_condicion 05_timedwait \
        06_buffer 07_buffer_sem 08_lectores_escritores 09_filosofos \
        10_plantilla
PROGS = $(addsuffix $(EXE),$(BASES))

all: $(PROGS)

%$(EXE): %.c util.h semaforo.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Ejecuta todos los ejemplos con su comportamiento correcto
test: all
	./01_carrera$(EXE) con
	./02_trylock$(EXE)
	./03_interbloqueo$(EXE) mal
	./03_interbloqueo$(EXE) orden
	./04_condicion$(EXE)
	./05_timedwait$(EXE)
	./06_buffer$(EXE)
	./07_buffer_sem$(EXE)
	./08_lectores_escritores$(EXE)
	./09_filosofos$(EXE) mal
	./09_filosofos$(EXE) orden
	./09_filosofos$(EXE) camarero
	./10_plantilla$(EXE)
	@echo "make test: todo OK"

# Lo mismo, compilado con ThreadSanitizer, que detecta condiciones de carrera
# (solo Linux y macOS: MinGW no lo incluye).
tsan:
	@for p in 01_carrera 02_trylock 04_condicion 06_buffer 07_buffer_sem \
	          08_lectores_escritores 10_plantilla; do \
	  $(CC) $(CFLAGS) -fsanitize=thread $$p.c -o $${p}_tsan $(LDLIBS) || exit 1; \
	done
	./01_carrera_tsan con
	./02_trylock_tsan
	./04_condicion_tsan
	./06_buffer_tsan
	./07_buffer_sem_tsan
	./08_lectores_escritores_tsan
	./10_plantilla_tsan
	@echo "make tsan: sin condiciones de carrera"

clean:
	rm -f $(PROGS) *_tsan *_tsan.exe
	rm -rf *.dSYM

.PHONY: all test tsan clean
