# El lenguaje C

## 1. Introducción a C

### 1.1. ¿Qué es C?
C es un lenguaje de programación de propósito general, de nivel medio (aunque a menudo se le considera de bajo nivel por su capacidad de acceso a la memoria) y compilado. Fue desarrollado por Dennis Ritchie entre 1969 y 1973 en los Laboratorios Bell para ser utilizado en el sistema operativo UNIX. Es uno de los lenguajes más influyentes de la historia y ha servido de base para muchos otros como C++, C#, Java, Python, entre otros .

### 1.2. Filosofía del Lenguaje
La filosofía de C se centra en la **eficiencia**, el **control** y la **portabilidad**. Ofrece un conjunto de características mínimas pero muy potentes, dando al programador un control casi total sobre los recursos del hardware, especialmente la memoria. Esta cercanía al hardware le ha valido el apodo de "ensamblador de alto nivel" . Su principal desventaja es que esta libertad conlleva una gran responsabilidad; la gestión manual de la memoria y el uso de punteros son fuentes comunes de errores difíciles de depurar .

### 1.3. Configuración del Entorno de Desarrollo
Para programar en C, necesitarás un editor de texto y un compilador.
- **Editor de texto:** Puedes usar cualquier editor (Notepad, VS Code, Sublime Text, Vim, etc.).
- **Compilador:** El compilador convierte tu código fuente en un programa ejecutable. Los más comunes son el **GCC (GNU Compiler Collection)** en sistemas Linux/macOS y **MinGW** o **Clang** en Windows.
- **Compilación:** El proceso básico desde la terminal es:
  ```bash
  gcc nombre_archivo.c -o nombre_ejecutable
  ./nombre_ejecutable
  ```
  Donde `nombre_archivo.c` es tu código fuente, `-o nombre_ejecutable` define el nombre del programa resultante, y la última línea lo ejecuta .

### 1.4. Tu Primer Programa en C
El programa "Hola Mundo" es el clásico punto de partida. Ilustra la estructura básica de un programa en C.

```c
#include <stdio.h>  // Incluye la librería estándar de entrada/salida

// La función main es el punto de entrada de todo programa en C
int main() {
    // printf es una función de la librería stdio.h para imprimir en pantalla
    printf("¡Hola, Mundo!\n"); // \n es un carácter de nueva línea

    return 0; // Indica que el programa terminó exitosamente
}
```
**Salida esperada:**
```
¡Hola, Mundo!
```
**Análisis:**
- `#include <stdio.h>`: Es una directiva del preprocesador que incluye el contenido del archivo de cabecera `stdio.h`, necesario para usar funciones como `printf` .
- `int main()`: Declara la función principal. Todo programa en C debe tener una función `main`. El `int` indica que devuelve un valor entero al sistema operativo .
- `printf("¡Hola, Mundo!\n");`: Llama a la función `printf` para imprimir texto en la consola. El `\n` mueve el cursor a la siguiente línea .
- `return 0;`: Devuelve el valor `0` al sistema operativo para indicar que la ejecución fue correcta. Cualquier valor distinto de cero suele indicar un error .

---

## 2. Fundamentos del Lenguaje

### 2.1. Estructura de un Programa
Un programa en C se compone de:
1.  **Directivas del preprocesador:** Comienzan con `#` (ej. `#include`, `#define`).
2.  **Declaraciones globales:** Variables o funciones que existen fuera de cualquier otra función.
3.  **Función `main`:** Es el punto de entrada obligatorio.
4.  **Otras funciones:** Puedes definir tus propias funciones para organizar el código.

### 2.2. Tipos de Datos Primitivos
C tiene varios tipos de datos básicos para almacenar diferentes tipos de información .

| Tipo     | Descripción                                            | Tamaño (aprox.) | Rango (ejemplo para 32 bits)   |
| :------- | :----------------------------------------------------- | :-------------- | :----------------------------- |
| `char`   | Carácter o entero pequeño de 1 byte                    | 1 byte          | -128 a 127 o 0 a 255           |
| `int`    | Entero                                                 | 4 bytes         | -2,147,483,648 a 2,147,483,647 |
| `float`  | Número de punto flotante de precisión simple           | 4 bytes         | 1.2e-38 a 3.4e+38              |
| `double` | Número de punto flotante de precisión doble            | 8 bytes         | 2.3e-308 a 1.7e+308            |
| `void`   | Sin valor (se usa para funciones que no retornan nada) | N/A             | N/A                            |

**Modificadores de tipo:** Pueden usarse para alterar el tamaño o el signo de los tipos básicos.
- `short` y `long`: Modifican el tamaño de `int`. Ej: `short int`, `long int` (o simplemente `short`, `long`).
- `signed` y `unsigned`: Modifican si el tipo puede almacenar números negativos o solo positivos. Ej: `unsigned int`.

