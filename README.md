# unixlib

`fork`, `wait/waitpid`, `getppid`, `getuid`, `pipe`, señales (`signal`, `sigaction`,
`kill`, `raise`, `pause`, `alarm`, `sigprocmask`...) y **System V IPC** (memoria compartida,
semaforos y colas de mensajes) para Windows con
**TDM-GCC / MinGW-w64** (>= 9.2, Windows 7+). **Header-only.** En Unix las cabeceras
reenvian a POSIX.

## Uso

```
gcc -Iinclude -Icompat prog.c -o prog.exe      # TDM-GCC
mingw32-make test                              # compila y ejecuta las pruebas
```

- `-Iinclude`: `#include <unixlib/unixlib.h>` (o `fork.h`, `pipes.h`, `signals.h`, `shm.h`, `sem.h`, `msg.h`).
- `-Icompat` (opcional): hace que el codigo Unix compile **sin modificar**:
  `<unistd.h>`, `<signal.h>`, `<sys/wait.h>`, `<sys/shm.h>`, `<sys/sem.h>`,
  `<sys/msg.h>` y `<sys/ipc.h>` aportan tambien `fork`, `pipe`, `wait`,
  `sigaction`, `kill`, `pause`, `alarm`... (`tests/tutorial/` lo comprueba).
- Los nombres POSIX son macros; `-DUNIXLIB_NO_POSIX_NAMES` deja solo `ufork_*`,
  `upipe_*`, `usig_*`.
- El estado interno usa `__declspec(selectany)`: una sola instancia aunque incluyas las
  cabeceras desde varios `.c` (`tests/test_multi_*.c`).

## Compilar desde macOS/Linux hacia Windows (cross-compilation)

En macOS: `brew install mingw-w64` (trae `x86_64-w64-mingw32-gcc` y `i686-w64-mingw32-gcc`).

```
make cross        # build/win64/*.exe  (64 bits, enlazados en estatico)
make cross32      # build/win32/*.exe  (32 bits)
make cross-zip    # dist/unixlib-windows.zip para copiar a Windows
make cross-test   # ejecuta los .exe con Wine, si lo tienes instalado
```

Los `.exe` no necesitan DLL de MinGW (`-static`). Para compilar tu propio programa:
`x86_64-w64-mingw32-gcc -Iinclude -Icompat -static prog.c -o prog.exe`. Wine es opcional y
no siempre funciona en Apple Silicon: lo normal es copiar los `.exe` a Windows y ejecutarlos allí.

## Como funciona fork() (importante)

El hijo es el mismo .exe relanzado con la misma linea de comandos, que **re-ejecuta
main() desde el principio**. Su fork() devuelve 0; hasta llegar alli, los fork() previos
devuelven un pid ficticio, pipe() devuelve los pipes heredados y stdout/stderr se
silencian (`ufork_replaying()`). Consecuencias: el codigo anterior a fork() debe ser
determinista y sin efectos externos (archivos, red, entrada del usuario, rand sin semilla
fija); las variables no se copian; `getpid()` previo al fork da el pid del hijo.
fork() anidados (el hijo hace fork) funcionan con el mismo mecanismo.

## Pipes

fds reales del CRT: `read/write/close/dup/dup2` funcionan sin envoltorios (tambien
`dup2(fd[1], STDOUT_FILENO)`), y `read()` devuelve 0 en EOF. Solo los pipes creados con
`pipe()` y aun abiertos pasan al hijo; archivos y sockets no. Cierra en cada proceso los
extremos que no uses, o el lector no vera EOF.

## Procesos

- Estado de wait: `WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG`, `WNOHANG`.
- Huerfanos: el hijo sobrevive al padre y `getppid()` devuelve 1.
  `-DUFORK_KILL_ORPHANS`: los hijos mueren con el padre (Job Object).
- Zombies: no existen en Windows; `wait()` funciona pero `ps` no los muestra.
- `getuid()/geteuid()/getgid()` devuelven 1000 (valor fijo).
- Maximo 64 hijos simultaneos por proceso, 128 pipes.

## Señales

Numeros como Linux: SIGHUP 1, SIGINT 2, SIGQUIT 3, SIGKILL 9, SIGUSR1 10, SIGUSR2 12,
SIGALRM 14, SIGTERM 15, SIGCHLD 17, SIGCONT 18. Capturables con `signal/sigaction`: esas
(salvo SIGKILL), SIGABRT, y SIGILL/SIGFPE/SIGSEGV via CRT. Ctrl+C/Break = SIGINT;
cerrar consola/logoff/apagado = SIGTERM.

