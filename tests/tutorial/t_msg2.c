#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define MSGSZ 128
struct message {
  long type;
  char text[MSGSZ];
} msg;

int main() {
  int status;
  key_t msqkey = 999;
  int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR);
  
  pid_t pid1 = fork();
  if (pid1 == 0) {
    // Proceso hijo 1 - recibe mensajes tipo 1
    for (int i = 0; i < 3; i++) {
      msgrcv(msqid, &msg, MSGSZ, 1, 0);
      printf("Hijo1 recibió: %s\n", msg.text);
    }
    exit(0);
  }
  
  pid_t pid2 = fork();
  if (pid2 == 0) {
    // Proceso hijo 2 - recibe mensajes tipo 2
    for (int i = 0; i < 3; i++) {
      msgrcv(msqid, &msg, MSGSZ, 2, 0);
      printf("Hijo2 recibió: %s\n", msg.text);
    }
    exit(0);
  }
  
  // Proceso padre - envía mensajes de diferentes tipos
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 1");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 1");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 2");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 2");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 1;
  strcpy(msg.text, "Mensaje para hijo1 - 3");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  msg.type = 2;
  strcpy(msg.text, "Mensaje para hijo2 - 3");
  msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
  
  wait(NULL);
  wait(NULL);
  msgctl(msqid, IPC_RMID, NULL);
  exit(0);
}