### 2.3. Variables, Constantes y Literales
- **Variables:** Son espacios de memoria con un nombre asociado donde se almacenan datos. Deben declararse antes de usarse.
  ```c
  int edad;               // Declaración de una variable entera
  float altura = 1.75;    // Declaración e inicialización
  char inicial = 'J';     // Un carácter se encierra entre comillas simples ''
  ```
- **Constantes (`#define`):** Son valores que no cambian. Se definen con la directiva `#define` antes de la compilación.
  ```c
  #define PI 3.14159
  #define NOMBRE "Mi Programa"
  ```
- **Literales:** Son valores escritos directamente en el código. Ej: `'a'`, `"Hola"`, `10`, `3.14`.

### 2.4. Operadores
C proporciona una amplia variedad de operadores .
- **Aritméticos:** `+`, `-`, `*`, `/`, `%` (módulo o resto de división).
- **Relacionales:** `==`, `!=`, `<`, `>`, `<=`, `>=` (comparan valores, devuelven 1 si es verdadero, 0 si es falso).
- **Lógicos:** `&&` (Y), `||` (O), `!` (NO) (operan sobre valores booleanos).
- **De asignación:** `=`, `+=`, `-=`, `*=`, `/=`, `%=`.
- **Incremento y Decremento:** `++` (aumenta en 1), `--` (disminuye en 1). Pueden ser prefijos (`++i`) o sufijos (`i++`).
- **Ternario (`? :`):** Es una forma abreviada de `if-else`. `(condición) ? expresión1 : expresión2`.
- **De acceso a miembro:** `.` (para estructuras y uniones), `->` (para punteros a estructuras).
- **Bitwise (o a nivel de bits):** `&` (AND), `|` (OR), `^` (XOR), `~` (NOT), `<<` (desplazamiento izquierda), `>>` (desplazamiento derecha).

### 2.5. Comentarios
- **Comentarios de bloque:** Pueden ocupar varias líneas y se encierran entre `/*` y `*/`.
  ```c
  /* Este es un comentario
     de múltiples líneas. */
  ```
- **Comentarios de línea:** Comienzan con `//` y terminan al final de la línea (disponible desde el estándar C99).
  ```c
  int x = 5; // Esto es un comentario de una línea.
  ```

---

## 3. Control de Flujo

### 3.1. Sentencias Condicionales: `if`, `else if`, `else`
Permiten ejecutar bloques de código dependiendo de si una condición es verdadera o falsa .

```c
#include <stdio.h>

int main() {
    int nota = 85;

    if (nota >= 90) {
        printf("Sobresaliente\n");
    } else if (nota >= 80) {
        printf("Notable\n");
    } else if (nota >= 70) {
        printf("Aprobado\n");
    } else {
        printf("Reprobado\n");
    }
    return 0;
}
```
**Salida esperada:**
```
Notable
```

### 3.2. La Sentencia `switch`
Es útil para tomar decisiones basadas en el valor exacto de una expresión (generalmente de tipo `int` o `char`) .

```c
#include <stdio.h>

int main() {
    char operacion = '+';
    int a = 10, b = 5;

    switch (operacion) {
        case '+':
            printf("Resultado: %d\n", a + b);
            break;  // break es crucial para salir del switch
        case '-':
            printf("Resultado: %d\n", a - b);
            break;
        case '*':
            printf("Resultado: %d\n", a * b);
            break;
        default: // Si no coincide con ningún case
            printf("Operación no válida\n");
    }
    return 0;
}
```
**Salida esperada:**
```
Resultado: 15
```
*Nota: Olvidar el `break` provocará que la ejecución continúe con el siguiente `case` ("fall-through").*

### 3.3. Bucles: `while`, `do-while`, `for`
- **`while`:** Evalúa la condición al principio. Si es falsa, el bucle no se ejecuta .
  ```c
  int i = 0;
  while (i < 5) {
      printf("%d ", i);
      i++;
  }
  ```
- **`do-while`:** Evalúa la condición al final. El bucle se ejecuta al menos una vez.
  ```c
  int j = 0;
  do {
      printf("%d ", j);
      j++;
  } while (j < 5);
  ```
- **`for`:** Ideal cuando se conoce el número de iteraciones. Agrupa inicialización, condición y actualización .
  ```c
  // Imprime los números pares del 0 al 10
  for (int k = 0; k <= 10; k += 2) {
      printf("%d ", k);
  }
  ```

### 3.4. Sentencias de Salto: `break`, `continue`, `goto`
- **`break`:** Sale inmediatamente del bucle o `switch` más interno en el que se encuentra.
- **`continue`:** Salta a la siguiente iteración del bucle (ignora el resto del código en la iteración actual).
- **`goto`:** Transfiere el control a una etiqueta definida. Su uso está generalmente desaconsejado, ya que puede hacer que el código sea difícil de leer y mantener, pero puede ser útil para salir de bucles anidados en casos excepcionales .

