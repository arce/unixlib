# Manejo de archivos en Unix

En la industria del software, la mayoría de los programas se escriben para almacenar la información obtenida del programa. Una de estas formas es almacenar la información obtenida en un archivo. Las diferentes operaciones que se pueden realizar en un archivo son: 

1. Creación de un nuevo fichero (fopen() con atributos como "a" o "a+" o "w" o "w+")
2. Apertura de un archivo existente (fopen())
3. Lectura de un archivo (fread(), fscanf() o fgets())
4. Escribir en un archivo (fwrite(), fprintf() o fputs())
5. Desplazarse a una ubicación específica de un archivo (fseek(), rewind())
6. Cierre de un archivo (fclose())

## Escribiendo y leyendo datos en C

Las funciones fread/fwrite se utilizan para leer/escribir datos desde/a un archivo abierto por la función fopen. 

> ```c
> size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
> 
> size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
> ```

Las funciones fread/fwrite se utilizan para leer/escribir datos desde/a un fichero abierto por la función fopen. Estas funciones aceptan tres argumentos. El primer argumento es un puntero al búfer utilizado para leer/escribir los datos. Los datos leídos/escritos se presentan en forma de elementos 'nmemb' de tamaño 'size' bytes.

En caso de éxito, fread/fwrite devuelve el número de bytes realmente leídos/escritos desde/hacia el flujo abierto por la función fopen. En caso de fallo, se devuelve un número menor de byes (entonces solicitados para leer/escribir).

```C
#include <stdio.h>
#include <stdlib.h>

struct Employee {
  int id;
  char name[10];
  int age;
};

void writeData(char *fileName, struct Employee emp) {
  FILE *f = fopen(fileName, "w");
  if (f != NULL) {
    int result = fwrite(&emp, sizeof(struct Employee), 1, f);

    if (result == 0) {
      printf("Error writing to the given file\n");
      exit(1);
    } else {
      printf("Structure is written to the file successfully!\n");
    }

    fclose(f);
  } else {
    printf("Error opening the file to write data\n");
    exit(1);
  }
}

void readData(char *fileName, struct Employee *empData) {
  FILE *f = fopen(fileName, "r");

  if (f != NULL) {
    while (fread(empData, sizeof(struct Employee), 1, f))
      ;
    fclose(f);
  } else {
    printf("Error opening the file to read data\n");
    exit(1);
  }
}

int main() {
  char fileName[] = "employee.dat";
  struct Employee emp = {224, "Alex", 30};
  struct Employee empResult;
  writeData(fileName, emp);
  readData(fileName, &empResult);

  printf("Data from file, Id: %d, Age: %d, Name: %s\n", empResult.id,
         empResult.age, empResult.name);

  return 0;
}
```

## Accesando registros

La función fseek() se utiliza para establecer el indicador de posición de archivo para el flujo en una nueva posición. Esta función acepta tres argumentos. 

> ```c
> int fseek(FILE *stream, long offset, int whence);
> ```

El primer argumento es el puntero del flujo FILE devuelto por la función fopen(). El segundo argumento 'offset' indica la cantidad de bytes a buscar. El tercer argumento 'whence' indica desde dónde se debe buscar el número de bytes 'offset'. Los valores disponibles para whence son SEEK_SET, SEEK_CUR, o SEEK_END.  Estos tres valores (en orden) representan el inicio del fichero, la posición actual y el final del fichero.

