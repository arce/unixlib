#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
int main () {
  int i,status,n=3;
  for (i=0; i<n; i++) fork();
  printf ("Process ID: %ld, Parent ID: %ld\n",
           (long)getpid (), (long)getppid());
  for (i=0; i<n; i++) wait(&status);
  exit(0);
}
