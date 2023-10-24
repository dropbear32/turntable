/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "socket.h"

#include "../audio/audio.h"
#include "../util/atomic.h"
#include "../util/file.h"
#include "../util/queue.h"
#include "client.h"
#include "server.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/socket.h>
#endif

struct file_queue_cleanse_data {
    char *filter;
    bool invert;
};

void file_queue_destroy_item(void *param) {
    struct turntable_queue_data *item = (struct turntable_queue_data *)param;
    free(item->filename);
    free(item->src);
    free(item);
}

int file_queue_cleanse(void *data, void *in) {
    struct turntable_queue_data *item = (struct turntable_queue_data *)data;
    struct file_queue_cleanse_data options = *(struct file_queue_cleanse_data *)in;
    if (!!strcmp(options.filter, item->src) == options.invert) {
        file_queue_destroy_item(item);
        return 1;
    }
    return 0;
}

void clear_queued_files(struct turntable_queue *queue, char *filter, bool invert_match) {
    struct file_queue_cleanse_data in = { filter, invert_match };
    queue_cleanse(queue, file_queue_cleanse, &in);
}

// TODO: This really should be using more complicated logic since the index width can be as high as 255.
size_t get_packet_index(uint8_t const *const packet) {
    size_t result = 0;
    for (int i = 0; i < packet[HEADER_LEN]; i++) {
        result |= (size_t)packet[HEADER_LEN + 1 + i] << (8 * i);
    }
    return result;
}

void inc_packet_index(uint8_t *const packet) {
    uint8_t width = packet[HEADER_LEN];
    for (int i = 0; i < width; i++) {
        if ((++packet[HEADER_LEN + 1 + i]) != 0)
            break;
    }
}