---

## 4. Funciones

Las funciones son bloques de código reutilizables que realizan una tarea específica. Ayudan a organizar el programa y evitar la repetición de código .

### 4.1. Definición y Declaración de Funciones
- **Definición:** Es el cuerpo de la función, donde se escribe el código que realiza la tarea.
- **Declaración (o Prototipo):** Es una línea que indica el nombre de la función, el tipo de dato que devuelve y los tipos de sus parámetros. Se coloca antes de la función `main` para que el compilador sepa que la función existe antes de usarla.

### 4.2. Parámetros y Argumentos
- **Parámetros:** Son las variables que se declaran en la definición de la función para recibir datos.
- **Argumentos:** Son los valores que se pasan a la función cuando se llama.

### 4.3. Valor de Retorno
La función puede devolver un valor usando la sentencia `return`. El tipo de este valor debe coincidir con el tipo declarado en la función. Si la función no devuelve nada, se declara como `void`.

### 4.4. Prototipos de Funciones y Archivos de Cabecera
Un prototipo es la declaración de la función antes de su uso.
```c
#include <stdio.h>

// Prototipos de funciones (declaraciones)
int sumar(int a, int b);
void saludar();

int main() {
    int resultado = sumar(5, 3);
    printf("La suma es: %d\n", resultado);
    saludar();
    return 0;
}

// Definiciones de funciones
int sumar(int a, int b) { // a y b son parámetros
    return a + b;
}

void saludar() {
    printf("¡Hola desde una función!\n");
}
```
**Salida esperada:**
```
La suma es: 8
¡Hola desde una función!
```

### 4.5. Recursividad
Una función puede llamarse a sí misma. Es una técnica poderosa para resolver problemas que se pueden descomponer en subproblemas más pequeños (ej. series de Fibonacci, factoriales, búsqueda binaria) .

```c
#include <stdio.h>

// Función recursiva para calcular el factorial
int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1; // Caso base
    } else {
        return n * factorial(n - 1); // Llamada recursiva
    }
}

int main() {
    int num = 5;
    printf("El factorial de %d es %d\n", num, factorial(num));
    return 0;
}
```
**Salida esperada:**
```
El factorial de 5 es 120
```

---

## 5. Arreglos y Cadenas

### 5.1. Arreglos Unidimensionales
Un arreglo es una colección de elementos del mismo tipo de datos, almacenados en posiciones contiguas de memoria .

```c
#include <stdio.h>

int main() {
    int numeros[5]; // Declara un arreglo de 5 enteros
    int otros[3] = {10, 20, 30}; // Declara e inicializa

    numeros[0] = 100; // Asigna un valor al primer elemento (índice 0)
    numeros[4] = 500; // Asigna al quinto elemento (índice 4)

    for (int i = 0; i < 5; i++) {
        // Imprime todos los elementos del arreglo 'numeros'
        printf("numeros[%d] = %d\n", i, numeros[i]);
    }

    return 0;
}
```
**Salida esperada:**
```
numeros[0] = 100
numeros[1] = 0
numeros[2] = 0
numeros[3] = 0
numeros[4] = 500
```

### 5.2. Cadenas de Caracteres (`char[]`)
En C, una cadena es un arreglo de caracteres (`char`) que termina con el carácter nulo `'\0'` .

```c
#include <stdio.h>
#include <string.h> // Contiene funciones para manejar strings

int main() {
    char saludo[] = "Hola"; // El compilador añade '\0' al final
    char nombre[20];

    printf("Tu nombre: ");
    scanf("%s", nombre); // Lee una cadena, peligroso por desbordamiento de buffer

    // strcat concatena 'nombre' a 'saludo'
    strcat(saludo, " ");
    strcat(saludo, nombre);
    printf("%s\n", saludo);

    return 0;
}
```
*Entrada: `"Carlos"`*
**Salida esperada:**
```
Tu nombre: Carlos
Hola Carlos
```

### 5.3. Arreglos Multidimensionales
Son arreglos de arreglos, como las matrices .

```c
#include <stdio.h>

int main() {
    // Declaración e inicialización de una matriz 2x3
    int matriz[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
    return 0;
}
```
**Salida esperada:**
```
1 2 3
4 5 6
```

---

## 6. Punteros

### 6.1. ¿Qué es un Puntero?
Un puntero es una variable que almacena la **dirección de memoria** de otra variable, en lugar de almacenar un valor directamente .

