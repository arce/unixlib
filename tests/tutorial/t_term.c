#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

int ejecutando = 1;

void manejador_terminacion(int sig) {
    printf("Proceso %d: Recibida señal de terminación, realizando limpieza...\n", 
           getpid());
    ejecutando = 0;
}

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Proceso hijo
        signal(SIGTERM, manejador_terminacion);
        signal(SIGINT, manejador_terminacion);
        
        printf("Hijo: PID=%d, trabajando...\n", getpid());
        while (ejecutando) {
            printf("Hijo: Realizando trabajo...\n");
            sleep(1);
        }
        printf("Hijo: Terminando de forma ordenada\n");
        exit(0);
    } else {
        // Proceso padre
        printf("Padre: PID=%d, hijo creado con PID=%d\n", getpid(), pid);
        sleep(3);
        printf("Padre: Enviando señal de terminación al hijo\n");
        kill(pid, SIGTERM);
        wait(NULL);
        printf("Padre: Hijo terminado\n");
    }
    
    exit(0);
}
