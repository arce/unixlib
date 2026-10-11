#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSGSZ 128
struct message {
  long type;
  char text[MSGSZ];
} msg;

int main() {
  int status;
  key_t msqkey = 999;
  int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR);
  pid_t pid = fork();
  if (pid != 0) { // el padre
    msg.type = 1;
    strcpy(msg.text, "This is message 1");
    msgsnd(msqid, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
    printf("Message sent\n");
    wait(&status);
    msgctl(msqid, IPC_RMID, NULL);
  } else { // el hijo
    msgrcv(msqid, &msg, MSGSZ, 1, 0);
    printf("Message received: %s\n", msg.text);
  }
  exit(0);
}
