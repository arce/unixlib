#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    pid_t pid;
    
    pid = fork();
    
    if (pid == 0) {
        // Proceso hijo
        printf("Hijo: PID=%d, PPID=%d\n", getpid(), getppid());
        printf("Hijo: Durmiendo 5 segundos...\n");
        sleep(5);
        printf("Hijo: Ahora mi PPID es %d (debería ser 1 si mi padre terminó)\n", 
               getppid());
        printf("Hijo: Terminando\n");
        exit(0);
    } else {
        // Proceso padre
        printf("Padre: PID=%d, creé al hijo %d\n", getpid(), pid);
        printf("Padre: Terminando antes que mi hijo\n");
        exit(0);
    }
    
    return 0;
}
