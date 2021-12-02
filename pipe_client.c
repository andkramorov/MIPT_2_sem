#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif
#define SERVER_FIFO "/tmp/server_pipe"
#define CLIENT_FIFO_TEMPLATE "/tmp/client_pipe%d%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 3*sizeof(pid_t)+3*sizeof(int))
#define message_size sizeof(pid_t) + sizeof(int)+2
#define MAX__TRYES 10000
#define MAX_TRYES_BEFORE_CONNECTION_IS_ESTABLISHED 1000000
#define MAX_WRITE_TRYES 10
// message template   @...pid....i...#
int main() {
    int server_fifo_out, dummy_server_fifo_in, client_fifo_in, dummy_client_fifo_out, v, i, j;
    char client_fifo[CLIENT_FIFO_NAME_LEN];
    char flags;
    char message[message_size];
    char client_pipe_buffer[PIPE_BUF];
    struct timespec time;
    umask(0);
    v = mkfifo(SERVER_FIFO, S_IWUSR | S_IRUSR);
    if((v == -1) && (errno != EEXIST)) {
        perror(" Can't create server fifo ");
        exit(EXIT_FAILURE);
    }
    dummy_server_fifo_in = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK); //creating dummy server fifo in, so we won't get SIGPIPE trying to write to server fifo
    if(dummy_server_fifo_in == -1) {
        perror("Can't open dummy server fifo for reading ");
        exit(EXIT_FAILURE);
    }
    server_fifo_out = open(SERVER_FIFO, O_WRONLY);
    if(server_fifo_out == -1) {
        perror("Can't open server fifo for writing ");
        exit(EXIT_FAILURE);
    }
    time.tv_sec = 0;
    time.tv_nsec = 1000000;
    for(i = 0; i < MAX_WRITE_TRYES; i++) {
        snprintf(client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, getpid(), i);   //creating client fifo path
        v = mkfifo(client_fifo, S_IWUSR | S_IRUSR);
        if (v == -1) {
            if(errno == EEXIST) {
                perror(" Client fifo already exists");
                exit(EXIT_FAILURE);
            }
            perror(" Can't create  client fifo ");
            exit(EXIT_FAILURE);
        }
        client_fifo_in = open(client_fifo, O_RDONLY | O_NONBLOCK);
        if (client_fifo_in == -1) {
            perror(" Can't open client fifo for reading ");
            exit(EXIT_FAILURE);
        }
        dummy_client_fifo_out = open(client_fifo, O_WRONLY); // so we won't see EOF
        if(dummy_client_fifo_out == -1) {
            perror(" Can't open dummy client fifo for writing ");
            exit(EXIT_FAILURE);
        }
        message[0] = '@';// creating message
        message[message_size-1] = '#';
        *(int *) (message+1+sizeof(pid_t));
        *(pid_t *)(message+1) = getpid();
        write(server_fifo_out, message, message_size);//writing message with pid to server fifo

        for(j = 0; j < MAX_TRYES_BEFORE_CONNECTION_IS_ESTABLISHED; j++) {//trying to receive first data package; if non has been received, aborting and starting anew
            v = read(client_fifo_in, client_pipe_buffer, PIPE_BUF);
            if(v == -1) {// no data has been received
                if(errno != EAGAIN) 
                    exit(EXIT_FAILURE);
            }
            else {// data has been received, starting normal reading loop
                if(write(STDOUT_FILENO, client_pipe_buffer, v) < v){
                    perror(" Can't write to output file descriptor, aborting");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            nanosleep(&time, 0);
        }
        if(j == MAX_TRYES_BEFORE_CONNECTION_IS_ESTABLISHED) {
            fprintf(stderr, " Connection  is not established, repeating attempt to establish connection");
            continue;
        }
        flags = fcntl(client_fifo_in, F_GETFL);// turning off O_NONBLOCK flag, so futher read will block until more data is availible
        flags &= ~O_NONBLOCK;
        fcntl(client_fifo_in, F_SETFL, flags);
        close(dummy_client_fifo_out);
        for(;;) {
            v = read(client_fifo_in, client_pipe_buffer, PIPE_BUF);
            if(v == -1) {
                perror(" Error during read syscall, aborting");
                exit(EXIT_FAILURE);
            }
            if(v == 0) // all data has been read
                break; 
            // successfully retrieved data from pipe
            if(write(STDOUT_FILENO, client_pipe_buffer, v) < v){
                perror(" Can't write to output file descriptor, aborting");
                exit(EXIT_FAILURE);
            }
        }
        close(client_fifo_in);
        break;
    }
    close(server_fifo_out);
    close(dummy_server_fifo_in);
    if(i == MAX_WRITE_TRYES) {
        fprintf(stderr, " No writer was found, exiting");
        exit(EXIT_FAILURE);
    }
    else
        exit(EXIT_SUCCESS);
}
