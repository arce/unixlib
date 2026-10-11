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
