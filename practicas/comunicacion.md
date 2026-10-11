# Comunicación Interprocesos

> **Nota para Windows:** los ejemplos de este tutorial usan llamadas Unix (`fork`, `shmget`, `semget`, `msgget`). Puede ejecutarlos en Windows con TDM-GCC/MinGW-w64 usando la biblioteca **unixlib**: <https://github.com/arce/unixlib>. Basta compilar con `gcc -I<ruta>/include -I<ruta>/compat programa.c -o programa.exe`, sin modificar el código.

## Introducción a la Comunicación Interprocesos

La comunicación interprocesos (IPC) es un mecanismo fundamental en los sistemas operativos que permite que los procesos se comuniquen y sincronicen entre sí. Existen diversos mecanismos de IPC, cada uno con sus propias características, ventajas y casos de uso específicos. Los principales mecanismos de IPC en Unix incluyen:

- **Memoria Compartida**: Permite que múltiples procesos accedan a la misma región de memoria
- **Colas de Mensajes**: Permite el intercambio de mensajes estructurados entre procesos
- **Tuberías (Pipes)**: Comunicación unidireccional entre procesos relacionados
- **Señales**: Notificaciones asíncronas entre procesos
- **Sockets**: Comunicación entre procesos en diferentes máquinas

En esta sección nos enfocaremos en la memoria compartida y las colas de mensajes, dos de los mecanismos más utilizados en la programación de sistemas.

## Memoria Compartida

Uno de los mecanismos más simples de comunicación interprocesos es la memoria compartida. La memoria compartida permite que dos o más procesos accedan a la misma memoria. Cuando un proceso cambia la memoria todos los otros procesos ven las modificaciones. Existen dos grupos de funciones para manipular memoria compartida: System V y POSIX. A continuación se muestra la forma de comunicar procesos utilizando las funciones de System V.

La memoria compartida es el mecanismo de IPC más rápido, ya que los datos no necesitan ser copiados entre el espacio del kernel y el espacio del usuario. Sin embargo, requiere sincronización explícita entre los procesos para evitar condiciones de carrera.

### Escritura y lectura en memoria

La función *shmget(shm_key,getpagesize(),IPC_CREAT | S_IRUSR | S_IWUSR)* crea un nuevo segmento de memoria compartida, ó bien, accede a uno existente si *shm_key* ya ha sido usada. Si la llamada es exitosa, *shmget* retorna un identificador de segmento (*shm_id*). Para hacer que el segmento esté disponible, el proceso debe invocar a la función *shmat(shm_id,NULL,0)*. Esta misma función se utiliza para desvincular la memoria del espacio de direcciones de proceso, pero utilizando *shmdt(shm)*.

La memoria compartida se puede ver como un conjunto simple de bytes y escribir de la misma forma como se haría con otros flujos de bytes mediante la función *sprintf()*, de igual forma la lectura se puede realizar mediante la función *sscanf()*.

