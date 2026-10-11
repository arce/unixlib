# Manejo de directorios

## Estadísticas de Archivos

Los sistemas operativos Unix y similares (como Linux) mantienen un registro exhaustivo de cada archivo y directorio en el sistema de archivos. Esta información, conocida como **metadatos**, se almacena en estructuras de datos llamadas **inodos**. Para acceder a estos metadatos, Unix proporciona un conjunto de llamadas al sistema, siendo `stat` y `fstat` dos de las más fundamentales. Estas funciones permiten a los programas obtener un "estado" o "instantánea" informativa de un archivo en un momento dado, sin necesidad de abrir o leer su contenido.

### Diferencias Clave y Uso

La diferencia principal radica en cómo se referencia al archivo objetivo:

- **`int stat(const char *path, struct stat *buf)`**
    - **Referencia por Ruta:** Esta función toma la **ruta de acceso** (path) al archivo como una cadena de caracteres.
    - **Descripción:** Si la ruta es un enlace simbólico, `stat` **desreferencia** automáticamente el enlace y devuelve la información sobre el archivo al que apunta.
- **`int fstat(int fildes, struct stat *buf)`**
    - **Referencia por Descriptor:** Esta función trabaja con un **descriptor de archivo** (file descriptor) que debe haber sido obtenido previamente mediante una llamada como `open`, `creat`, `pipe`, o `socket`.
    - **Descripción:** Proporciona información sobre el archivo exactamente como está representado por el descriptor abierto. Si el descriptor se refiere a un enlace simbólico, `fstat` devuelve información sobre el enlace mismo, no sobre el destino.

Ambas funciones devuelven `0` en caso de éxito y `-1` en caso de error, estableciendo la variable global `errno` para indicar la causa del fallo.

### La Estructura `struct stat`

La información recuperada se almacena en una estructura de tipo `struct stat`, definida en `<sys/stat.h>`. Los campos específicos pueden variar ligeramente entre sistemas, pero generalmente incluyen:

```c
struct stat {
    dev_t     st_dev;     /* ID del dispositivo que contiene el archivo */
    ino_t     st_ino;     /* Número de inodo */
    mode_t    st_mode;    /* Modo de protección y tipo de archivo */
    nlink_t   st_nlink;   /* Número de enlaces físicos */
    uid_t     st_uid;     /* ID del usuario propietario */
    gid_t     st_gid;     /* ID del grupo propietario */
    dev_t     st_rdev;    /* ID del dispositivo (si es archivo especial) */
    off_t     st_size;    /* Tamaño total en bytes (para archivos regulares) */
    blksize_t st_blksize; /* Tamaño de bloque para E/S del sistema de archivos */
    blkcnt_t  st_blocks;  /* Número de bloques de 512B asignados */
    time_t    st_atime;   /* Hora del último acceso (atime) */
    time_t    st_mtime;   /* Hora de la última modificación (mtime) */
    time_t    st_ctime;   /* Hora del último cambio del inodo (ctime) */
};
```

#### Campos relevantes

1.  **`st_mode`:** Este es quizás el campo más importante. Contiene máscaras de bits que indican:
    - **Tipo de Archivo:** Se puede determinar si es un archivo regular (`S_ISREG(mode)`), un directorio (`S_ISDIR(mode)`), un enlace simbólico (`S_ISLNK(mode)`), un dispositivo, etc.
    - **Permisos de Acceso:** Los permisos para el usuario propietario, grupo y otros (lectura, escritura, ejecución).

2.  **`st_size`:** Proporciona el tamaño del archivo en bytes. Para archivos regulares, este es el tamaño del contenido. Para enlaces simbólicos, es la longitud de la ruta a la que apunta.

3.  **`st_ino` y `st_dev`:** Juntos, el número de inodo (`st_ino`) y el ID del dispositivo (`st_dev`) forman un identificador único para cada archivo en el sistema.

4.  **Marcas de Tiempo:**
    - **`st_atime`:** Último acceso a los datos del archivo.
    - **`st_mtime`:** Última modificación de los datos del archivo (cuando el contenido cambia).
    - **`st_ctime`:** Último cambio del inodo (cuando cambian metadatos como permisos, propietario o número de enlaces). Este campo se actualiza automáticamente cuando se modifica `st_mtime`.


