#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/prctl.h>

#define BUFFER_SIZE 4096
#define mask_usr  sigprocmask(SIG_SETMASK, &sigusr_mask, 0);
#define mask_full sigprocmask(SIG_SETMASK, &full, 0);
#define mask_def  sigprocmask(SIG_SETMASK, &def, 0);
#define sa_handler __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction

volatile sig_atomic_t ready, data;
sigset_t full, def, sigusr_mask;

void err_exit(pid_t pid);
void SIGTERM_handler(int sig);
void SIGCHLD_handler(int sig);
void SIGPRNT_handler(int sig) ;
void SIGUSR_parent_handler(int sig);
void SIGUSR_child_handler(int sig);
void send(int pid, int count, char *buffer);
void receive(int pid, int count, char * buffer);
// SIGUSR1 == 0, SIGUSR2 == 1 for communication in pipe mode
// SIGUSR1 to indicate that child is ready to receive another signal
int main(int argc, char *argv[]) {
    int count;
    pid_t pid;
    struct sigaction act;
    char buffer[BUFFER_SIZE];
    // preparing signal masks and establishing signal handlers
    sigaction(0, NULL, &act); 
    def = act.sa_mask;
    sigusr_mask = def;
    sigfillset(&full);   
    act.sa_flags = 0;
    act.sa_mask = full;
    act.sa_handler = SIGCHLD_handler;
    sigaction(SIGCHLD, &act, NULL);
    act.sa_handler = SIGTERM_handler;
    sigaction(SIGTERM, &act, NULL);
    sigaddset(&sigusr_mask, SIGUSR1); 
    sigaddset(&sigusr_mask, SIGUSR2);
    sigdelset(&sigusr_mask, SIGCHLD);
    act.sa_flags = SA_RESTART;
    act.sa_mask = sigusr_mask;
    pid = fork();
    if(pid != 0) {              // parent process
        int file_input;
        act.sa_handler = SIGUSR_parent_handler;   // setting up signal handlers
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);

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
        prctl(PR_SET_PDEATHSIG, SIGCHLD); // so we can get SIGCHLD whenever parent process terminates
        act.sa_handler = SIGUSR_child_handler;   // setting up signal handlers
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);
        act. sa_flags = 0;
        sigfillset(&act.sa_mask);
        act.sa_handler = SIGPRNT_handler;
        sigaction(SIGCHLD, &act, NULL);
        pid = getppid();

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

void err_exit(pid_t pid) { //terminates both parent and child process, pid is either parent id, or child id
    mask_full
    if(kill(pid, SIGTERM) == -1)
        fprintf(stderr," Can't send signal to pid %d to terminate it" , pid);
    exit(EXIT_FAILURE);
}
void SIGTERM_handler(int sig) {
    exit(EXIT_FAILURE);
}
void SIGCHLD_handler(int sig) {
    fprintf(stderr, " Child process has been terminated. Aborting");
    exit(EXIT_FAILURE);
}
void SIGPRNT_handler(int sig) {
    fprintf(stderr, " Parent process has been terminated. Aborting");
    exit(EXIT_FAILURE);
}
void SIGUSR_parent_handler(int sig){
        ready = 1;
}
void SIGUSR_child_handler(int sig) {
    if(sig == SIGUSR1)
        data = 0;
    else
        data = 1;
}
void send(int pid, int count, char *buffer) {
    int i;    
    for(i = 0; i < 8*count; i++) {
        mask_usr
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
        mask_usr
        if(kill(pid, SIGUSR1) == -1) // sending ready signal
            err_exit(pid);
        sigsuspend(&def);  // waiting for response
        mask_def
        buffer[i/8] |= ((1<<(i%8)) * data);
    }
}