### 6.2. Operadores de Dirección (`&`) e Indirección (`*`)
- **`&` (Operador de Dirección):** Obtiene la dirección de memoria de una variable.
- **`*` (Operador de Indirección o Desreferencia):** Accede al valor almacenado en la dirección de memoria que contiene el puntero.

### 6.3. Punteros y Arreglos
El nombre de un arreglo actúa como un puntero a su primer elemento. `arr` es equivalente a `&arr[0]`.

```c
#include <stdio.h>

int main() {
    int arr[] = {10, 20, 30, 40, 50};
    int *ptr = arr; // ptr apunta al primer elemento de arr

    printf("Valor en arr[0] usando el puntero: %d\n", *ptr);
    // Se puede usar aritmética de punteros para acceder a otros elementos
    printf("Valor en arr[2] usando el puntero: %d\n", *(ptr + 2));

    // Las siguientes son formas equivalentes de acceder a arr[3]
    printf("arr[3] = %d\n", arr[3]);
    printf("*(arr + 3) = %d\n", *(arr + 3));
    printf("ptr[3] = %d\n", ptr[3]);

    return 0;
}
```
**Salida esperada:**
```
Valor en arr[0] usando el puntero: 10
Valor en arr[2] usando el puntero: 30
arr[3] = 40
*(arr + 3) = 40
ptr[3] = 40
```

### 6.4. Aritmética de Punteros
Puedes sumar o restar enteros a un puntero para moverlo a través de la memoria. El incremento (`ptr++`) mueve el puntero al siguiente elemento de su tipo, sumando el tamaño del tipo en bytes al puntero .

### 6.5. Punteros y Funciones
- **Paso por referencia:** Al pasar un puntero a una función, ésta puede modificar el valor de la variable original en la función llamante.
- **Punteros a funciones:** Permiten pasar funciones como argumentos a otras funciones.

```c
#include <stdio.h>

// Función que modifica el valor de 'x' usando un puntero
void sumarUno(int *x) {
    *x = *x + 1;
}

int main() {
    int num = 5;
    printf("Antes: %d\n", num);
    sumarUno(&num); // Pasamos la dirección de num
    printf("Después: %d\n", num);
    return 0;
}
```
**Salida esperada:**
```
Antes: 5
Después: 6
```

### 6.6. Punteros a Punteros
Son variables que almacenan la dirección de memoria de otro puntero. Útiles para manejar matrices dinámicas o modificar un puntero dentro de una función.

---

## 7. Gestión de Memoria Dinámica

La memoria dinámica se asigna en tiempo de ejecución desde el **heap** (montón) utilizando funciones de la librería `stdlib.h` .

### 7.1. `malloc()`, `calloc()`, `realloc()` y `free()`
- **`malloc(size_t size)`:** Reserva `size` bytes de memoria y devuelve un puntero `void*` al inicio del bloque. No inicializa la memoria.
- **`calloc(size_t num, size_t size)`:** Reserva memoria para `num` elementos de `size` bytes cada uno y **inicializa todos los bytes a cero**.
- **`realloc(void *ptr, size_t new_size)`:** Cambia el tamaño del bloque de memoria al que apunta `ptr` al nuevo tamaño `new_size`. Puede mover el bloque a una nueva ubicación.
- **`free(void *ptr)`:** Libera el bloque de memoria al que apunta `ptr`, devolviéndolo al sistema.

```c
#include <stdio.h>
#include <stdlib.h> // Necesario para malloc, calloc, free

int main() {
    int *arr;
    int n = 5;

    // Asignar memoria para un arreglo de 'n' enteros
    arr = (int*) malloc(n * sizeof(int));

    // Siempre verificar si malloc falló (devuelve NULL)
    if (arr == NULL) {
        printf("Error: No se pudo asignar memoria.\n");
        return 1;
    }

    // Usar la memoria
    for (int i = 0; i < n; i++) {
        arr[i] = i * 10;
    }

    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    // Es crucial liberar la memoria cuando ya no se necesita
    free(arr);

    return 0;
}
```
**Salida esperada:**
```
0 10 20 30 40
```

### 7.2. Errores Comunes (Memory Leaks, Dangling Pointers)
- **Memory Leak (Fuga de memoria):** Ocurre cuando se pierde el puntero al bloque de memoria asignado sin haberlo liberado con `free()`.
- **Dangling Pointer (Puntero colgante):** Ocurre cuando un puntero apunta a una dirección de memoria que ya ha sido liberada. Acceder a él puede causar errores de segmentación o comportamiento indefinido.

---

## 8. Estructuras y Uniones

### 8.1. Definición de Estructuras (`struct`)
Una estructura es un tipo de dato definido por el usuario que agrupa variables de diferentes tipos bajo un mismo nombre .