Para eliminar el área de memoria compartida se utiliza la función *shmctl(shm_id,IPC_RMID,NULL)*.

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  int i,status;
  double d;
  char s[256];
  key_t shm_key = 999;

  int shm_id = shmget(shm_key, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);
  char *shm = (char *)shmat(shm_id, NULL, 0);

  // El padre escribe antes de hacer fork(). Así el hijo, que hereda el
  // segmento ya adjuntado, siempre encuentra el valor completo sin
  // necesitar sincronización adicional.
  sprintf(shm, "%i %lf %s", 10, 20.5, "Hello World\n");

  pid_t pid = fork();
  if (pid != 0) {  // el padre
    wait(&status);
    shmdt(shm);
    shmctl(shm_id, IPC_RMID, NULL);
  } else { // el hijo
    sscanf(shm, "%i %lf %[^\n]", &i, &d, s);
    printf("%d %lf %s\n", i, d, s);
    shmdt(shm);
  }
  exit(0);
}
```

**Nota importante:** si la escritura se hiciera *después* del *fork()*, no habría ninguna garantía de que el hijo lea el dato después de que el padre lo haya escrito, ya que el orden de ejecución del padre y el hijo tras el *fork()* no está definido; podría darse una condición de carrera en la que el hijo lea memoria sin inicializar. Por eso aquí se escribe antes de crear al hijo. Cuando ambos procesos necesitan seguir modificando la memoria compartida después del *fork()*, sí se requiere un mecanismo explícito de sincronización, como se muestra más adelante con semáforos.

### Acceso a Arreglos en Memoria Compartida

También es posible acceder directamente a un arreglo de valores compartidos de igual forma como se realizaría si dicho arreglo fuera no compartido. El siguiente ejemplo muestra un padre que escribe 100 valores en el arreglo *shm* y luego su hijo lee dichos valores y los imprime.

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  int status;
  key_t shm_key = 999;

  int shm_id = shmget(shm_key, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);
  int *shm = (int *)shmat(shm_id, NULL, 0);

  // Se escribe el arreglo completo antes del fork() para evitar que el
  // hijo lea posiciones todavía no inicializadas por el padre.
  for (int i = 0; i < 100; i++)
    shm[i] = i * 10;

  pid_t pid = fork();
  if (pid != 0) {  // el padre
    wait(&status);
    shmdt(shm);
    shmctl(shm_id, IPC_RMID, NULL);
  } else { // el hijo
    for (int i=0;i<100;i++)
      printf("%d ", shm[i]);
    shmdt(shm);
  }
  exit(0);
}
```

### Sincronización en Memoria Compartida

La memoria compartida no proporciona mecanismos de sincronización automáticos. Es responsabilidad del programador implementar mecanismos de sincronización, como semáforos o cerrojos, para evitar condiciones de carrera cuando múltiples procesos acceden concurrentemente a la memoria compartida.

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <time.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void inicializar_semaforo(int sem_id, int valor) {
    union semun arg;
    arg.val = valor;
    semctl(sem_id, 0, SETVAL, arg);
}

