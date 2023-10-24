/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "server.h"

#ifdef _WIN32
#include "winsockfix.h"
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "../util/sleep.h"
#include "socket.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_server(void) {
    int status;
    struct addrinfo hints;
    struct addrinfo *info;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Generate info
    if ((status = getaddrinfo(NULL, TURNTABLE_PORT, &hints, &info))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        pthread_cond_signal(&SocketState.socket_cond);
        return;
    }

    if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
        perror("socket");
        pthread_cond_signal(&SocketState.socket_cond);
        return;
    }

    // Allow address reuse; this solves EADDRINUSE when quickly restarting the server
    int one = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        fprintf(stderr, "failed to set SO_REUSEADDR for the listening socket; this may lead to errors if you quickly disconnect/reconnect\n");
    }

    // Bind our descriptor to the info we generated
    if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
        perror("bind");
        pthread_cond_signal(&SocketState.socket_cond);
        return;
    }

    freeaddrinfo(info);

    // Set to non-blocking
#ifdef _WIN32
    u_long iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

    listen(sockfd, BACKLOG_LEN);
    printf("listening on port %s\n", TURNTABLE_PORT);

    pthread_cond_signal(&SocketState.socket_cond);

    int incoming_sockfd = -1;
    struct sockaddr incoming_info;
    socklen_t incoming_info_len = sizeof(incoming_info);
    char *recv_data_buf = malloc(PACKET_SIZE);
    SocketState.send_data_buf = malloc(PACKET_SIZE);
    ssize_t recv_len;
    char client_name[64];

    // Logical packets can be transmitted over multiple real packets.
    // This acts as a buffer so the logical-packet parser gets full packets.
    char *recv_data_packet_buf = malloc(PACKET_SIZE * 2);
    ssize_t recv_packet_len = 0;

    // Main loop
    while (true) {
        do {
            if (!SocketState.socket_thread_should_run) {
                close(incoming_sockfd);
                close(sockfd);
                return;
            }
            incoming_sockfd = accept(sockfd, &incoming_info, &incoming_info_len);
            msleep(10);
        } while (incoming_sockfd < 0);
        getnameinfo(&incoming_info, sizeof(incoming_info), client_name, sizeof(client_name), NULL, 0, 0);
        printf("connected to %s\n", client_name);
        SocketState.socket_connected = true;

        while ((recv_len = recv(incoming_sockfd, recv_data_buf, PACKET_SIZE, MSG_DONTWAIT))) {
            if (!SocketState.socket_thread_should_run) {
                free(SocketState.send_data_buf);
                SocketState.send_data_ready = false;
                close(incoming_sockfd);
                close(sockfd);
                return;
            }
            if (SocketState.send_data_ready) {
                ssize_t sent_len = send_data(SocketState.send_data_buf, PACKET_SIZE, incoming_sockfd);
                if (sent_len < 0) {
                    close(incoming_sockfd);
                    SocketState.send_data_buf[0] = -1;
                } else {
                    memset(SocketState.send_data_buf, 0, PACKET_SIZE);
                }
                SocketState.send_data_ready = false;
            }
            if (recv_len <= 0) {
                msleep(10);
                continue;
            }
            memcpy(recv_data_packet_buf + recv_packet_len, recv_data_buf, recv_len);
            recv_packet_len += recv_len;
            if (recv_packet_len >= PACKET_SIZE) {
                parse_packet(recv_data_packet_buf, PACKET_SIZE, incoming_sockfd, client_name);
                recv_packet_len -= PACKET_SIZE;
                memmove(recv_data_packet_buf, recv_data_packet_buf + PACKET_SIZE, recv_packet_len);
            }
        }

        printf("%s disconnected\n", client_name);
        clear_queued_files(SocketState.file_queue, client_name, false);
        SocketState.socket_connected = false;
        close(incoming_sockfd);
    }
}
