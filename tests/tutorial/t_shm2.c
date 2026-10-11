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
