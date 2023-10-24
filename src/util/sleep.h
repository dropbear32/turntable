/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_SLEEP_H
#define TURNTABLE_SLEEP_H

#ifdef _WIN32
#include <Windows.h>
#define msleep(ms) Sleep(ms);
#else
#include <unistd.h>
#define msleep(ms) usleep(ms * 1000);
#endif

#endif /* TURNTABLE_SLEEP_H */
