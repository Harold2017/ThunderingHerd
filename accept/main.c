#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define ADDRESS "127.0.0.1"
#define PORT 6000
#define WORKERS 8

int worker_handler(int fd, int idx);

int main() {
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ADDRESS, &address.sin_addr);
    address.sin_port = htons(PORT);

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    bind(fd, (struct sockaddr *)&address, sizeof(address));
    listen(fd, 5);

    for (int i = 0; i < WORKERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker_handler(i, fd);
        } else {
            if (pid < 0) {
                printf("Unable to create child process\n");
            }
        }
    }

    // wait for child
    int status;
    wait(&status);
    return 0;
}

int worker_handler(int idx, int fd) {
    while(1) {
        printf("Worker %d (pid:%d) starts to accept connections\n", idx, getpid());

        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int connection = accept(fd, (struct sockaddr *)&client, &client_len);

        if (connection != -1) {
            printf("Worker %d successfully accepts connection from %s:%d\n", idx, inet_ntoa(client.sin_addr), client.sin_port);
        } else {
            printf("Worker %d failed to accept connection\n", idx);
        }
        close(connection);
    }
    return 0;
}