void wait_semaforo(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void signal_semaforo(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main() {
    int status;
    key_t shm_key = 999;
    key_t sem_key = 888;
    
    // Crear semáforo
    int sem_id = semget(sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    inicializar_semaforo(sem_id, 1);
    
    // Crear memoria compartida
    int shm_id = shmget(shm_key, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);
    int *shm = (int *)shmat(shm_id, NULL, 0);
    *shm = 0;
    
    pid_t pid = fork();
    if (pid != 0) {  // el padre
        // Incrementar el contador con sincronización
        for (int i = 0; i < 1000; i++) {
            wait_semaforo(sem_id);
            (*shm)++;
            signal_semaforo(sem_id);
        }
        wait(&status);
        printf("Valor final: %d\n", *shm);
        shmdt(shm);
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_id, 0, IPC_RMID);
    } else { // el hijo
        // Incrementar el contador con sincronización
        for (int i = 0; i < 1000; i++) {
            wait_semaforo(sem_id);
            (*shm)++;
            signal_semaforo(sem_id);
        }
        shmdt(shm);
    }
    exit(0);
}
```

### Problema del Productor-Consumidor con Memoria Compartida

Uno de los problemas clásicos en compartición de memoria es el problema del productor-consumidor. En este escenario, un proceso productor genera datos y los coloca en un buffer compartido, mientras que un proceso consumidor los extrae y procesa. La sincronización es crucial para evitar que el productor escriba en un buffer lleno o que el consumidor lea de un buffer vacío.

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <time.h>

#define BUFFER_SIZE 10

struct shared_data {
    int buffer[BUFFER_SIZE];
    int in;    // índice donde el productor colocará el próximo item
    int out;   // índice donde el consumidor tomará el próximo item
};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void inicializar_semaforo(int sem_id, int valor) {
    union semun arg;
    arg.val = valor;
    semctl(sem_id, 0, SETVAL, arg);
}

void wait_semaforo(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void signal_semaforo(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main() {
    key_t shm_key = 999;
    key_t sem_empty = 888;
    key_t sem_full = 777;
    key_t sem_mutex = 666;
    
    // Crear semáforos
    int sem_empty_id = semget(sem_empty, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int sem_full_id = semget(sem_full, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int sem_mutex_id = semget(sem_mutex, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    
    inicializar_semaforo(sem_empty_id, BUFFER_SIZE);  // buffer vacío inicialmente
    inicializar_semaforo(sem_full_id, 0);              // ningún item disponible
    inicializar_semaforo(sem_mutex_id, 1);             // mutex para acceso al buffer
    
    // Crear memoria compartida
    int shm_id = shmget(shm_key, sizeof(struct shared_data), IPC_CREAT | S_IRUSR | S_IWUSR);
    struct shared_data *shared = (struct shared_data *)shmat(shm_id, NULL, 0);
    shared->in = 0;
    shared->out = 0;
    
    pid_t pid = fork();
    
    if (pid == 0) { // Proceso productor
        for (int i = 0; i < 20; i++) {
            wait_semaforo(sem_empty_id);  // espera espacio disponible
            wait_semaforo(sem_mutex_id);   // acceso exclusivo al buffer
            
            // Produce un item
            int item = rand() % 100;
            shared->buffer[shared->in] = item;
            shared->in = (shared->in + 1) % BUFFER_SIZE;
            printf("Productor produjo: %d\n", item);
            
            signal_semaforo(sem_mutex_id);   // libera el buffer
            signal_semaforo(sem_full_id);     // incrementa items disponibles
            
            sleep(rand() % 2);  // simula tiempo de producción
        }
        shmdt(shared);
        exit(0);
    } else { // Proceso consumidor
        for (int i = 0; i < 20; i++) {
            wait_semaforo(sem_full_id);   // espera que haya items
            wait_semaforo(sem_mutex_id);   // acceso exclusivo al buffer
            
            // Consume un item
            int item = shared->buffer[shared->out];
            shared->out = (shared->out + 1) % BUFFER_SIZE;
            printf("Consumidor consumió: %d\n", item);
            
            signal_semaforo(sem_mutex_id);   // libera el buffer
            signal_semaforo(sem_empty_id);    // incrementa espacio disponible
            
            sleep(rand() % 3);  // simula tiempo de consumo
        }
        wait(NULL);
        shmdt(shared);
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_empty_id, 0, IPC_RMID);
        semctl(sem_full_id, 0, IPC_RMID);
        semctl(sem_mutex_id, 0, IPC_RMID);
        exit(0);
    }
}
```

### Memoria Compartida con Estructuras Complejas

La memoria compartida puede almacenar estructuras de datos complejas, no solo arreglos simples. Esto permite implementar sistemas más sofisticados de comunicación entre procesos.

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_PERSONAS 20
#define MAX_NOMBRE 50

struct persona {
    int id;
    char nombre[MAX_NOMBRE];
    int edad;
};

struct shared_data {
    struct persona personas[MAX_PERSONAS];
    int num_personas;
};

int main() {
    key_t shm_key = 999;
    
    // Crear memoria compartida para la estructura
    int shm_id = shmget(shm_key, sizeof(struct shared_data), IPC_CREAT | S_IRUSR | S_IWUSR);
    struct shared_data *shared = (struct shared_data *)shmat(shm_id, NULL, 0);
    
    // Inicializar datos
    shared->num_personas = 0;
    
    pid_t pid = fork();
    
    if (pid == 0) { // Proceso hijo - añade personas
        struct persona p1 = {1, "Ana", 25};
        struct persona p2 = {2, "Carlos", 30};
        
        shared->personas[shared->num_personas++] = p1;
        shared->personas[shared->num_personas++] = p2;
        
        printf("Hijo añadió 2 personas\n");
        shmdt(shared);
        exit(0);
    } else { // Proceso padre - lee personas
        wait(NULL);
        
        printf("Padre leyó %d personas:\n", shared->num_personas);
        for (int i = 0; i < shared->num_personas; i++) {
            printf("ID: %d, Nombre: %s, Edad: %d\n", 
                   shared->personas[i].id, 
                   shared->personas[i].nombre, 
                   shared->personas[i].edad);
        }
        
        shmdt(shared);
        shmctl(shm_id, IPC_RMID, NULL);
    }
    
    exit(0);
}
```

## Paso de mensajes

Otro mecanismo utilizado para comunicar procesos consiste en el paso de mensajes. En este caso se crean mensajes que son transferidos entre los procesos. Múltiples procesos pueden leer y/o escribir a las colas de mensajes que se crean.

Las colas de mensajes ofrecen varias ventajas sobre la memoria compartida:

- Proporcionan sincronización automática
- Permiten comunicación estructurada mediante mensajes con tipo
- No requieren mecanismos de sincronización adicionales
- Son más seguras, ya que los datos se copian entre el espacio del kernel y el usuario

### Envío y recepción de mensajes

La llamada al sistema *msgget (msqkey,msgflg)* devuelve el identificador para la cola de mensajes asociada al valor de la llave *msqkey*. La función *msgsnd(msqid, msgp, msgsz, msgflg)* envía el mensaje apuntado por *msgp* a la cola identificada mediante *msqid*. Cada mensaje se le asigna un tipo que puede ser filtrado por el receptor.

La llamada al sistema *msgrcv(msqid, msgp, msgsz, msgtyp, msgflg)* permite recibir un mensaje que ha sido enviado previamente. El mensaje es almacenado en donde indique *msgp* y es filtrado por el tipo indicado por *msgtyp*.

Para eliminar la cola de mensajes se utiliza la función *msgctl* con el ID de la cola y el valor *IPC_RMID*.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSGSZ 128
struct message {
  long type;
  char text[MSGSZ];
} msg;

int main() {
  int status;
  key_t msqkey = 999;
  int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR);
  pid_t pid = fork();
  if (pid != 0) { // el padre
    msg.type = 1;
    strcpy(msg.text, "This is message 1");
    msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
    printf("Message sent\n");
    wait(&status);
    msgctl(msqid, IPC_RMID, NULL);
  } else { // el hijo
    msgrcv(msqid, &msg, MSGSZ, 1, 0);
    printf("Message received: %s\n", msg.text);
  }
  exit(0);
}
```

### Múltiples Tipos de Mensajes

Las colas de mensajes permiten filtrar mensajes por tipo, lo que facilita la comunicación dirigida entre múltiples procesos. Cada proceso puede recibir mensajes de un tipo específico mientras ignora los demás.

```c
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define MSGSZ 128
struct message {
  long type;
  char text[MSGSZ];
} msg;

int main() {
  int status;
  key_t msqkey = 999;
  int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR);
  
  pid_t pid1 = fork();
  if (pid1 == 0) {
    // Proceso hijo 1 - recibe mensajes tipo 1
    for (int i = 0; i < 3; i++) {
      msgrcv(msqid, &msg, MSGSZ, 1, 0);
      printf("Hijo1 recibió: %s\n", msg.text);
    }
    exit(0);
  }
  
  pid_t pid2 = fork();
  if (pid2 == 0) {
    // Proceso hijo 2 - recibe mensajes tipo 2
    for (int i = 0; i < 3; i++) {
      msgrcv(msqid, &msg, MSGSZ, 2, 0);
      printf("Hijo2 recibió: %s\n", msg.text);
    }
    exit(0);
  }
  
  // Proceso padre - envía mensajes de diferentes tipos
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 1");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 1");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 2");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 2");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 3");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 3");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  wait(NULL);
  wait(NULL);
  msgctl(msqid, IPC_RMID, NULL);
  exit(0);
}
```

### Problema de los Filósofos con Colas de Mensajes

El problema de los filósofos es otro clásico de sincronización que se puede implementar usando colas de mensajes. En este caso, cada filósofo es un proceso que necesita dos tenedores (recursos) para comer.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define NUM_FILOSOFOS 5
#define MSGSZ 64
#define TIPO_SOLICITUD 1
#define TIPO_LIBERACION 2
#define TIPO_TERMINADO 3

struct message {
    long type;
    int filosofo_id;
    char text[MSGSZ];
};

#define TAM_MSG (sizeof(struct message) - sizeof(long))

// Simulación del problema de los filósofos
void filosofo(int id, int cola_id) {
    struct message msg;
    int tenedor_izq = id;
    int tenedor_der = (id + 1) % NUM_FILOSOFOS;

    // El último filósofo toma primero el tenedor derecho y luego el
    // izquierdo (los demás lo hacen al revés). Esta asimetría rompe la
    // espera circular: si todos tomaran siempre primero su tenedor
    // izquierdo, los 5 podrían tomar uno cada uno y quedar bloqueados
    // esperando el segundo (interbloqueo clásico del problema).
    int primero = (id == NUM_FILOSOFOS - 1) ? tenedor_der : tenedor_izq;
    int segundo = (id == NUM_FILOSOFOS - 1) ? tenedor_izq : tenedor_der;

    for (int i = 0; i < 3; i++) {
        // Pensar
        printf("Filósofo %d pensando...\n", id);
        sleep(rand() % 3 + 1);

        // Tomar el primer tenedor (solicitar recurso)
        printf("Filósofo %d intenta tomar tenedor %d\n", id, primero);
        msg.type = TIPO_SOLICITUD;
        msg.filosofo_id = id;
        sprintf(msg.text, "%d", primero);
        msgsnd(cola_id, &msg, TAM_MSG, 0);
        msgrcv(cola_id, &msg, TAM_MSG, id + 10, 0); // espera confirmación

        // Tomar el segundo tenedor
        printf("Filósofo %d intenta tomar tenedor %d\n", id, segundo);
        msg.type = TIPO_SOLICITUD;
        msg.filosofo_id = id;
        sprintf(msg.text, "%d", segundo);
        msgsnd(cola_id, &msg, TAM_MSG, 0);
        msgrcv(cola_id, &msg, TAM_MSG, id + 10, 0);

        // Comer
        printf("Filósofo %d está comiendo...\n", id);
        sleep(rand() % 2 + 1);

        // Liberar tenedores
        printf("Filósofo %d libera tenedores\n", id);
        msg.type = TIPO_LIBERACION;
        msg.filosofo_id = id;
        sprintf(msg.text, "%d", primero);
        msgsnd(cola_id, &msg, TAM_MSG, 0);
        sprintf(msg.text, "%d", segundo);
        msgsnd(cola_id, &msg, TAM_MSG, 0);
    }

    // Avisar al padre (gestor) que este filósofo terminó
    msg.type = TIPO_TERMINADO;
    msg.filosofo_id = id;
    msgsnd(cola_id, &msg, TAM_MSG, 0);
    exit(0);
}

int main() {
    key_t cola_key = 999;
    int cola_id = msgget(cola_key, IPC_CREAT | S_IRUSR | S_IWUSR);

    // El padre crea los procesos filósofos
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            filosofo(i, cola_id);
        }
    }

    // El padre actúa como gestor de tenedores (recursos)
    int tenedores[NUM_FILOSOFOS];
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        tenedores[i] = 1; // 1 = disponible
    }

    struct message msg;
    int filosofos_activos = NUM_FILOSOFOS;
    while (filosofos_activos > 0) {
        msgrcv(cola_id, &msg, TAM_MSG, 0, 0); // recibe cualquier tipo de mensaje

        if (msg.type == TIPO_SOLICITUD) {
            int tenedor = atoi(msg.text);
            if (tenedores[tenedor]) {
                tenedores[tenedor] = 0;
                struct message resp;
                resp.type = msg.filosofo_id + 10;
                resp.filosofo_id = msg.filosofo_id;
                strcpy(resp.text, "OK");
                msgsnd(cola_id, &resp, TAM_MSG, 0);
            } else {
                // El tenedor está ocupado: se reenvía la solicitud para reintentarla
                msgsnd(cola_id, &msg, TAM_MSG, 0);
            }
        } else if (msg.type == TIPO_LIBERACION) {
            int tenedor = atoi(msg.text);
            tenedores[tenedor] = 1;
        } else if (msg.type == TIPO_TERMINADO) {
            filosofos_activos--;
        }
    }

    // Esperar a todos los hijos
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        wait(NULL);
    }

    msgctl(cola_id, IPC_RMID, NULL);
    return 0;
}
```

**Correcciones respecto a una implementación ingenua de este ejemplo:** el gestor (el proceso padre) necesita saber *cuál* filósofo hizo cada solicitud para poder responderle con el tipo de mensaje correcto (*id + 10*); por eso el mensaje incluye ahora el campo *filosofo_id*. Además, el gestor debe seguir atendiendo solicitudes y liberaciones mientras haya filósofos activos —no un número fijo de mensajes—, por lo que procesa mensajes en un ciclo hasta recibir el aviso de término (*TIPO_TERMINADO*) de los 5 filósofos; de lo contrario, el programa se queda bloqueado esperando confirmaciones que nunca llegarían. Finalmente, si los 5 filósofos siempre tomaran primero su tenedor izquierdo, podría darse el interbloqueo clásico del problema: los 5 toman su tenedor izquierdo a la vez y quedan esperando para siempre el derecho (que es el izquierdo de su vecino). Por eso el último filósofo toma los tenedores en el orden inverso, rompiendo la espera circular.

## Memoria Compartida y Colas de Mensajes

| Característica      | Memoria Compartida              | Colas de Mensajes               |
| ------------------- | ------------------------------- | ------------------------------- |
| Velocidad           | Muy rápida                      | Más lenta (requiere copias)     |
| Sincronización      | Requiere mecanismos adicionales | Sincronización automática       |
| Estructura de datos | Datos sin estructura            | Mensajes estructurados con tipo |
| Complejidad         | Mayor                           | Menor                           |
| Control de acceso   | Manual                          | Automático mediante tipos       |
| Persistencia        | Hasta que se elimine            | Hasta que se elimine            |
| Comunicación        | Bidireccional                   | Bidireccional con filtrado      |
| Escalabilidad       | Alta para grandes datos         | Media para datos pequeños       |
| Manejo de errores   | Menos robusto                   | Más robusto                     |

## Ejercicios


Todos los ejercicios siguientes deben resolverse con **un único programa** en el que **un proceso padre crea los procesos hijos con los que se comunica** (el padre hace *fork()* directamente de todos sus hijos; no debe haber hijos que a su vez hagan *fork()* de otros procesos).

1. Cree un proceso padre que cree 10 procesos hijos. Utilizando un arreglo en memoria compartida, el padre almacenará dos números aleatorios para cada hijo (puede usar *r = rand() % n;* para generar un número aleatorio entre 0 y n). Cada hijo deberá leer únicamente los dos números que le corresponden, sumarlos e imprimir el resultado.

2. Cree un proceso padre que cree 4 procesos hijos. Utilizando un arreglo en memoria compartida, el padre le asignará a cada hijo un intervalo de tiempo aleatorio entre 2 y 6 segundos. Cada hijo dormirá en intervalos de 1 segundo, mostrando en cada despertar: "Proceso [PID]: han pasado [X] segundos", hasta completar su tiempo total asignado.

3. Cree un proceso padre que cree 6 procesos hijos. Utilizando un arreglo en memoria compartida, el padre le asignará a cada hijo un número aleatorio entre 1 y 8. Cada hijo deberá calcular el factorial del número recibido y guardarlo en la posición que le corresponde de otro arreglo en memoria compartida. El padre deberá esperar a que todos sus hijos terminen y luego mostrará los factoriales calculados por cada uno.

4. Cree un proceso padre que cree 5 procesos hijos. Cada hijo suma su número de orden (1 a 5) 1000 veces a un contador entero en memoria compartida. Proteja el contador con un semáforo System V (*semget*, *semop*) y verifique que el padre obtiene el total esperado (1000 × 15).

5. Cree un proceso padre (productor) y un proceso hijo (consumidor) que se comunican mediante una posición de memoria compartida. Con dos semáforos System V, el padre produce 10 números aleatorios, uno a la vez, y espera a que el hijo lo consuma e imprima antes de producir el siguiente.

6. Cree un proceso padre que cree 2 procesos hijos y les envíe, utilizando una misma cola de mensajes, 5 mensajes a cada uno (use el campo *type* del mensaje para dirigir cada mensaje al hijo correspondiente). Cada hijo deberá recibir e imprimir los 5 mensajes que le fueron enviados a él.

7. Cree un proceso padre que cree un proceso hijo y se comuniquen bidireccionalmente usando dos colas de mensajes. El padre enviará 5 números aleatorios al hijo, quien calculará el cuadrado de cada número y se lo devolverá al padre. El padre deberá mostrar cada número original junto con su cuadrado recibido.

8. Cree un proceso padre que envíe a una cola de mensajes 6 mensajes de texto con prioridades 1, 2 o 3 (el *type* del mensaje) en orden arbitrario. Un único hijo debe recibirlos usando *msgrcv* con tipo negativo de modo que se impriman primero los de mayor prioridad (tipo menor).

9. Cree un proceso padre que envíe 12 números a una cola de mensajes y luego 3 mensajes de fin. Cree 3 procesos hijos que compiten por recibir mensajes de la cola; cada hijo calcula el cuadrado del número recibido e imprime "Hijo [n] procesó [x]", hasta recibir el mensaje de fin.

10. Cree un proceso padre que cree 3 procesos hijos, comunicados en cadena mediante distintas colas de mensajes. El padre enviará, a través de la primera cola, el texto "Ser o no ser, ese es el dilema" al primer hijo. El primer hijo recibirá el mensaje, repetirá cada palabra y lo enviará al segundo hijo mediante una segunda cola. El segundo hijo recibirá el texto y lo convertirá a mayúsculas, enviándolo mediante una tercera cola al tercer hijo, quien finalmente imprimirá el texto resultante.

11. Defina una estructura `struct empleado { int id; char nombre[20]; double salario; }` y cree en memoria compartida un arreglo de 5 empleados inicializado por el padre. Cada uno de 5 hijos aplica un aumento del 10 % al empleado que le corresponde. El padre espera a los hijos e imprime la tabla antes y después de los aumentos.

12. Cree un proceso padre que cree 4 procesos hijos. El padre almacenará en memoria compartida 5 números. El primer hijo sumará los dos primeros números y guardará el resultado en memoria compartida; el segundo hijo multiplicará ese resultado por el tercer número y guardará el nuevo resultado; el tercer hijo le restará el cuarto número al resultado anterior y lo guardará; y el cuarto hijo dividirá el resultado anterior entre el quinto número. Cada hijo deberá imprimir el resultado de la operación que realizó. *Nota:* el padre debe esperar (*wait*) a que cada hijo termine antes de crear al siguiente, para garantizar que el valor que cada hijo necesita ya haya sido escrito en memoria compartida.
