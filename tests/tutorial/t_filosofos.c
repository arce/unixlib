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
