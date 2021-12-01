#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif
#define SERVER_FIFO "/tmp/server_pipe"
#define CLIENT_FIFO_TEMPLATE "/tmp/client_pipe%d%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 3*sizeof(pid_t)+3*sizeof(int))
#define message_size sizeof(pid_t) + sizeof(int)+2
// message template   @...pid....#
void pipe_clear() {
    int in;
    char buf[PIPE_BUF];
    in = open(SERVER_FIFO, S_IRUSR | O_NONBLOCK);
    fprintf(stderr, " Error has occured during writing to the pipe, clearing the pipe");
    if(in == -1) {
        perror(" can't open SERVER_FIFO for reading,exiting ");
        exit(EXIT_FAILURE);
    }
    while(read(in, (void *)buf, PIPE_BUF)){}
    close(in);
}
int main(int argc, char *argv[]) {
    int dummy_server_fifo_out, server_fifo_in, client_fifo_out, file_input, v;
    char flags;
    char message_buffer[message_size];
    char client_fifo[CLIENT_FIFO_NAME_LEN+1];
    if(argc != 2) {
        fprintf(stderr, "Wrong argument count");
        exit(EXIT_FAILURE);
    }
    file_input = open(argv[1], O_RDONLY);
    if(file_input == -1) {
        fprintf(stderr," Error opening file %s", argv[1]);
        perror(" ");
        exit(EXIT_FAILURE);
    }
    umask(0);
    v = mkfifo(SERVER_FIFO, S_IWUSR | S_IRUSR);
    if((v == -1) && (errno != EEXIST)) {
        perror(" Can't create pipe ");
        exit(EXIT_FAILURE);
    }
    server_fifo_in = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK); //  we use O_NONBLOCK, so we won't block after opening
    if(server_fifo_in == -1) {
        perror("Can't open fifo for reading ");
        exit(EXIT_FAILURE);
    }
    dummy_server_fifo_out = open(SERVER_FIFO, O_WRONLY);  // so we won't see EOF while trying to read;
    if(dummy_server_fifo_out == -1) {
        perror("Can't open fifo for writing ");
        exit(EXIT_FAILURE);
    }
    flags = fcntl(server_fifo_in, F_GETFL);
    flags &= ~O_NONBLOCK; /* Disable O_NONBLOCK bit for server fifo in */
    fcntl(server_fifo_in, F_SETFL, flags);
    for(;;) {
        for(;;) {
            v = read(server_fifo_in, (void *) message_buffer, message_size);
            if(v != message_size || message_buffer[0] != '@' || message_buffer[message_size-1] != '#') {
                fprintf(stderr, " Error during reading from the pipe, clearing the pipe ");
                pipe_clear();
            }
            else
                break;
        }
        //recreating client fifo path
        snprintf(client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, *(int *)(message_buffer+1), *(int *)(message_buffer+1+sizeof(int)));
        client_fifo_out = open(client_fifo,  O_WRONLY);
        if(client_fifo_out == -1) {
            fprintf(stderr, " Can't open client fifo for reading");
            perror(" ");
            continue;
        }
        while(splice(file_input, NULL, client_fifo_out, NULL, PIPE_BUF, NULL ) > 0) {}
        close(client_fifo_out);
        break;
    }
    close(file_input);
    close(dummy_server_fifo_out);
    close(server_fifo_in);
}
