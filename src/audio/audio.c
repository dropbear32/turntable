/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "audio.h"

#if defined(__linux__)
#include "audio-linux.h"
#elif defined(__APPLE__)
#include "audio-macos.h"
#elif defined(_WIN32)
#include "audio-windows.h"
#endif

#include "../socket/socket.h"
#include "../util/atomic.h"
#include "../util/file.h"
#include "../util/magic.h"
#include "../util/queue.h"
#include "../util/sleep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *audio_handler(void *param) {
    (void)param;

    if (SocketState.file_queue == NULL) {
        SocketState.file_queue = malloc(sizeof *SocketState.file_queue);
        SocketState.file_queue->data = malloc(sizeof *SocketState.file_queue->data);
        SocketState.file_queue->count = 0;
        SocketState.file_queue->max = 1;
    }

    while (AudioState.flags.handler_thread_should_run) {
        if (!AudioState.flags.handler_thread_is_running && SocketState.file_queue->count > 0 &&
            !(SocketState.socket_connected && SocketState.is_client)) {
            struct turntable_queue_data *front_elem = queue_pop(SocketState.file_queue);
            if (!strcmp(front_elem->src, "localhost")) {
                size_t file_size = get_file_size(front_elem->filename);
                if (file_size > 2 * SIZE_GB) {
                    printf("File too large (filesize limit is 2GB)\n");
                    continue;
                }

                if (file_size == 0) {
                    printf("File does not exist or is empty\n");
                    continue;
                }

                char *file_data = malloc(file_size);
                load_from_file(front_elem->filename, file_data, file_size);

                enum audio_file_type type = get_file_type((uint8_t *)file_data, file_size);
                if (type != WAV) {
                    printf("File is not a RIFF/WAV file; cannot play\n");
                    continue;
                }

                AudioState.audio = malloc(sizeof *AudioState.audio);
                AudioState.audio->data = file_data;
                AudioState.audio->len = file_size;
                printf("Playing %s...\n", front_elem->filename);
                start_audio_thread(AudioState.audio);
                atomic_write(&AudioState.flags.should_play, true);
            } else {
                printf("Requesting %s...\n", front_elem->filename);
                char *packet = malloc(PACKET_SIZE);
                packet[0] = PACKET_AUDIOFILE << 5 | AUDIOFILE_FILE_REQ;
                strcpy(packet + HEADER_LEN, front_elem->filename);
                for (size_t i = 0; i < PACKET_SIZE; i++) {
                    atomic_write(SocketState.send_data_buf + i, packet[i]);
                }
                atomic_write(&SocketState.send_data_ready, true);
                while (SocketState.send_data_ready)
                    continue;
                free(packet);
                while (!AudioState.flags.should_play)
                    msleep(10);
                printf("Playing %s...\n", front_elem->filename);
                start_audio_thread(AudioState.audio);
            }
        }
        msleep(10);
    }

    return NULL;
}

void start_audio_thread(struct audio_data *data) {
    pthread_create(&AudioState.threads.platform_audio, NULL, start_audio, data);
}

void *start_audio(void *param) {
    atomic_write(&AudioState.flags.handler_thread_is_running, true);
    struct audio_data *audio = (struct audio_data *)param;
    if (audio == NULL || audio->data == NULL || audio->len == 0) {
        fprintf(stderr, "no data\n");
    } else {
        start_platform_audio(param);
        free(((struct audio_data *)param)->data);
        free(param);
    }
    atomic_write(&AudioState.flags.should_stop, false);
    atomic_write(&AudioState.flags.should_play, false);
    atomic_write(&AudioState.flags.handler_thread_is_running, false);
    return NULL;
}