```c
#include <stdio.h>
#include <string.h>

// Definición de la estructura 'Persona'
struct Persona {
    char nombre[50];
    int edad;
    float altura;
};

int main() {
    // Declaración de una variable de tipo struct Persona
    struct Persona persona1;

    // Asignación de valores a los miembros
    strcpy(persona1.nombre, "Ana");
    persona1.edad = 30;
    persona1.altura = 1.68;

    printf("Nombre: %s\n", persona1.nombre);
    printf("Edad: %d\n", persona1.edad);
    printf("Altura: %.2f\n", persona1.altura);

    // Inicialización directa
    struct Persona persona2 = {"Luis", 25, 1.75};
    printf("\nPersona 2: %s, %d años, %.2f m\n", persona2.nombre, persona2.edad, persona2.altura);

    return 0;
}
```
**Salida esperada:**
```
Nombre: Ana
Edad: 30
Altura: 1.68

Persona 2: Luis, 25 años, 1.75 m
```

### 8.2. Acceso a Miembros (`.` y `->`)
- **Operador `.` (punto):** Se usa para acceder a los miembros de una variable de tipo estructura (o unión).
- **Operador `->` (flecha):** Es un atajo para acceder a los miembros a través de un puntero a una estructura. `ptr->miembro` es equivalente a `(*ptr).miembro`.

### 8.3. Estructuras y Funciones
Las estructuras se pueden pasar a funciones por valor (se copia toda la estructura) o por referencia (se pasa un puntero a la estructura, lo cual es más eficiente para estructuras grandes y permite modificar la original).

```c
#include <stdio.h>
#include <string.h>

struct Persona { char nombre[50]; int edad; };

// Pasar por referencia (con puntero)
void cumpleanios(struct Persona *p) {
    p->edad++; // Equivalente a (*p).edad++
}

int main() {
    struct Persona p = {"Carlos", 29};
    printf("Antes: %d\n", p.edad);
    cumpleanios(&p);
    printf("Después: %d\n", p.edad);
    return 0;
}
```
**Salida esperada:**
```
Antes: 29
Después: 30
```

### 8.4. `typedef` para Alias de Tipos
`typedef` se usa para crear alias para tipos de datos, mejorando la legibilidad del código.

```c
#include <stdio.h>

// Crear un alias 'Entero' para el tipo 'int'
typedef int Entero;

// Crear un alias 'Persona' para 'struct Persona'
typedef struct Persona {
    char nombre[50];
    int edad;
} Persona; // Aquí 'Persona' ya es el alias

int main() {
    Entero x = 10; // Ahora 'Entero' es sinónimo de 'int'
    Persona p1 = {"María", 22}; // Usamos el alias sin necesidad de 'struct'
    printf("Edad de %s: %d\n", p1.nombre, p1.edad);
    return 0;
}
```

### 8.5. Uniones (`union`)
Una unión es similar a una estructura, pero **todos sus miembros comparten la misma ubicación de memoria**. El tamaño de la unión es el tamaño de su miembro más grande. Solo se puede almacenar un valor a la vez en ella .

```c
#include <stdio.h>
#include <string.h>

union Dato {
    int i;
    float f;
    char str[20];
};

int main() {
    union Dato dato;

    dato.i = 10;
    printf("dato.i: %d\n", dato.i);

    dato.f = 3.14;
    printf("dato.f: %f\n", dato.f);
    // Al asignar f, el valor de dato.i se corrompe
    printf("dato.i después de cambiar a f: %d\n", dato.i);
    return 0;
}
```

---

## 9. Entrada y Salida (I/O)

### 9.1. Entrada y Salida Estándar: `printf()` y `scanf()`
- **`printf()`:** Se usa para escribir texto formateado en la salida estándar (generalmente la pantalla) .
  - *Especificadores de formato:* `%d` (int), `%f` (float/double), `%c` (char), `%s` (string), `%p` (puntero).
- **`scanf()`:** Se usa para leer texto formateado desde la entrada estándar (generalmente el teclado). **Es una función peligrosa** si no se controla el tamaño del buffer de entrada, ya que puede causar desbordamientos de buffer.

```c
#include <stdio.h>

int main() {
    int edad;
    char nombre[30];

    printf("Ingresa tu nombre: ");
    // %s es para strings, pero no verifica el tamaño del buffer
    scanf("%29s", nombre); // Se recomienda limitar la longitud a 29 caracteres

    printf("Ingresa tu edad: ");
    scanf("%d", &edad); // Usar & para pasar la dirección de la variable

    printf("Nombre: %s, Edad: %d\n", nombre, edad);
    return 0;
}
```

### 9.2. Entrada y Salida por Caracteres: `getchar()`, `putchar()`
- **`getchar()`:** Lee un solo carácter de la entrada estándar.
- **`putchar()`:** Escribe un solo carácter en la salida estándar .

