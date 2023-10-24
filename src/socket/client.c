/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "client.h"

#ifdef _WIN32
#include "winsockfix.h"
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "../util/atomic.h"
#include "../util/pthread.h"
#include "../util/queue.h"
#include "../util/sleep.h"
#include "socket.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_client(char const *const hostname) {
    int status;
    struct addrinfo hints;
    struct addrinfo *info;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Generate info
    if ((status = getaddrinfo(hostname, TURNTABLE_PORT, &hints, &info))) {
        fprintf(stderr, "getaddrinfo: %s (provided hostname was '%s')\n", gai_strerror(status), hostname);
        pthread_cond_signal(&SocketState.socket_cond);
        return;
    }

    // Get a socket descriptor
    struct addrinfo *p;
    for (p = info; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        printf("Attempting connection...\n");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd);
            continue;
        }
        printf("Connected\n");

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "failed to connect\n");
        pthread_cond_signal(&SocketState.socket_cond);
        return;
    }

    char s[64];
    getnameinfo(p->ai_addr, sizeof(*p->ai_addr), s, sizeof(s), NULL, 0, NI_NAMEREQD | NI_NUMERICHOST);
    printf("connected to %s\n", s);

    freeaddrinfo(info);

    SocketState.socket_connected = true;
    pthread_cond_signal(&SocketState.socket_cond);

    char *recv_data_buf = malloc(PACKET_SIZE);
    SocketState.send_data_buf = malloc(PACKET_SIZE);
    ssize_t recv_len;

    // Main loop
    while ((recv_len = recv(sockfd, recv_data_buf, PACKET_SIZE, MSG_DONTWAIT))) {
        if (!SocketState.socket_thread_should_run) {
            close(sockfd);
            return;
        }
        if (SocketState.send_data_ready) {
            ssize_t sent_len = send_data(SocketState.send_data_buf, PACKET_SIZE, sockfd);
            if (sent_len < 0) {
                close(sockfd);
                sockfd = -1;
                SocketState.send_data_buf[0] = -1;
            } else {
                memset(SocketState.send_data_buf, 0, PACKET_SIZE);
            }
            atomic_write(&SocketState.send_data_ready, false);
        }
        if (recv_len <= 0) {
#ifdef _WIN32
            int err;
            if ((err = WSAGetLastError()))
                printf("err = %d\n", err);
#endif
            msleep(10);
            continue;
        }
        parse_packet(recv_data_buf, recv_len, sockfd, hostname);
    }
    printf("%s disconnected\n", s);

    free(SocketState.send_data_buf);
    SocketState.send_data_buf = NULL;
    SocketState.send_data_ready = false;
    SocketState.socket_connected = false;
    close(sockfd);
}