En caso de éxito, esta función devuelve 0, en caso contrario devuelve -1.

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
/* Prepare an array of double values. */
#define SIZE 5
  double A[SIZE] = {1.0, 2.0, 3.0, 4.0, 5.0};
  /* Write array to a file. */
  FILE *fp = fopen("test.bin", "wb");
  fwrite(A, sizeof(double), SIZE, fp);
  fclose(fp);

  /* Read the double values into array B. */
  double B[SIZE];
  fp = fopen("test.bin", "rb");

  /* Set the file position indicator in front of third double value. */
  if (fseek(fp, sizeof(double) * 2L, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed in file %s at line # %d\n", __FILE__,
            __LINE__ - 2);
    fclose(fp);
    return EXIT_FAILURE;
  }

  int ret_code = fread(B, sizeof(double), 1, fp); /* read one double value  */
  printf("ret_code == %d\n", ret_code); /* print the number of values read */
  printf("B[0] == %.1f\n", B[0]);       /* print one value */

  fclose(fp);
  return EXIT_SUCCESS;
}
```



## Control de archivos abiertos

Con la función *fcntl* se tiene control sobre un archivo abierto mediante una llamada previa a *open*. Este control va a consistir en las posibilidades de cambiar los modos permitidos de acceso al archivo, y de bloquear el acceso a parte del mismo o a su totalidad. El bloqueo tiene especial importancia cuando varios procesos trabajan simultáneamente con un archivo, y es imprescindible que los accesos a determinados registros del mismo sean atómicos.

- Un cerrojo de lectura indica que el proceso actual está leyendo del archivo, por lo que ningún otro proceso debe escribir en el área bloqueada. Puede haber varios cerrojos de lectura simultáneos sobre una misma región de un archivo.
- Un cerrojo de escritura indica que el proceso actual está escribiendo en el fichero, por lo que ningún proceso debe leer del área bloqueada. Sólo puede haber un cerrojo de escritura sobre una misma área del fichero.

Los cerrojos fijados por un proceso sobre un archivo se borran cuando el proceso termina. Además, los cerrojos no son heredados por los procesos hijos tras la llamada a *fork*. Como un ejemplo se muestra un proceso que realiza un bloqueo de escritura sobre un archivo.

```C
	#include <fcntl.h>
	#include <stdio.h>
	#include <string.h>
	#include <unistd.h>
	
	int main (int argc, char* argv[]) {
	 char* file = argv[1];
	 int fd;
	 struct flock lock;
	
	 printf ("opening %s\n", file);
	 /* Open a file descriptor to the file. */
	 fd = open (file, O_WRONLY);
	 printf ("locking\n");
	 /* Initialize the flock structure. */
	 memset (&lock, 0, sizeof(lock));
	 lock.l_type = F_WRLCK;
	 /* Place a write lock on the file. */
	 fcntl (fd, F_SETLKW, &lock);
	
	 printf ("locked; hit Enter to unlock... ");
	 /* Wait for the user to hit Enter. */
	 getchar ();
	
	 printf ("unlocking\n");
	 /* Release the lock. */
	 lock.l_type = F_UNLCK;
	 fcntl (fd, F_SETLKW, &lock);
	
	 close (fd);
	 return 0;
	}
```

## Descriptores de archivo: `open`, `read` y `write`

Además de la biblioteca estándar (`fopen`, `fread`, `fprintf`...), Unix ofrece llamadas al sistema de más bajo nivel que trabajan con **descriptores de archivo**, números enteros que identifican un archivo abierto (0 es la entrada estándar, 1 la salida estándar y 2 el error estándar). Estas llamadas no usan un buffer propio y son las que necesitan funciones como `fcntl` o `dup2`.

| Función | Para qué sirve |
| ------- | -------------- |
| `int open(ruta, banderas, permisos)` | abre o crea un archivo; devuelve el descriptor o -1 |
| `ssize_t read(fd, buf, n)` | lee hasta `n` bytes; devuelve los bytes leídos, 0 al final del archivo o -1 si hay error |
| `ssize_t write(fd, buf, n)` | escribe `n` bytes; devuelve los bytes escritos o -1 |
| `off_t lseek(fd, desplazamiento, origen)` | mueve la posición actual (como `fseek`) |
| `int close(fd)` | cierra el descriptor |

Las banderas más usadas de `open` son `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT` (crear si no existe), `O_TRUNC` (vaciar el archivo) y `O_APPEND` (escribir siempre al final). Un detalle importante: `read` puede devolver **menos** bytes de los pedidos, por lo que se debe usar su valor de retorno y no suponer que siempre lee el buffer completo. En Windows se agrega la bandera `O_BINARY` para que no se conviertan los saltos de línea (`\r\n` por `\n`); en Unix no existe, por eso el ejemplo la define como 0.

``` c
// a_copia.c: copiar un archivo con open/read/write y un buffer de 4096 bytes
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0 // solo existe en Windows: evita traducir saltos de línea
#endif

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Uso: %s origen destino\n", argv[0]);
    return 1;
  }
  int in = open(argv[1], O_RDONLY | O_BINARY);
  if (in < 0) {
    perror(argv[1]);
    return 1;
  }
  int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
  if (out < 0) {
    perror(argv[2]);
    return 1;
  }

  char buffer[4096];
  long long total = 0;
  ssize_t n;
  clock_t t0 = clock();
  while ((n = read(in, buffer, sizeof buffer)) > 0) { // read puede devolver menos de lo pedido
    if (write(out, buffer, n) != n) {
      perror("write");
      return 1;
    }
    total += n;
  }
  close(in);
  close(out);
  printf("%lld bytes copiados en %.3f s\n", total, (double)(clock() - t0) / CLOCKS_PER_SEC);
  return 0;
}
```

## Bloqueo de archivos compartidos

Cuando dos procesos modifican el mismo archivo, hay una condición de carrera: ambos pueden leer el mismo valor, modificarlo y escribirlo, y uno de los dos cambios se pierde. La solución es un **cerrojo** (*lock*) sobre el archivo, o sobre la región que se va a modificar: solo un proceso a la vez puede entrar a la sección crítica.

En Unix se usa `fcntl` (como en el ejemplo de la sección anterior); en Windows la función equivalente es `LockFileEx`. Para escribir programas que funcionen en ambos sistemas, el siguiente encabezado `cerrojo.h` ofrece las funciones `bloquear(fd, inicio, largo)` y `desbloquear(fd, inicio, largo)`, que usan una u otra según el sistema. Un `largo` de 0 significa "hasta el final del archivo".

``` c
// cerrojo.h: bloqueo de regiones de un archivo, igual en Unix y en Windows
#ifndef CERROJO_H
#define CERROJO_H

