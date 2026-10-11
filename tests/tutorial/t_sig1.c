#include <signal.h>
#include <stdio.h>

    void myhandler(int a) { printf("In myhandler\n"); }
    int main(void) {
       struct sigaction info, handler;
       handler.sa_handler = &myhandler;
       if (sigaction(SIGUSR1,&handler,&info) != -1)
           printf("New handler set.\n");
       raise(SIGUSR1);
}
