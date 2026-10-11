    #include <signal.h>
    #include <stdio.h>
    int main(void) {
        kill(0,SIGUSR1);
        return 0;
}