#ifdef _WIN32
#include <io.h>
#include <string.h>
#include <windows.h>

// Bloquea 'len' bytes desde 'inicio' (len = 0: hasta el final). Espera si está ocupado.
static inline int bloquear(int fd, long inicio, long len) {
  OVERLAPPED ov;
  memset(&ov, 0, sizeof ov);
  ov.Offset = (DWORD)inicio;
  DWORD bajo = len ? (DWORD)len : 0xFFFFFFFF, alto = len ? 0 : 0xFFFFFFFF;
  return LockFileEx((HANDLE)_get_osfhandle(fd), LOCKFILE_EXCLUSIVE_LOCK, 0, bajo, alto, &ov) ? 0 : -1;
}
static inline int desbloquear(int fd, long inicio, long len) {
  OVERLAPPED ov;
  memset(&ov, 0, sizeof ov);
  ov.Offset = (DWORD)inicio;
  DWORD bajo = len ? (DWORD)len : 0xFFFFFFFF, alto = len ? 0 : 0xFFFFFFFF;
  return UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, bajo, alto, &ov) ? 0 : -1;
}

#else
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static inline int bloquear(int fd, long inicio, long len) {
  struct flock fl;
  memset(&fl, 0, sizeof fl);
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = inicio;
  fl.l_len = len; // 0 = hasta el final del archivo
  return fcntl(fd, F_SETLKW, &fl);
}
static inline int desbloquear(int fd, long inicio, long len) {
  struct flock fl;
  memset(&fl, 0, sizeof fl);
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = inicio;
  fl.l_len = len;
  return fcntl(fd, F_SETLKW, &fl);
}
#endif

#endif
```

El siguiente programa es el caso clásico: varios procesos incrementan un contador guardado en un archivo. Primero se inicializa con `a_contador init` y luego se ejecutan tres copias a la vez (por ejemplo, tres consolas). Si los cerrojos funcionan, la última copia en terminar debe ver el valor 3000; sin ellos (comente las líneas de `bloquear` y `desbloquear`) el resultado es menor porque se pierden incrementos.

``` c
// a_contador.c: varios procesos incrementan un contador guardado en un archivo
// Uso: a_contador init          (crea el archivo con el valor 0)
//      a_contador               (suma 1000 veces; ejecute 3 copias a la vez)
#include "cerrojo.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

int main(int argc, char *argv[]) {
  int fd = open("contador.dat", O_RDWR | O_CREAT | O_BINARY, 0644);
  if (fd < 0) {
    perror("contador.dat");
    return 1;
  }
  if (argc > 1 && strcmp(argv[1], "init") == 0) {
    int cero = 0;
    write(fd, &cero, sizeof cero);
    close(fd);
    printf("contador inicializado en 0\n");
    return 0;
  }

  for (int i = 0; i < 1000; i++) {
    int valor = 0;
    bloquear(fd, 0, sizeof valor); // sección crítica: leer, modificar y escribir
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &valor, sizeof valor) != sizeof valor)
      valor = 0;
    valor++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &valor, sizeof valor);
    desbloquear(fd, 0, sizeof valor);
  }

  int final = 0;
  lseek(fd, 0, SEEK_SET);
  read(fd, &final, sizeof final);
  printf("proceso terminó; el contador vale ahora %d\n", final);
  close(fd);
  return 0;
}
```

Observe que solo se bloquean los 4 bytes del contador y no el archivo completo: así otros procesos podrían trabajar sobre otras partes del archivo al mismo tiempo (el *bloqueo de regiones* es la técnica que usan las bases de datos).

## Archivos de bitácora (`O_APPEND`)

Con `O_APPEND`, cada `write` se agrega al final del archivo sin importar la posición actual, lo cual es ideal para una bitácora (*log*) que varios procesos comparten. Para que las líneas no se mezclen se debe escribir cada línea con **una sola llamada** a `write`. En Unix esa llamada es atómica; en Windows, `O_APPEND` no lo garantiza entre procesos, por lo que el ejemplo usa además un cerrojo.

``` c
// a_bitacora.c: varios procesos agregan líneas a un mismo archivo de bitácora
#include "cerrojo.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <process.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

