#include <stdio.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
struct msg {
    long type;
};
void errexit(int qid) {
    if(errno == EIDRM)
        perror("message queue was deleted");
    else if(msgctl(qid, IPC_RMID, 0) == -1) {
        if(errno == EIDRM)
            perror("message queue was deleted ");
        else if(errno == EPERM)
            perror("msgctl error, can't delete the message queue due to priority issues");
        else perror(" msgctl error, can't delete the message queue ");
    }
    else
        fprintf(stderr, " Successfully deleted the message queue ");
    exit(EXIT_FAILURE);
}
int main(int argc, char * argv[]) {
    int i, qid, pid, n, v;
    struct msg message;
    setvbuf(stdout, 0, _IONBF, 0);
    if(argc != 2) {
        fprintf(stderr, " Wrong arument count\n");
        exit(EXIT_FAILURE);
    }
    n = atoi(argv[1]);
    qid = msgget(IPC_PRIVATE, IPC_CREAT  | 0666);
    if (qid == -1) {
        perror("msgget error");
        exit(EXIT_FAILURE);
    }
    for(i = 1; i <= n; i++) {
        pid = fork();
        if(pid == -1) {
            perror("error during fork");
            errexit(qid);
        }
        if(pid == 0) {
            break;
        }
    }
    if(pid == 0) {
        struct msg message;
        if(i == 1) {
            message.type = 1;
            printf(" 1 ");
            if(msgsnd(qid, &message, 0, 0) == -1) {
                perror(" Error sending message");
                errexit(qid);
            }
        }
        else  {
            if(msgrcv(qid, &message, 0, (long) i-1, 0) == -1) {
                perror("Error receiving message");
                errexit(qid);
            }
            message.type = i;
            printf(" %d ", i);
            if(msgsnd(qid, &message, 0, 0)  == -1) {
                perror(" Error sending message");
                errexit(qid);
            }
 
        }
    }
    else {
        msgrcv(qid, &message, 0, n, 0);
        v = msgctl(qid, IPC_RMID, 0);
        if(v == -1) {
            perror("msgctl error, can't delete the message queue");
            exit(EXIT_FAILURE);
        }
    }
}
