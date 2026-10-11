#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
int main () {
  int status,i,n=3;
  for (i=0; i<n; i++) {
    if (fork()!=0) break;
  }
  printf ("Process ID: %ld, Parent ID: %ld\n",
          (long)getpid(), (long)getppid());
  wait(&status);
  exit(0); 
}