int main(void) {
  int fd = open("bitacora.log", O_WRONLY | O_CREAT | O_APPEND | O_BINARY, 0644);
  if (fd < 0) {
    perror("bitacora.log");
    return 1;
  }
  for (int i = 1; i <= 100; i++) {
    char linea[80];
    int n = snprintf(linea, sizeof linea, "proceso %d, línea %d\n", (int)getpid(), i);
    bloquear(fd, 0, 0); // en Windows, O_APPEND por sí solo no es atómico entre procesos
    write(fd, linea, n); // una sola llamada a write por línea
    desbloquear(fd, 0, 0);
  }
  close(fd);
  printf("proceso %d terminó\n", (int)getpid());
  return 0;
}
```

Ejecute tres copias a la vez y verifique con `wc -l bitacora.log` (o contando las líneas del archivo) que hay 300 líneas completas.

## Ejercicios


Los ejercicios 1–5 usan solo funciones estándar de C y corren sin cambios en Windows y en Unix. Los ejercicios 6–10 usan bloqueos de archivos: en Unix use `fcntl`; para que funcionen también en Windows use el encabezado `cerrojo.h` presentado en este tutorial (en Windows `O_APPEND` no es atómico entre procesos, así que bloquee también las escrituras del ejercicio 12).

1. Escriba un programa en C que escriba a un archivo la información de 10 empleados (código, nombre, salario). Luego escriba otro programa que imprima los datos de los empleados que se encuentran en posiciones pares del archivo. Debe usar la función `fseek` para acceder directamente a los registros (no se permite búsqueda secuencial).
2. Escriba un programa que actualice el salario del empleado número *k* (leído por teclado) en el archivo del ejercicio anterior, modificando únicamente ese registro con `fseek` y sin reescribir el resto del archivo. Muestre el registro antes y después.
3. Escriba un programa que calcule cuántos registros tiene un archivo de empleados sin leerlo completo (use `fseek(f, 0, SEEK_END)` y `ftell`) y que luego imprima los registros en orden inverso, del último al primero.
4. Escriba un programa que genere un archivo con 100 registros ordenados por código y otro que implemente búsqueda binaria sobre ese archivo usando `fseek`. Debe imprimir cuántos registros tuvo que leer para encontrar el código solicitado.
5. Implemente el borrado lógico de registros: cada registro tiene un campo `activo`. Un programa marca como inactivo el registro con un código dado, y otro programa compacta el archivo copiando únicamente los registros activos a un archivo nuevo.
6. Escriba dos programas en C. Un programa llenará el archivo con información de 10 empleados de forma incremental: agrega un empleado, duerme unos 2 segundos y luego agrega otro. Un segundo programa deberá leer periódicamente (cada 2 segundos) el archivo e imprimirlo. Usted debe usar bloqueos para impedir que el programa lector obtenga información inconsistente.
7. Escriba dos programas en C que escriban simultáneamente de forma alterna a un archivo datos de 10 empleados. Los programas deben bloquear y liberar el archivo cuando terminen de escribir.
8. Escriba un programa en C que permita votar por 4 candidatos, la opción seleccionada debe ser guardada en un archivo. Utilice bloqueos de archivos para evitar que una persona realice su voto si otra persona está votando en ese momento. Un programa adicional debe imprimir el resultado de las votaciones.
9. Escriba un programa en C que permita adquirir boletos para un concierto, la cantidad de boletos es limitada (20 boletos) así que se debe bloquear el archivo de boletos mientras otra persona está reservando. Un programa adicional debe imprimir el número de boleto asignado y el nombre de la persona que lo adquirió.
10. Escriba dos programas que realicen depósitos y retiros sobre una cuenta bancaria cuyo saldo se guarda en un registro de un archivo con varias cuentas. Cada programa debe bloquear únicamente la región del archivo correspondiente al registro de la cuenta (por ejemplo con `fcntl` y `l_start`/`l_len`), no el archivo completo.
11. Escriba un programa que copie un archivo usando `open`, `read` y `write` con un buffer de 4096 bytes, convirtiendo las letras mayúsculas a minúsculas, e imprima al final el número de bytes copiados y el tiempo empleado.
12. Escriba un programa que abra un archivo de bitácora en modo de agregado (`O_APPEND`) y escriba 100 líneas con su *PID* y un número de secuencia. Ejecute 3 copias del programa al mismo tiempo y verifique que el archivo tiene 300 líneas sin mezclar el contenido de unas con otras.