### 9.3. Entrada y Salida de Líneas: `fgets()`, `puts()`
- **`fgets()`:** Lee una línea completa (incluyendo espacios en blanco) de un flujo de datos, respetando un límite de tamaño. **Es más segura que `scanf()`**.
- **`puts()`:** Escribe una cadena en la salida estándar y añade automáticamente un salto de línea.

---

## 10. Manejo de Archivos

### 10.1. Conceptos Básicos: El Tipo `FILE`
`FILE` es un tipo de datos definido en `stdio.h` que es utilizado para representar un flujo de archivo. Usamos un puntero a `FILE` para acceder a él.

### 10.2. Abrir y Cerrar Archivos: `fopen()`, `fclose()`
- **`fopen(const char *filename, const char *mode)`:** Abre un archivo.
  - *Modos:* `"r"` (lectura), `"w"` (escritura, sobreescribe), `"a"` (append, añade al final), `"r+"` (lectura/escritura), `"w+"` (lectura/escritura, sobreescribe), etc.
- **`fclose(FILE *stream)`:** Cierra un archivo abierto. Es importante cerrar los archivos para guardar los cambios y liberar recursos.

### 10.3. Lectura y Escritura: `fprintf()`, `fscanf()`, `fgetc()`, `fputc()`, `fread()`, `fwrite()`
- **`fprintf(FILE *stream, const char *format, ...)`:** Escribe formateado en un archivo.
- **`fscanf(FILE *stream, const char *format, ...)`:** Lee formateado desde un archivo.
- **`fgetc(FILE *stream)`:** Lee un carácter de un archivo.
- **`fputc(int char, FILE *stream)`:** Escribe un carácter en un archivo.
- **`fread(void *ptr, size_t size, size_t nmemb, FILE *stream)`:** Lee datos binarios de un archivo.
- **`fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)`:** Escribe datos binarios en un archivo.

```c
#include <stdio.h>

int main() {
    FILE *archivo;
    char texto[] = "Este es un texto de ejemplo.";

    // Abrir el archivo en modo escritura ("w")
    archivo = fopen("ejemplo.txt", "w");
    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return 1;
    }

    // Escribir en el archivo
    fprintf(archivo, "%s\n", texto);
    fclose(archivo); // Cerrar el archivo

    // Abrir el archivo en modo lectura ("r")
    archivo = fopen("ejemplo.txt", "r");
    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return 1;
    }

    // Leer y mostrar el contenido
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        printf("%s", buffer);
    }

    fclose(archivo);
    return 0;
}
```
**Salida esperada:**
```
Este es un texto de ejemplo.
```

### 10.4. Posicionamiento en Archivos: `fseek()`, `ftell()`
- **`ftell(FILE *stream)`:** Devuelve la posición actual del puntero de archivo (en bytes desde el inicio).
- **`fseek(FILE *stream, long offset, int whence)`:** Mueve el puntero de archivo a una nueva posición.
  - `offset`: Número de bytes a desplazar.
  - `whence`: Punto de referencia (`SEEK_SET` inicio, `SEEK_CUR` posición actual, `SEEK_END` final).

---

## 11. Referencia de la Librería Estándar

La potencia de C reside en su librería estándar, que proporciona un conjunto de funciones para tareas comunes .

### 11.1. Matemáticas (`math.h`)
Requiere enlazar con la librería matemática durante la compilación: `gcc programa.c -o programa -lm`.

| Función                          | Descripción                                 | Ejemplo                        |
| :------------------------------- | :------------------------------------------ | :----------------------------- |
| `double sqrt(double x)`          | Raíz cuadrada de `x`                        | `sqrt(9.0)` devuelve `3.0`     |
| `double pow(double x, double y)` | `x` elevado a la `y`                        | `pow(2.0, 3.0)` devuelve `8.0` |
| `double sin(double x)`           | Seno de `x` (en radianes)                   | `sin(0.0)` devuelve `0.0`      |
| `double cos(double x)`           | Coseno de `x` (en radianes)                 | `cos(0.0)` devuelve `1.0`      |
| `double tan(double x)`           | Tangente de `x` (en radianes)               | `tan(1.0)`                     |
| `double exp(double x)`           | Exponencial (`e^x`)                         | `exp(1.0)` devuelve `2.718...` |
| `double log(double x)`           | Logaritmo natural de `x`                    | `log(2.718...)` devuelve `1.0` |
| `double fabs(double x)`          | Valor absoluto de `x`                       | `fabs(-5.5)` devuelve `5.5`    |
| `double floor(double x)`         | Redondea hacia abajo al entero más cercano  | `floor(3.9)` devuelve `3.0`    |
| `double ceil(double x)`          | Redondea hacia arriba al entero más cercano | `ceil(3.1)` devuelve `4.0`     |

