    #include <signal.h>
    #include <stdio.h>
    #include <unistd.h>
    void myhandler(int a) { printf("Signal Received\n"); }
    int main(void) {
       struct sigaction info, handler;
       handler.sa_handler = &myhandler;
       if (sigaction(SIGINT,&handler,&info) != -1)
           printf("New handler set.\n");
       while (1)
pause(); }
