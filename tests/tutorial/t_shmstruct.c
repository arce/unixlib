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