```c
#include <stdio.h>
#include <math.h>

int main() {
    double angulo = 30.0; // en grados
    double radianes = angulo * M_PI / 180.0; // M_PI viene en math.h
    printf("seno(30°) = %f\n", sin(radianes));
    printf("sqrt(144) = %f\n", sqrt(144.0));
    return 0;
}
```

### 11.2. Manejo de Cadenas (`string.h`)
Provee funciones para manipular cadenas de caracteres.

| Función                                        | Descripción                                        |
| :--------------------------------------------- | :------------------------------------------------- |
| `size_t strlen(const char *s)`                 | Devuelve la longitud de `s` (sin contar el `'\0'`) |
| `char *strcpy(char *dest, const char *src)`    | Copia `src` a `dest`                               |
| `char *strcat(char *dest, const char *src)`    | Concatena `src` al final de `dest`                 |
| `int strcmp(const char *s1, const char *s2)`   | Compara `s1` y `s2` lexicográficamente             |
| `char *strchr(const char *s, int c)`           | Encuentra la primera ocurrencia de `c` en `s`      |
| `char *strstr(const char *s, const char *sub)` | Encuentra la primera ocurrencia de `sub` en `s`    |

```c
#include <stdio.h>
#include <string.h>

int main() {
    char src[] = "Mundo";
    char dest[20] = "Hola ";

    printf("Longitud de src: %lu\n", strlen(src));
    strcat(dest, src);
    printf("dest: %s\n", dest);

    if (strcmp("hola", "HOLA") != 0) {
        printf("Las cadenas son diferentes.\n");
    }
    return 0;
}
```

### 11.3. Utilidades Generales (`stdlib.h`)
Contiene funciones para conversiones, gestión de memoria, números aleatorios, etc.

| Función                                 | Descripción                                            |
| :-------------------------------------- | :----------------------------------------------------- |
| `int atoi(const char *str)`             | Convierte una cadena a un entero `int`                 |
| `double atof(const char *str)`          | Convierte una cadena a un `double`                     |
| `void *malloc(size_t size)`             | Asigna memoria dinámica                                |
| `void free(void *ptr)`                  | Libera memoria dinámica                                |
| `void *calloc(size_t num, size_t size)` | Asigna e inicializa a cero un arreglo                  |
| `void *realloc(void *ptr, size_t size)` | Redimensiona un bloque de memoria                      |
| `int rand(void)`                        | Genera un número pseudo-aleatorio entre 0 y `RAND_MAX` |
| `void srand(unsigned int seed)`         | Establece la semilla para `rand()`                     |
| `int system(const char *command)`       | Ejecuta un comando del sistema operativo               |

### 11.4. Verificación de Caracteres (`ctype.h`)
Provee funciones para clasificar y convertir caracteres.

| Función              | Descripción                                      |
| :------------------- | :----------------------------------------------- |
| `int isalpha(int c)` | Verifica si `c` es una letra                     |
| `int isdigit(int c)` | Verifica si `c` es un dígito                     |
| `int isalnum(int c)` | Verifica si `c` es alfanumérico (letra o dígito) |
| `int isspace(int c)` | Verifica si `c` es un espacio en blanco          |
| `int islower(int c)` | Verifica si `c` es una letra minúscula           |
| `int isupper(int c)` | Verifica si `c` es una letra mayúscula           |
| `int tolower(int c)` | Convierte `c` a minúscula si es mayúscula        |
| `int toupper(int c)` | Convierte `c` a mayúscula si es minúscula        |

### 11.5. Manejo de Archivos (`stdio.h`)
Además de `printf` y `scanf`, contiene funciones para I/O de archivos.

| Función                                               | Descripción                                          |
| :---------------------------------------------------- | :--------------------------------------------------- |
| `FILE *fopen(...)`                                    | Abre un archivo                                      |
| `int fclose(FILE *)`                                  | Cierra un archivo                                    |
| `int fprintf(FILE *, ...)`                            | Escribe formateado a un archivo                      |
| `int fscanf(FILE *, ...)`                             | Lee formateado desde un archivo                      |
| `char *fgets(char *, int, FILE *)`                    | Lee una línea de un archivo                          |
| `int fputs(const char *, FILE *)`                     | Escribe una cadena a un archivo                      |
| `int fgetc(FILE *)`                                   | Lee un carácter de un archivo                        |
| `int fputc(int, FILE *)`                              | Escribe un carácter a un archivo                     |
| `size_t fread(void *, size_t, size_t, FILE *)`        | Lee datos binarios de un archivo                     |
| `size_t fwrite(const void *, size_t, size_t, FILE *)` | Escribe datos binarios a un archivo                  |
| `int fseek(FILE *, long, int)`                        | Cambia la posición del puntero en el archivo         |
| `long ftell(FILE *)`                                  | Obtiene la posición actual del puntero en el archivo |
| `void rewind(FILE *)`                                 | Vuelve al inicio del archivo                         |

