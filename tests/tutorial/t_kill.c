#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
int main () {
	pid_t pid = fork();
  if (pid == 0) { // el hijo
    while (1)
      printf ("Hijo corriendo ...\n");
  } else {  // el padre
    sleep(1);
    kill(pid, SIGKILL);
  }
  exit(0);
}
