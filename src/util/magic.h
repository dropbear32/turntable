/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_MAGIC_H
#define TURNTABLE_MAGIC_H

#include <stddef.h>
#include <stdint.h>

// clang-format off
enum audio_file_type {
    WAV,
    FLAC,
    OGG,
    MP3,
    UNKNOWN
};
// clang-format on

enum audio_file_type get_file_type(uint8_t const *const, size_t const);

#endif