### Ejemplo de Uso

```c
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_archivo>\n", argv[0]);
        return 1;
    }

    struct stat file_info;

    // Usamos stat() porque tenemos la ruta como argumento
    if (stat(argv[1], &file_info) == -1) {
        fprintf(stderr, "Error al leer el archivo '%s': %s\n",
                argv[1], strerror(errno));
        return 1;
    }

    // Imprimir información obtenida
    printf("Información para: %s\n", argv[1]);
    printf("------------------------\n");
    printf("Tamaño: %lld bytes\n", (long long)file_info.st_size);
    printf("Enlaces: %ld\n", (long)file_info.st_nlink);
    printf("Propietario UID: %d\n", file_info.st_uid);
    printf("Grupo GID: %d\n", file_info.st_gid);

    // Determinar el tipo de archivo
    if (S_ISREG(file_info.st_mode))
        printf("Tipo: Archivo regular\n");
    else if (S_ISDIR(file_info.st_mode))
        printf("Tipo: Directorio\n");
#ifdef S_ISLNK // los enlaces simbólicos solo existen en Unix
    else if (S_ISLNK(file_info.st_mode))
        printf("Tipo: Enlace simbólico\n");
#endif
    else
        printf("Tipo: Otro\n");

    // Imprimir la última fecha de modificación
    printf("Última modificación: %s", ctime(&file_info.st_mtime));

    return 0;
}
```

## Administración de directorios

Las funciones `opendir` y `readdir` forman parte de la familia de funciones para directorios en C, definidas en el encabezado `<dirent.h>`. Su propósito es proporcionar una forma estandarizada y portable de leer el contenido de un directorio, abstrayendo a los programadores de los detalles específicos de la implementación del sistema de archivos .

La función `opendir` es el punto de partida para cualquier operación de lectura de directorios. Su misión es abrir un "flujo de directorio" correspondiente a la ruta especificada y posicionarlo en la primera entrada . Si tiene éxito, devuelve un puntero a una estructura de tipo `DIR`, que actúa como un manejador para las operaciones subsequentes. Es crucial verificar que el valor devuelto no sea `NULL`, ya que esto indica un error que puede deberse a permisos insuficientes, a que la ruta no existe o a que no es un directorio, entre otras causas .

Una vez obtenido el manejador del directorio mediante `opendir`, se utiliza la función `readdir` para leer las entradas de forma secuencial . Cada llamada a `readdir` devuelve un puntero a una estructura `struct dirent` que contiene información sobre la siguiente entrada en el directorio. Cuando no hay más entradas por leer o ocurre un error, la función retorna `NULL`. Un patrón de uso común y correcto es comparar estrictamente el resultado con `false` o `NULL` en un ciclo, para evitar confusiones si algún archivo tiene un nombre que pueda evaluarse como falso en un contexto booleano, como "0" . Es importante recordar que el puntero retornado por `readdir` puede ser sobrescrito en llamadas posteriores .

Finalmente, después de completar la lectura de todas las entradas, es una buena práctica de programación cerrar el flujo del directorio utilizando la función `closedir`. Esto libera los recursos del sistema asociados con el manejador `DIR`. 

### Ejemplo de Uso

El siguiente fragmento de código ilustra el flujo completo de trabajo con estas funciones, mostrando cómo listar el contenido de un directorio, excluyendo las entradas especiales "." y "..".

```c
#include <stdio.h>
#include <dirent.h>

int main() {
    DIR *directorio;
    struct dirent *entrada;

    // Abrir el directorio actual
    if ((directorio = opendir(".")) == NULL) {
        perror("No se pudo abrir el directorio");
        return 1;
    }

    // Leer entradas secuencialmente
    while ((entrada = readdir(directorio)) != NULL) {
        // Omitir las entradas '.' y '..'
        if (entrada->d_name[0] != '.') {
            printf("Encontrado: %s\n", entrada->d_name);
        }
    }

    // Cerrar el flujo de directorio
    closedir(directorio);
    return 0;
}
```

## Recorrido recursivo de un árbol de directorios