---

## Ejercicios

**Ejercicio 1.1**

Escriba un programa en C, llamado *ejercicio1_1.c*, que reciba una secuencia de medidas dadas en pies y pulgadas, y las convierta a metros (un número negativo representa el final de la secuencia). Recuerde que 1 pulgada = 2.55 cm y 1 pie = 12 pulgadas. Por ejemplo, si la entrada es:

```
10 2
6 7
4 5
-1
```

la salida debería ser:

```
3.1
2.01
1.35
```

**Ejercicio 1.2**

Escriba un programa en C, llamado *ejercicio1_2.c*, que reciba un monto de dinero (en miles de colones) y luego imprima la cantidad mínima de billetes con que se puede pagar dicho monto. Por ejemplo, si la entrada es:

```
18000
```

la salida debería ser:

```
un billete de 10000
un billete de 5000
un billete de 2000
un billete de 1000
```

**Ejercicio 1.3**

Escriba un programa en C, llamado *ejercicio1_3.c*, que reciba las longitudes de los dos catetos de un triángulo rectángulo; e imprima la longitud de su hipotenusa. Por ejemplo, si la entrada es:

```
8 13
```

la salida sería:

```
15.26
```

**Ejercicio 1.4**

Escriba un programa en C, llamado *ejercicio1_4.c*, que reciba un número de placa e imprima los días que le toca la restricción vehicular. LUNES: 1 y 2, MARTES: 3 y 4, MIERCOLES: 5 y 6, JUEVES: 7 y 8, VIERNES: 9 y 0, SABADOS: pares, DOMINGOS: impares. Por ejemplo, si la entrada es:

```
756243
```

la salida sería:

```
MARTES DOMINGO
```

**Ejercicio 1.5**

Escriba un programa en C, llamado *ejercicio1_5.c*, que reciba la edad de una persona e imprima en que etapa del desarrollo humano se encuentra dicha persona: infancia (menor de 6 años), niñez (menor de 12 años), adolescencia (menor de 18 años), juventud (menor de 25 años), adultez (menor de 60 años), o vejez. Por ejemplo, si la entrada es:

```
28
```

la salida sería:

```
adultez
```

**Ejercicio 1.6**

Escriba un programa en C, llamado *ejercicio1_6.c*, que reciba la fecha de nacimiento de una persona (día y mes), imprima el signo del zodiaco al cuál pertenece: aries (21/03 al 20/04), tauro (21/04 al 21/05), géminis (22/05 al 21/06), cáncer (21/06 al 23/07), leo (24/07 al 23/08), virgo (24/08 al 23/09), libra (24/09 al 23/10), escorpión (24/10 al 22/11), sagitario (23/11 al 21/12), capricornio (22/12 al 20/01), acuario (21/01 al 19/02), piscis (20/02 al 20/03). Por ejemplo, si la entrada es:

```
19 02
```

la salida sería:

```
acuario
```

**Ejercicio 1.7**

Escriba un programa en C, llamado *ejercicio1_7.c*, que lea una secuencia de valores enteros (un número negativo representa el final de la secuencia). El programa luego debe imprimir el valor promedio, el mínimo y el máximo de todos los valores ingresados (no use arreglos ni otra estructura de datos). Por ejemplo, si la entrada es:

```
10 20 30 40 -1
```

la salida sería:

```
25
10
40
```

**Ejercicio 1.8**

Escriba un programa en C, llamado *ejercicio1_8.c*, que reciba tres valores X, Y, Z y luego imprima dichos valores en orden creciente (no utilice arreglos ni ningún otro tipo de estructuras de datos). Por ejemplo, si la entrada es:

```
34 8 12
```

la salida debería ser:

```
8 12 34
```

**Ejercicio 1.9**

Escriba un programa en C, llamado *ejercicio1_9.c*, que recibe un valor N e imprima todos los números menores que N que son múltiplos de 3 pero no son múltiplos de 6. Por ejemplo, si la entrada es:

```
40
```

la salida sería:

```
3  9  15  21  27  33 39
```

**Ejercicio 1.10**

Escriba un programa en C, llamado *ejercicio1_10.c*, que reciba una secuencia de números enteros (un número negativo representa el final de la secuencia) e imprima sus logaritmos enteros en base 2 (no utilice la biblioteca cmath). El logaritmo entero (base 2) de un entero es el número de veces que el entero se puede dividir (de forma entera) por 2 hasta que el resultado sea 1. Por ejemplo, si la entrada es:

```
24
12
18
-1
```

la salida sería:

```
4
3
4
```
