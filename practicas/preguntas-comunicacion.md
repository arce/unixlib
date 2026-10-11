# Preguntas de Laboratorio: Comunicación Interprocesos

Las siguientes 10 preguntas están pensadas para un **laboratorio individual de 2 horas**: a cada estudiante se le asignará **una única pregunta**, la cual deberá resolver por su cuenta durante la sesión.

Todas las preguntas deben resolverse con **un único programa** en el que **un proceso padre crea los procesos hijos con los que se comunica** (el padre hace *fork()* directamente de todos sus hijos; no debe haber hijos que a su vez hagan *fork()* de otros procesos). Según la pregunta asignada, se debe utilizar memoria compartida (*shmget*, *shmat*, *shmdt*, *shmctl*) o colas de mensajes (*msgget*, *msgsnd*, *msgrcv*, *msgctl*), tal como se explicó en la sección de Comunicación Interprocesos.

Recuerde imprimir resultados claros (incluyendo el PID del proceso cuando sea relevante) y liberar/eliminar correctamente los recursos de IPC utilizados (memoria compartida, semáforos o colas de mensajes) antes de que el programa termine.

## Pregunta 1

Cree un proceso padre que cree 10 procesos hijos. Utilizando un arreglo en memoria compartida, el padre almacenará un número aleatorio entre 1 y 100 para cada hijo (puede usar *r = rand() % n;* para generar un número aleatorio entre 0 y n). Cada hijo deberá leer únicamente el número que le corresponde y determinar si es un número primo, imprimiendo el resultado (por ejemplo: "El proceso [PID] recibió el número 17, que sí es primo").

## Pregunta 2

Cree un proceso padre que cree 5 procesos hijos. Utilizando un arreglo en memoria compartida, el padre le asignará a cada hijo un radio aleatorio entre 1 y 20. Cada hijo deberá calcular el área del círculo correspondiente a su radio (área = π × r²) e imprimir el resultado junto con su identificador de proceso.

## Pregunta 3

Cree un proceso padre que cree 4 procesos hijos. Los cuatro hijos incrementarán, cada uno 500 veces, un contador compartido almacenado en memoria compartida, utilizando un semáforo para evitar condiciones de carrera (de forma similar a como se explicó en la sección de sincronización con memoria compartida). Al finalizar todos los hijos, el padre deberá imprimir el valor final del contador y verificar si coincide con el valor esperado (2000).

## Pregunta 4

Cree un proceso padre que cree 7 procesos hijos. Utilizando un arreglo en memoria compartida, el padre le asignará a cada hijo la edad de una persona (número aleatorio entre 1 y 90). Cada hijo deberá clasificar la edad que le corresponde en una de las categorías "niño" (0-12), "adolescente" (13-17), "adulto" (18-64) o "adulto mayor" (65 o más) e imprimir el resultado.

## Pregunta 5

Cree un proceso padre que cree 4 procesos hijos. El padre almacenará en memoria compartida una matriz de 4x4 con números aleatorios entre 1 y 20 (puede representarla como un arreglo de 16 posiciones). A cada hijo se le asignará una fila distinta de la matriz; cada hijo deberá calcular la suma de los cuatro números de su fila y guardar el resultado en un segundo arreglo en memoria compartida. El padre deberá esperar a que todos los hijos terminen y luego mostrará la suma correspondiente a cada fila.

## Pregunta 6

Cree un proceso padre que cree 3 procesos hijos y les envíe, utilizando una misma cola de mensajes, 4 mensajes a cada uno (use el campo *type* del mensaje para dirigir cada mensaje al hijo correspondiente). Cada mensaje contendrá una nota de examen (número entre 0 y 100). Cada hijo deberá recibir sus 4 notas, calcular el promedio e imprimirlo.

## Pregunta 7

Cree un proceso padre que cree un proceso hijo y se comuniquen bidireccionalmente usando dos colas de mensajes. El padre enviará 6 números aleatorios al hijo; el hijo deberá determinar si cada número es par o impar y devolver al padre dicha clasificación (por ejemplo, el texto "par" o "impar"). El padre deberá mostrar cada número original junto con la clasificación recibida.

## Pregunta 8

Cree un proceso padre que cree 3 procesos hijos, comunicados en cadena mediante distintas colas de mensajes. El padre enviará, a través de la primera cola, el texto "La sincronización evita condiciones de carrera" al primer hijo. El primer hijo eliminará todos los espacios del texto y lo enviará al segundo hijo mediante una segunda cola. El segundo hijo contará la cantidad de caracteres del texto recibido y enviará, mediante una tercera cola, tanto el texto sin espacios como su longitud al tercer hijo, quien deberá imprimir ambos datos.

## Pregunta 9

Cree un proceso padre que cree 3 procesos hijos, comunicados en cadena mediante distintas colas de mensajes. El padre enviará, a través de la primera cola, el texto "Un proceso zombie no libera sus recursos" al primer hijo. El primer hijo reemplazará cada vocal "o" por el carácter "0" y enviará el resultado al segundo hijo mediante una segunda cola. El segundo hijo invertirá el texto completo (de derecha a izquierda) y lo enviará al tercer hijo mediante una tercera cola, quien mostrará el resultado final.

## Pregunta 10

Cree un proceso padre que cree 4 procesos hijos. Utilizando una única cola de mensajes y el campo *type* para dirigir los mensajes, cada hijo enviará al padre una solicitud que contenga su identificador y un número aleatorio entre 1 y 100. El padre, tras recibir las 4 solicitudes, determinará cuál hijo envió el número mayor y responderá a cada hijo (dirigiendo la respuesta mediante *type*) indicándole si su número fue el mayor recibido o no. Cada hijo deberá imprimir si su número resultó ser el mayor entre los cuatro o no.