Un directorio puede contener otros directorios, de modo que para visitar todo un árbol se necesita una función **recursiva**: lista el directorio y, cada vez que encuentra un subdirectorio, se llama a sí misma con la ruta de ese subdirectorio. Para saber si una entrada es un directorio se usa `stat` con la ruta completa (que se arma con `snprintf`) y la macro `S_ISDIR`. Se omiten las entradas especiales `.` y `..`; de lo contrario la recursión no terminaría nunca.

El siguiente programa imprime el árbol indentado según la profundidad y calcula cuántos archivos hay y cuántos bytes ocupan. El separador `/` funciona también en Windows.

``` c
// d_recorrido.c: recorrido recursivo de un directorio
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void recorrer(const char *ruta, int nivel, long long *total, int *archivos) {
  DIR *d = opendir(ruta);
  if (d == NULL) {
    perror(ruta);
    return;
  }
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
      continue;

    char hijo[1024];
    snprintf(hijo, sizeof hijo, "%s/%s", ruta, e->d_name);

    struct stat st;
    if (stat(hijo, &st) == -1) {
      perror(hijo);
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      printf("%*s%s/\n", nivel * 2, "", e->d_name);
      recorrer(hijo, nivel + 1, total, archivos); // recursión sobre el subdirectorio
    } else {
      printf("%*s%s (%lld bytes)\n", nivel * 2, "", e->d_name, (long long)st.st_size);
      *total += st.st_size;
      (*archivos)++;
    }
  }
  closedir(d); // se cierra antes de volver al nivel anterior
}

int main(int argc, char *argv[]) {
  const char *inicio = argc > 1 ? argv[1] : ".";
  long long total = 0;
  int archivos = 0;
  recorrer(inicio, 0, &total, &archivos);
  printf("\n%d archivos, %lld bytes en total\n", archivos, total);
  return 0;
}
```

Pruébelo con `./d_recorrido .` o con la ruta de cualquier carpeta. Note que `closedir` se llama antes de salir de cada nivel: si un directorio tiene muchos niveles de profundidad, cada nivel mantiene abierto su propio flujo hasta terminar, por lo que es importante cerrarlos siempre.

## Crear, renombrar y borrar

Las funciones básicas para administrar directorios y archivos son:

| Función | Efecto |
| ------- | ------ |
| `mkdir(ruta, permisos)` | crea un directorio |
| `rmdir(ruta)` | borra un directorio, solo si está vacío |
| `rename(origen, destino)` | cambia el nombre o mueve un archivo o directorio |
| `remove(ruta)` | borra un archivo |
| `chdir(ruta)` | cambia el directorio actual del proceso |

Todas devuelven 0 si tienen éxito y -1 si fallan, dejando el motivo en `errno` (se puede imprimir con `perror`). Hay una diferencia entre sistemas: en Windows `mkdir` recibe solo la ruta, sin los permisos, por lo que el ejemplo define una macro que cubre ambos casos.

``` c
// d_gestion.c: crear, renombrar y borrar directorios y archivos
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define CREAR_DIR(ruta) mkdir(ruta) // en Windows mkdir no recibe permisos
#else
#define CREAR_DIR(ruta) mkdir(ruta, 0755)
#endif

static void resultado(const char *accion, int r) {
  printf("%-28s %s\n", accion, r == 0 ? "ok" : "falló");
  if (r != 0)
    perror("   motivo");
}

int main(void) {
  resultado("mkdir prueba", CREAR_DIR("prueba"));

  FILE *f = fopen("prueba/datos.txt", "w");
  if (f != NULL) {
    fputs("hola\n", f);
    fclose(f);
  }

  resultado("rename datos -> final", rename("prueba/datos.txt", "prueba/final.txt"));

  struct stat st;
  if (stat("prueba/final.txt", &st) == 0)
    printf("final.txt mide %lld bytes\n", (long long)st.st_size);

  resultado("rmdir prueba (no vacío)", rmdir("prueba")); // debe fallar: aún tiene un archivo
  resultado("remove final.txt", remove("prueba/final.txt"));
  resultado("rmdir prueba", rmdir("prueba"));
  return 0;
}
```

El programa prueba además que `rmdir` falla mientras el directorio no esté vacío.

## Directorio actual y rutas absolutas