`kill(pid, sig)` funciona hacia hijos de fork() y hacia cualquier proceso que use
unixlib y ya haya instalado un handler (o llamado a `pause/sigsuspend/alarm`). Sin buzon
se aplica la accion por defecto (terminar; SIGCHLD/SIGCONT se ignoran). Una señal enviada
a un hijo que aun arranca queda pendiente (se entrega con 100 ms de margen,
`-DUFORK_TERM_GRACE_MS=n`). `kill(-pid, s)` actua sobre `pid`; `kill(0, s)` sobre todos los
demas procesos con buzon unixlib.

Diferencias con POSIX: el handler corre en un hilo vigilante (no interrumpe al principal)
y las entregas son en serie; la mascara es de todo el proceso; `sa_mask`, `SA_RESTART`,
`SA_RESETHAND`, `SA_NODEFER` se ignoran; `siginfo_t` solo trae `si_signo`; SIGCHLD no se
genera solo; SIGSTOP no esta soportada; sin handler, SIGTERM/SIGUSR1... terminan el
proceso y `wait()` lo informa como señal.

## IPC de System V (shm.h, sem.h, msg.h)

`shmget/shmat/shmdt/shmctl`, `semget/semop/semctl`, `msgget/msgsnd/msgrcv/msgctl`, `ftok`,
`getpagesize`, `key_t`, `IPC_CREAT/IPC_EXCL/IPC_NOWAIT/IPC_RMID/IPC_STAT`, `SETVAL`,
`struct sembuf`, `MSG_NOERROR/MSG_EXCEPT`. Cada objeto es un *file mapping* con nombre
derivado de la key (`Local\unixlib_shm_k000003e7`...), asi que procesos con la misma key
(incluso no emparentados) ven el mismo objeto. Un mutex y un evento nombrados bloquean y
despiertan a los procesos en `semop`/`msgsnd`/`msgrcv`.

- `union semun` **no** la define la libreria (igual que glibc): la define tu programa.
- Tras `IPC_RMID`, la misma key crea un objeto nuevo y a cero aunque otro proceso aun
  conserve abierto el viejo.
- `IPC_PRIVATE` funciona; los hijos reencuentran el objeto por el replay de fork().
- Colas: 16384 bytes de datos, mensaje maximo 8192 (como Linux). `msgsz` no incluye el
  `long type`, que en Windows mide 4 bytes.

**Replay de fork() y IPC.** El hijo repite las llamadas anteriores al fork. Para que no
destruyan el estado compartido, mientras `ufork_replaying()` es verdadero:
`shmget/semget/msgget` abren el objeto existente; `semop`, `msgsnd`, `msgrcv`,
`semctl(SETVAL/SETALL/IPC_RMID)`, `shmctl(IPC_RMID)` y `msgctl(IPC_RMID)` no hacen nada;
`shmat` da una vista **privada** (copy-on-write), de modo que `*shm = 0;` o
`sprintf(shm, ...)` repetidos no tocan el segmento real. Al terminar el replay esa vista
se sustituye, **en la misma direccion**, por la compartida. Limites: un `msgrcv` hecho antes
del fork no se reproduce (el hijo no obtiene ese dato); hasta 64 objetos vivos por tipo y
128 vistas por proceso; IPC_PRIVATE solo se hereda un nivel (hijo directo).

Diferencias con Unix: `SEM_UNDO` se ignora; un `semop/msgrcv` bloqueado no se interrumpe
con señales (sin `EINTR`); `shmat` devuelve direcciones alineadas a 128 bytes, no a
pagina; `shm_nattch` es aproximado (un proceso que muere sin `shmdt` no lo decrementa);
no hay permisos ni `ipcs/ipcrm`; los objetos desaparecen cuando termina el ultimo proceso
que los tiene abiertos.

## Estado de la verificacion

Compila sin avisos (`-Wall -Wextra`) para x86_64 e i686 Windows con cabeceras mingw-w64,
incluido enlazado con dos unidades de traduccion y el codigo de los tutoriales (procesos,
señales, pipes e IPC) sin modificar. **No se ha ejecutado en Windows**: corre `mingw32-make test` (`test_all.exe` y `test_ipc.exe`) en tu TDM-GCC; `test_ipc` comprueba
el caso mas delicado: que el replay del hijo no pise la memoria compartida ya modificada.
