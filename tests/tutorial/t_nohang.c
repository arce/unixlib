#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
int main () {
  int child, status;
  pid_t pid = fork();
  while ((pid != 0) && 
	       ((child = waitpid(-1,&status,WNOHANG))==0)) { 
    printf ("Esperando ..\n");
  }
  if (pid!=0) { // el padre
    printf("Final de hijo: %ld\n",child);
  } else { // el hijo
    sleep(1);
    printf("Child ID: %ld\n", (long)getpid());
  }
  exit(0);
}
