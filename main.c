#include "sh.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void sig_handler(int signum);

int main(int argc, char **argv, char **envp) {
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);

    return sh(argc, argv, envp);
}

void sig_handler(int signum) {
    switch (signum) {
        case SIGINT:
            printf("\nReceived SIGINT. Exiting...\n");
            exit(0);
            break;
        case SIGTSTP:
            printf("\nReceived SIGTSTP. Suspending...\n");
            signal(SIGTSTP, SIG_IGN);
            kill(getpid(), SIGSTOP);
            signal(SIGTSTP, sig_handler);
            printf("\nResuming...\n");
            break;
        default:
            break;
    }
}

