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