Toda ruta relativa se interpreta a partir del **directorio actual** del proceso, que se consulta con `getcwd` y se cambia con `chdir`. Para obtener la ruta completa de una ruta relativa se usa `realpath` en Unix y `_fullpath` en Windows.

``` c
// d_ruta.c: directorio actual, cambio de directorio y ruta absoluta
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define RUTA_ABSOLUTA(rel, buf) _fullpath((buf), (rel), sizeof(buf))
#else
#define RUTA_ABSOLUTA(rel, buf) realpath((rel), (buf))
#endif

int main(void) {
  char actual[1024], absoluta[1024];

  if (getcwd(actual, sizeof actual) != NULL)
    printf("directorio actual : %s\n", actual);

  if (RUTA_ABSOLUTA(".", absoluta) != NULL)
    printf("'.' equivale a    : %s\n", absoluta);

  if (chdir("..") == 0 && getcwd(actual, sizeof actual) != NULL)
    printf("después de cd ..  : %s\n", actual);
  else
    perror("chdir");
  return 0;
}
```

> **Nota sobre Windows:** `stat` funciona, pero los permisos (`rwx`) y el propietario son aproximaciones, y los enlaces simbólicos (`S_ISLNK`, `lstat`, `readlink`) no existen, por lo que los programas de este tutorial evitan usarlos o los protegen con `#ifdef S_ISLNK`.

## Ejercicios


Los ejercicios solo usan `stat`, `opendir`/`readdir` y `S_ISREG`/`S_ISDIR`, por lo que no dependen de enlaces simbólicos.

1. Escribir un programa que reciba una ruta y liste recursivamente todos los archivos y directorios, mostrando para cada archivo: su ruta completa, tamaño en bytes y fecha de última modificación. Usar la opción `-l` del comando `ls` como referencia de formato.
2. Desarrollar un programa que tome como argumentos un nombre de archivo y un directorio de inicio, y realice una búsqueda recursiva. Para cada ocurrencia encontrada, debe imprimir la ruta absoluta. Ejemplo: `./mi_find /home/proyectos "config.txt"`.
3. Crear un programa que calcule el tamaño total que ocupa un directorio específico, sumando recursivamente el tamaño de todos los archivos regulares que contiene. Ignorar el tamaño de los propios directorios. Mostrar el resultado en un formato legible (ej. bytes, KB, MB).
4. Implementar un programa que muestre por pantalla la estructura de archivos y subdirectorios de un directorio dado, en formato de árbol (similar al comando `tree`), indentando según la profundidad.
5. Diseñar un programa que recorra recursivamente un directorio y clasifique los archivos por su extensión. Al finalizar, se debe mostrar un resumen con el número de archivos y el tamaño total agrupado por cada extensión encontrada (ej. `.c`: 15 archivos, 150 KB).
6. Escribir un programa que liste los archivos de un directorio mostrando sus permisos en formato `rwxr-xr-x` (a partir de `st_mode`), el tamaño y el nombre, como lo hace `ls -l`.
7. Desarrollar un programa que reciba un directorio y un número *N* e imprima los *N* archivos regulares más grandes que encuentre recursivamente, ordenados de mayor a menor.
8. Escribir un programa que imprima los archivos de un directorio (recursivamente) que fueron modificados en las últimas 24 horas, junto con su fecha y hora de modificación.
9. Crear un programa que organice los archivos de un directorio (sin recorrer subdirectorios) moviéndolos a subdirectorios según su extensión (por ejemplo, `.jpg` a `jpg/`, `.pdf` a `pdf/`). Debe crear cada subdirectorio si no existe y usar `rename` para mover los archivos.
10. Implementar un programa que busque recursivamente los directorios vacíos dentro de una ruta dada y los liste. Opcionalmente, con un argumento `-r`, debe eliminarlos con `rmdir`.
11. Escribir un programa que reciba las rutas de dos directorios y muestre qué nombres de archivo existen en uno pero no en el otro, y qué archivos con el mismo nombre tienen distinto tamaño o fecha de modificación (solo el primer nivel; no es necesario copiar archivos).
12. Construir un programa que encuentre archivos candidatos a duplicados dentro de un directorio y sus subdirectorios: agrupe los archivos que tengan el mismo tamaño e imprima cada grupo (no es necesario comparar el contenido).
