/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "turntable.h"

#include "audio/audio.h"
#include "socket/socket.h"
#include "util/atomic.h"
#include "util/file.h"
#include "util/magic.h"
#include "util/pthread.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct socket_state SocketState;
struct audio_state AudioState;

void turntable_init(void) {
    pthread_mutex_init(&SocketState.socket_command_mutex, NULL);
    pthread_cond_init(&SocketState.socket_cond, NULL);

    AudioState.flags.handler_thread_should_run = true;
    pthread_create(&AudioState.threads.audio_handler, NULL, audio_handler, NULL);
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

static void start_socket_thread(char *hostname) {
    if (SocketState.socket_thread_is_running) {
        fprintf(stderr, "error: socket already running\n");
        return;
    }
    atomic_write(&SocketState.socket_thread_should_run, true);
    pthread_mutex_lock(&SocketState.socket_command_mutex);
    pthread_create(&SocketState.socket_thread, NULL, start_socket, hostname);
    pthread_cond_wait(&SocketState.socket_cond, &SocketState.socket_command_mutex);
    pthread_mutex_unlock(&SocketState.socket_command_mutex);
}

// Server

void turntable_init_server(void) {
    start_socket_thread(NULL);
}

void turntable_close_server(void) {
    atomic_write(&SocketState.socket_thread_should_run, false);
    while (SocketState.socket_thread_is_running)
        continue;
}

bool turntable_connected_as_server(void) {
    return SocketState.socket_connected && !SocketState.is_client;
}

// Client

void turntable_init_client(char *hostname) {
    start_socket_thread(hostname);
}

void turntable_close_client(void) {
    atomic_write(&SocketState.socket_thread_should_run, false);
    while (SocketState.socket_thread_is_running)
        continue;
}

bool turntable_connected_as_client(void) {
    return SocketState.socket_connected && SocketState.is_client;
}

// Audio

void turntable_queue_file(char const *const filename) {
    if (SocketState.socket_connected && SocketState.is_client) {
        if (strlen(filename) > PACKET_SIZE - 2) {
            fprintf(stderr, "File path is too long\n");
            return;
        }
        atomic_write(SocketState.send_data_buf, PACKET_AUDIOFILE << 5 | AUDIOFILE_PLAY_REQ);
        for (size_t i = 1; i < strlen(filename) + 2; i++) { // + 2: header and null terminator
            atomic_write(SocketState.send_data_buf + i, filename[i - 1]);
        }
        struct turntable_queue_data *new_data = malloc(sizeof *new_data);
        new_data->filename = malloc(strlen(filename));
        strcpy(new_data->filename, filename);
        new_data->src = NULL;
        queue_push(SocketState.file_queue, new_data);
        atomic_write(&SocketState.send_data_ready, true);
        while (SocketState.send_data_ready)
            continue;
    } else {
        printf("Adding '%s' to queue...\n", filename);

        struct turntable_queue_data *new_data = malloc(sizeof *new_data);
        new_data->filename = malloc(strlen(filename));
        strcpy(new_data->filename, filename);
        new_data->src = malloc(strlen("localhost") + 1);
        strcpy(new_data->src, "localhost");
        queue_push(SocketState.file_queue, new_data);
    }
}

void turntable_pause_audio(void) {
    if (SocketState.socket_connected && SocketState.is_client) {
        atomic_write(SocketState.send_data_buf, PACKET_COMMAND << 5 | COMMAND_PAUSE);
        atomic_write(&SocketState.send_data_ready, true);
        while (SocketState.send_data_ready)
            continue;
    } else {
        atomic_write(&AudioState.flags.should_play, false);
    }
}

void turntable_resume_audio(void) {
    if (SocketState.socket_connected && SocketState.is_client) {
        atomic_write(SocketState.send_data_buf, PACKET_COMMAND << 5 | COMMAND_PLAY);
        atomic_write(&SocketState.send_data_ready, true);
        while (SocketState.send_data_ready)
            continue;
    } else {
        atomic_write(&AudioState.flags.should_play, true);
    }
}

void turntable_skip_audio(void) {
    if (SocketState.socket_connected && SocketState.is_client) {
        atomic_write(SocketState.send_data_buf, PACKET_COMMAND << 5 | COMMAND_SKIP);
        atomic_write(&SocketState.send_data_ready, true);
        while (SocketState.send_data_ready)
            continue;
    } else {
        atomic_write(&AudioState.flags.should_stop, true);
    }
}
