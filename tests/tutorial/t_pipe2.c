#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void escritor() {
    // Escribe varias líneas a stdout
    char *mensajes[] = {
        "Manzana", "Banana", "Cereza", "Dátil", "Uva",
        "Fruta del dragón", "Higo", "Kiwi", "Limón", "Mango", NULL
    };
    
    for (int i = 0; mensajes[i] != NULL; i++) {
        printf("%s\n", mensajes[i]);
    }
}

void lector() {
    char linea[100];
    int contador = 0;
    
    // Lee líneas de stdin y las procesa
    while (fgets(linea, sizeof(linea), stdin) != NULL) {
        // Eliminar salto de línea
        linea[strcspn(linea, "\n")] = 0;
        
        // Procesamiento simple: convertir a mayúsculas
        for (int i = 0; linea[i]; i++) {
            if (linea[i] >= 'a' && linea[i] <= 'z') {
                linea[i] = linea[i] - 32;
            }
        }
        
        printf("PROCESADO: %s\n", linea);
        contador++;
    }
    
    printf("Total procesado: %d elementos\n", contador);
}

int main() {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // Hijo 1 - Escritor
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        escritor();
        return 0;
    }

    if (fork() == 0) {
        // Hijo 2 - Lector
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        lector();
        return 0;
    }

    // Padre
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    
    printf("Pipeline finalizado: escritor | lector\n");
    return 0;
}
