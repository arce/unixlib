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
