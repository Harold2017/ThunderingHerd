//
// Created by harold on 6/1/2020.
//

#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>

#define ADDRESS "127.0.0.1"
#define PORT 6000
#define WORKERS 8
#define MAX_EVENTS 64

int worker_handler(int idx, int fd, int epoll_fd, struct epoll_event *events);

static int set_non_blocking(int fd);

void valid_ret(int errno) {
    if (errno == -1) {
        abort();
    }
}

int main() {
    int fd, epoll_fd;
    struct epoll_event event;
    struct epoll_event *events;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ADDRESS, &address.sin_addr);
    address.sin_port = htons(PORT);
    fd = socket(PF_INET, SOCK_STREAM, 0);
    bind(fd, (struct sockaddr *)&address, sizeof(address));

    int ret = set_non_blocking(fd);
    valid_ret(ret);
    ret = listen(fd, 5);
    valid_ret(ret);

    epoll_fd = epoll_create(MAX_EVENTS);
    valid_ret(epoll_fd);
    event.data.fd = fd;
    // add flag `EPOLLEXCLUSIVE` here
    event.events = EPOLLIN | EPOLLEXCLUSIVE; // event.events = EPOLLIN;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    valid_ret(ret);
    events = calloc(MAX_EVENTS, sizeof(event));

    for (int i = 0; i < WORKERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker_handler(i, fd, epoll_fd, events);
        } else {
            if (pid < 0) {
                printf("Unable to create child process\n");
            }
        }
    }

    // wait for child
    int status;
    wait(&status);
    free(events);
    close(fd);
    return 0;
}

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("set_non_blocking: fd invalid");
        return -1;
    }
    flags |= O_NONBLOCK;

    int ret = fcntl(fd, F_SETFL, flags);
    if (ret == -1) {
        perror("set_non_blocking: can NOT set nonblocking flag");
        return -1;
    }
    return 0;
}

int worker_handler(int idx, int fd, int epoll_fd, struct epoll_event *events) {
    printf("Create worker %d (pid:%d)\n", idx, getpid());
    while(1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        printf("Worker %d (pid:%d) starts to accept connections\n", idx, getpid());

        for (int i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
                printf("worker_handler: epoll error: %d\n", i);
                close(events[i].data.fd);
                continue;
            } else if (fd == events[i].data.fd) {
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
        }
    }
    return 0;
}