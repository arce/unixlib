#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];  // Array para los descriptores de archivo
    pid_t pid;
    char buffer[100];
    char mensaje[] = "Hola desde el proceso padre!";
    
    // Crear la tubería
    if (pipe(fd) == -1) {
        perror("Error al crear la tubería");
        return 1;
    }
    
    // Crear proceso hijo
    pid = fork();
    
    if (pid == -1) {
        perror("Error en fork");
        return 1;
    }
    
    if (pid == 0) {
        // Código del proceso hijo
        close(fd[1]);  // Cerrar extremo de escritura (no lo usa el hijo)
        
        // Leer mensaje del padre
        int bytes_leidos = read(fd[0], buffer, sizeof(buffer));
        if (bytes_leidos > 0) {
            printf("Proceso hijo recibió: %s\n", buffer);
        }
        
        close(fd[0]);  // Cerrar extremo de lectura
    } else {
        // Código del proceso padre
        close(fd[0]);  // Cerrar extremo de lectura (no lo usa el padre)
        
        // Escribir mensaje al hijo
        printf("Proceso padre enviando: %s\n", mensaje);
        write(fd[1], mensaje, strlen(mensaje) + 1);
        
        close(fd[1]);  // Cerrar extremo de escritura
        
        // Esperar a que el hijo termine
        wait(NULL);
    }
    
    return 0;
}
