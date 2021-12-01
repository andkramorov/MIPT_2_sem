#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

#define mask_full sigprocmask(SIG_SETMASK, &full, 0);
#define mask_def  sigprocmask(SIG_SETMASK, &def, 0);

#define sa_handler __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction

volatile sig_atomic_t ready, data;
sigset_t full, def, mask;

void err_exit(pid_t pid) { //terminates both parent and child process, pid is either parent id, or child id
    mask_full
    if(kill(pid, SIGTERM) == -1)
        fprintf(stderr," Can't send signal to pid %d to terminate it" , pid);
    exit(EXIT_FAILURE);
}
void SIGTERM_handler(int sig) {
    mask_full
    exit(EXIT_FAILURE);
}
void SIGUSR_parent_handler(int signal){
    if(signal == SIGUSR1)
        ready = 1;
}
void SIGUSR_child_handler(int signal) {
    if(signal == SIGUSR1)
        data = 0;
    else
        data = 1;
}
void send(int pid, int count, char *buffer) {
    int i;
    for(i = 0; i < 8*count; i++) {
        mask_full            // waiting until ready == 1
        if(ready == 0)
            sigsuspend(&def);
        mask_def
        ready = 0;           // sending the message
        if(buffer[i/8] & (1<<i%8)) {
            if(kill(pid, SIGUSR2) == -1)
                err_exit(pid);
        }
        else {
            if(kill(pid, SIGUSR1) == -1)
                err_exit(pid);
        }
    }
}
void receive(int pid, int count, char * buffer) {
    int i;
    for(i = 0 ; i < count; i++) buffer[i] = 0;
    for(i = 0; i < 8*count; i++) {
        mask_full
        if(kill(pid, SIGUSR1) == -1) // sending ready signal
            err_exit(pid);
        sigsuspend(&def);  // waiting fore response
        buffer[i/8] |= ((1<<(i%8)) * data);
    }
}
// SIGUSR1 == 0, SIGUSR2 == 1 for communication in pipe mode
// SIGUSR1 to indicate that child is ready to receive another signal
int main(int argc, char *argv[]) {
    int count;
    pid_t pid;
    struct sigaction act;
    char buffer[BUFFER_SIZE];

    sigfillset(&full);       // preparing signal masks
    sigaction(0, NULL, &act);
    def = act.sa_mask;
    mask = def;
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    act.sa_flags = SA_RESTART;
    act.sa_mask = mask;

    pid = fork();
    if(pid != 0) {              // parent process
        int file_input;
        act.sa_handler = SIGUSR_parent_handler;   // setting up signal handlers
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);
        act.sa_flags = 0;
        act.sa_mask = full;
        act.sa_handler = SIGTERM_handler;
        sigaction(SIGTERM, &act, 0);
        mask_full

        if(argc != 2) {          // opening input file
            fprintf(stderr, " Wrong argument count");
            err_exit(pid);
        }
        file_input = open(argv[1], O_RDONLY);
        if(file_input == -1) {
            fprintf(stderr, "Can't open file %s", argv[1]);
            perror(" ");
            err_exit(pid);
        }
        mask_def
        for(;;) {       // start sending file
            count = read(file_input, buffer, BUFFER_SIZE);
            if(count == -1)
                err_exit(pid);
            if(count == 0) { //all data has been written
                send(pid, sizeof(int), (char *) &count);
                break;
            }
            send(pid, sizeof(int), (char *) &count);
            send(pid, count, buffer);
        }
        close(file_input);
        exit(EXIT_SUCCESS);
    }
    else {
        pid = getppid();

        act.sa_handler = SIGUSR_child_handler;   // setting up signal handlers
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);
        act.sa_flags = 0;
        act.sa_mask = full;
        act.sa_handler = SIGTERM_handler;
        sigaction(SIGTERM, &act, 0);

        for(;;) {     // star receiving file
            receive(pid, sizeof(int), (char *) &count);
            if(count == 0)
                break;
            receive(pid, count, buffer);
            write(STDOUT_FILENO, buffer, count);
        }
        exit(EXIT_SUCCESS);
    }
}
