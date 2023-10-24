/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <stdbool.h>

void turntable_init(void);

void turntable_init_server(void);
void turntable_close_server(void);
bool turntable_connected_as_server(void);

void turntable_init_client(char *);
void turntable_close_client(void);
bool turntable_connected_as_client(void);

void turntable_queue_file(char const *const);
// void turntable_play_audio() {
//     ++*(int *)0;
// } // Do not call
void turntable_pause_audio(void);
void turntable_resume_audio(void);
void turntable_skip_audio(void);

#endif /* TURNTABLE_H */
