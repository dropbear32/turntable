/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_AUDIO_H
#define TURNTABLE_AUDIO_H

#include "../util/pthread.h"

#include <stdbool.h>

struct audio_data {
    char *data;
    size_t buf_size;
    size_t len;
};

struct audio_state {
    struct {
        bool should_play;
        bool should_stop;
        bool handler_thread_is_running;
        bool handler_thread_should_run;
    } flags;

    struct {
        pthread_t platform_audio;
        pthread_t audio_handler;
    } threads;

    struct audio_data *audio;
};

extern struct audio_data *audio;
extern struct audio_state AudioState;
void *audio_handler(void *);
void start_audio_thread(struct audio_data *data);
void *start_audio(void *);

#endif /* TURNTABLE_AUDIO_H */
