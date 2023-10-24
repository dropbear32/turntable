/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_SOCKET_H
#define TURNTABLE_SOCKET_H

#include "../util/pthread.h"
#include "../util/queue.h"

#ifdef _WIN32
typedef long long ssize_t;
#else
#include <netdb.h>
#endif

#include <stdbool.h>

#define PACKET_SIZE    4096
#define HEADER_LEN     1
#define BACKLOG_LEN    16
#define TURNTABLE_PORT "14135"

#define TYPE_MASK             0xE0
#define INFO_MASK             0x1F
#define PACKET_COMMAND        0x00
#define PACKET_AUDIOFILE      0x01
#define PACKET_ERROR          0x02
#define COMMAND_PLAY          0x00
#define COMMAND_PAUSE         0x01
#define COMMAND_SKIP          0x02
#define AUDIOFILE_START       0x00
#define AUDIOFILE_MIDDLE      0x01
#define AUDIOFILE_END         0x02
#define AUDIOFILE_PLAY_REQ    0x03
#define AUDIOFILE_FILE_REQ    0x04
#define AUDIOFILE_CLEAR_QUEUE 0x05
#define ERROR_BAD_PACKET      0x00
#define ERROR_FILE_MISSING    0x01

struct turntable_queue_data {
    char *filename;
    char *src;
};

struct socket_state {
    pthread_t socket_thread;
    pthread_mutex_t socket_command_mutex;
    pthread_cond_t socket_cond;
    bool socket_thread_is_running;
    bool socket_thread_should_run;
    bool socket_connected;
    bool is_client;
    char *send_data_buf;
    bool send_data_ready;
    struct turntable_queue *file_queue;
};

extern struct socket_state SocketState;

void *start_socket(void *);
ssize_t send_data(char const *const, size_t, int);
void parse_packet(char const *const, ssize_t, int, char const *const);
void clear_queued_files(struct turntable_queue *, char *, bool);

#endif /* TURNTABLE_SOCKET_H */
