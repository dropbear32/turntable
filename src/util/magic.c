/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "magic.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// clang-format off
uint8_t const wav_header[]         = "RIFF????WAVE";
uint8_t const flac_header[]        = "fLaC";
uint8_t const mp3_header_id3v1_a[] = "\xFF\xFB";
uint8_t const mp3_header_id3v1_b[] = "\xFF\xF3";
uint8_t const mp3_header_id3v1_c[] = "\xFF\xF2";
uint8_t const mp3_header_id3v2[]   = "ID3";
uint8_t const ogg_header[]         = "OggS";
// clang-format on

enum audio_file_type get_file_type(uint8_t const *const header, size_t const len) {
    if (len == 0 || header == NULL) {
        return UNKNOWN;
    }

    if (len >= 2) {
        if (memcmp(header, mp3_header_id3v1_a, 2) == 0)
            return MP3;
        if (memcmp(header, mp3_header_id3v1_b, 2) == 0)
            return MP3;
        if (memcmp(header, mp3_header_id3v1_c, 2) == 0)
            return MP3;
    }

    if (len >= 3) {
        if (memcmp(header, mp3_header_id3v2, 3) == 0)
            return MP3;
    }

    if (len >= 4) {
        if (memcmp(header, flac_header, 4) == 0)
            return FLAC;
        if (memcmp(header, ogg_header, 4) == 0)
            return OGG;
    }

    if (len >= 12) {
        if (memcmp(header, wav_header, 4) == 0 && memcmp(header + 8, wav_header + 8, 4) == 0)
            return WAV;
    }

    return UNKNOWN;
}