void parse_packet(char const *const char_data, ssize_t len, int sockfd, char const *const hostname) {
    uint8_t const *const data = (uint8_t *)char_data;

    if (len <= 0)
        return;

    uint8_t header = data[0];

    switch ((header & TYPE_MASK) >> 5) {
    case PACKET_COMMAND:
        if ((header & INFO_MASK) == COMMAND_PLAY) {
            AudioState.flags.should_play = true;
            break;
        }
        if ((header & INFO_MASK) == COMMAND_PAUSE) {
            AudioState.flags.should_play = false;
            break;
        }
        if ((header & INFO_MASK) == COMMAND_SKIP) {
            atomic_write(&AudioState.flags.should_stop, true);
            while (AudioState.flags.handler_thread_is_running)
                continue;
            break;
        }
        break;
    case PACKET_AUDIOFILE:
        if ((header & INFO_MASK) == AUDIOFILE_PLAY_REQ) {
            if (data[PACKET_SIZE - 1]) // file name has no null terminator
                return;
            struct turntable_queue_data *new_data = malloc(sizeof *new_data);
            new_data->filename = malloc(strlen((char *)data + HEADER_LEN) + 1);
            strcpy(new_data->filename, (char *)data + HEADER_LEN);
            new_data->src = malloc(strlen(hostname) + 1);
            strcpy(new_data->src, hostname);
            queue_push(SocketState.file_queue, new_data);
            atomic_write(&SocketState.file_queue->count, SocketState.file_queue->count);
            printf("%s added to queue\n", new_data->filename);
        }
        if ((header & INFO_MASK) == AUDIOFILE_FILE_REQ) {
            char const *const requested_file = (char *)data + HEADER_LEN;
            if (strcmp(((struct turntable_queue_data *)queue_peek(SocketState.file_queue))->filename, requested_file)) {
                fprintf(stderr,
                        "requested file '%s' was not next in queue, server is acting suspiciously. disconnect recommended.\n",
                        requested_file);
                fprintf(stderr, "next in queue was: %s\n", ((struct turntable_queue_data *)queue_peek(SocketState.file_queue))->filename);
                return;
            }
            printf("Sending file '%s'...\n", requested_file);
            queue_pop(SocketState.file_queue);
            size_t file_size = get_file_size(requested_file);
            if (file_size == 0) {
                printf("File is empty or does not exist\n");
                atomic_write(SocketState.send_data_buf, PACKET_ERROR << 5 | ERROR_FILE_MISSING);
                send_data(SocketState.send_data_buf, PACKET_SIZE, sockfd);
                return;
            }
            char *file_data = malloc(file_size);
            load_from_file(requested_file, file_data, file_size);

            unsigned int index_width = 1;
            while (true) {
                unsigned int const index_max = ((1 << (index_width * 8)) - 1);
                if (PACKET_SIZE - (HEADER_LEN + 1 + index_width) <= 0) {
                    fprintf(stderr, "your packet size is too small to transmit this file.\n");
                    atomic_write(SocketState.send_data_buf, PACKET_ERROR << 5 | ERROR_FILE_MISSING);
                    atomic_write(&SocketState.send_data_ready, true);
                    while (SocketState.send_data_ready)
                        continue;
                    return;
                }
                if (index_max < file_size / (PACKET_SIZE - (HEADER_LEN + 1 + index_width))) {
                    index_width++;
                } else {
                    break;
                }
            }

            unsigned int padding = 0;
            if (file_size % (PACKET_SIZE - (HEADER_LEN + 1 + index_width))) {
                padding = ((PACKET_SIZE - (HEADER_LEN + 1 + index_width)) - (file_size % (PACKET_SIZE - (HEADER_LEN + 1 + index_width))));
                printf("%u\n", padding);
                file_data = realloc(file_data, file_size + padding);
                memset(file_data + file_size, 0, padding);
            }

            uint8_t *buffer = malloc(PACKET_SIZE);
            size_t total_written = 0;
            size_t file_data_index = 0;

            buffer[HEADER_LEN] = index_width;
            for (size_t i = 0; i < index_width; i++) {
                buffer[HEADER_LEN + 1 + i] = 0;
            }

            while (file_data_index < file_size + padding) {
                size_t current_index = get_packet_index(buffer);
                if (current_index == 0) {
                    buffer[0] = PACKET_AUDIOFILE << 5 | AUDIOFILE_START;
                } else if (current_index == (file_size + padding) / (PACKET_SIZE - (HEADER_LEN + 1 + index_width)) - 1) {
                    buffer[0] = PACKET_AUDIOFILE << 5 | AUDIOFILE_END;
                } else {
                    buffer[0] = PACKET_AUDIOFILE << 5 | AUDIOFILE_MIDDLE;
                }
                memcpy(buffer + (HEADER_LEN + 1 + index_width), file_data + file_data_index, PACKET_SIZE - (HEADER_LEN + 1 + index_width));
                do {
                    ssize_t result =
                        send_data((char *)buffer + (total_written % PACKET_SIZE), PACKET_SIZE - (total_written % PACKET_SIZE), sockfd);
                    if (result < 0) {
                        perror("AUDIOFILE_FILE_REQ error");
                    }
                    total_written += result;
                } while (total_written % PACKET_SIZE != 0);
                inc_packet_index(buffer);
                file_data_index += PACKET_SIZE - (HEADER_LEN + 1 + index_width);
            }
            free(buffer);
            free(file_data);
        }
        if ((header & INFO_MASK) == AUDIOFILE_START) {
            AudioState.audio = malloc(sizeof *AudioState.audio);

            uint8_t index_width = data[HEADER_LEN];
            size_t audio_data_len = len - (HEADER_LEN + 1 + index_width);
            AudioState.audio->data = malloc(100 * SIZE_MB);
            AudioState.audio->buf_size = 100 * SIZE_MB;
            memcpy(AudioState.audio->data, data + (HEADER_LEN + 1 + index_width), audio_data_len);
            AudioState.audio->len = audio_data_len;
        }
        if ((header & INFO_MASK) == AUDIOFILE_MIDDLE || (header & INFO_MASK) == AUDIOFILE_END) {
            uint8_t index_width = data[HEADER_LEN];
            size_t audio_data_len = len - (HEADER_LEN + 1 + index_width);
            size_t index = get_packet_index(data);

            if (AudioState.audio->len + audio_data_len > AudioState.audio->buf_size) {
                AudioState.audio->data = realloc(AudioState.audio->data, AudioState.audio->buf_size *= 2);
            }

            memcpy(AudioState.audio->data + ((len - (HEADER_LEN + 1 + index_width)) * index),
                   data + (HEADER_LEN + 1 + index_width),
                   audio_data_len);
            AudioState.audio->len += audio_data_len;
            if ((header & INFO_MASK) == AUDIOFILE_END) {
                atomic_write(&AudioState.flags.should_play, true);
            }
        }
        break;
    case PACKET_ERROR:
        if ((header & INFO_MASK) == ERROR_FILE_MISSING) {
            AudioState.audio = NULL;
            atomic_write(&AudioState.flags.should_play, true);
        }
        break;
    }
}

ssize_t send_data(char const *const data, size_t len, int sockfd) {
    return send(sockfd, data, len, 0);
}

void *start_socket(void *param) {
    SocketState.socket_thread_is_running = true;
    SocketState.socket_connected = false;

    if (param == NULL) {
        SocketState.is_client = false;
        start_server();
    } else {
        SocketState.is_client = true;
        char *hostname = malloc(strlen(param) + 1);
        strcpy(hostname, param);
        start_client(hostname);
        free(hostname);
    }

    SocketState.socket_connected = false;
    SocketState.socket_thread_is_running = false;

    struct file_queue_cleanse_data in = { "localhost", true };
    queue_cleanse(SocketState.file_queue, file_queue_cleanse, &in);

    return NULL;
}
