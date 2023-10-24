/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "audio-linux.h"

#include "../util/sleep.h"
#include "audio.h"

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
typedef enum {
    WAV_FMT_PCM = 0x0001,
    WAV_FMT_IEEE_FLOAT = 0x0003,
    WAV_FMT_DOLBY_AC3_SPDIF = 0x0092,
    WAV_FMT_EXTENSIBLE = 0xfffe
} WAVE_FMT;
// clang-format on

typedef struct {
    uint32_t riff;
    uint32_t total_length;
    uint32_t wave;
    uint32_t fmt;
    uint32_t format_length;
    uint16_t format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t bytes_per_sample;
    uint16_t bits_per_sample;
    uint32_t chunk_tag;
    uint32_t chunk_size;
} WAVEFORMATEX;

void start_platform_audio(void *param) {
    struct audio_data *audio = (struct audio_data *)param;
    if (audio == NULL || audio->data == NULL || audio->len == 0) {
        fprintf(stderr, "no data\n");
        return;
    }

    WAVEFORMATEX format;
    memcpy(&format, audio->data, sizeof(format));
    size_t offset = 36; // Start of chunks
    while (memcmp(audio->data + offset, (char[]) { 'd', 'a', 't', 'a' }, 4)) {
        offset++;
        if (offset > audio->len) {
            printf("Malformed WAV file: no data chunk\n");
            return;
        }
    }
    format.chunk_size = 0;
    for (int i = 0; i < 4; i++)
        format.chunk_size += *(unsigned char *)(audio->data + offset + 4 + i) << (8 * i);
    char *audio_data = audio->data + offset + 8;

    snd_pcm_format_t pcmFormat;
    switch (format.bits_per_sample) {
    case 8:
        pcmFormat = SND_PCM_FORMAT_U8;
        break;
    case 16:
        pcmFormat = SND_PCM_FORMAT_S16_LE;
        break;
    case 24:
        if (format.bytes_per_sample / format.num_channels == 3)
            pcmFormat = SND_PCM_FORMAT_S24_3LE;
        else
            pcmFormat = SND_PCM_FORMAT_S24_LE;
        break;
    case 32:
        if (format.fmt == WAV_FMT_IEEE_FLOAT)
            pcmFormat = SND_PCM_FORMAT_FLOAT_LE;
        else
            pcmFormat = SND_PCM_FORMAT_S32_LE;
        break;
    default:
        pcmFormat = SND_PCM_FORMAT_UNKNOWN;
        break;
    }

    int err;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_malloc(&hw_params);

    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Error opening handle: %s\n", snd_strerror(err));
        return;
    }

    if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
        printf("error snd_pcm_hw_params_any: %s\n", snd_strerror(err));
        return;
    }

    if ((err = snd_pcm_hw_params_set_rate(handle, hw_params, format.sample_rate, 0)) < 0) {
        printf("error snd_pcm_hw_params_set_rate: %s\n", snd_strerror(err));
        return;
    }
    if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, format.num_channels)) < 0) {
        printf("error snd_pcm_hw_params_set_channels: %s\n", snd_strerror(err));
        return;
    }
    if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("error snd_pcm_hw_params_set_access: %s\n", snd_strerror(err));
        return;
    }
    if ((err = snd_pcm_hw_params_set_format(handle, hw_params, pcmFormat)) < 0) {
        printf("error snd_pcm_hw_params_set_format: %s\n", snd_strerror(err));
        return;
    }

    snd_pcm_uframes_t period_size = format.sample_rate / 8; // Constant here determines how many writes/sec happen
                                                            // and consequently how fast pausing happens.
    snd_pcm_uframes_t buffer_size = period_size * format.num_channels * format.bits_per_sample / 8;

    if ((err = snd_pcm_hw_params_set_buffer_size(handle, hw_params, buffer_size)) < 0) {
        printf("error snd_pcm_hw_params_set_buffer_size: %s\n", snd_strerror(err));
        return;
    }
    if ((err = snd_pcm_hw_params_set_period_size(handle, hw_params, period_size, 0)) < 0) {
        printf("error snd_pcm_hw_params_set_period_size: %s\n", snd_strerror(err));
        return;
    }

    if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
        printf("error snd_pcm_hw_params: %s\n", snd_strerror(err));
        return;
    }

    bool audio_is_playing = false;
    snd_pcm_uframes_t written = 0;
    snd_pcm_uframes_t total_frames = format.chunk_size / (format.num_channels * format.bits_per_sample / 8);
    while (written < total_frames) {
        if (AudioState.flags.should_play) {
            if (!audio_is_playing) {
                snd_pcm_pause(handle, 0);
                audio_is_playing = true;
            }
        } else {
            if (audio_is_playing) {
                snd_pcm_pause(handle, 1);
                audio_is_playing = false;
            }
            msleep(10);
        }
        if (AudioState.flags.should_stop)
            break;
        snd_pcm_uframes_t safe_size = ((total_frames - written) > period_size ? period_size : (total_frames - written));
        err = snd_pcm_writei(handle, audio_data + written * format.bytes_per_sample, safe_size);
        if (err < 0) {
            if ((err = snd_pcm_recover(handle, err, 1)) == 0) {
                printf("recovered after xrun (overrun/underrun)\n");
            } else {
                printf("error snd_pcm_recover: %s\n", snd_strerror(err));
                break;
            }
        } else {
            written += err;
        }
    }
    if (AudioState.flags.should_stop)
        snd_pcm_drop(handle);
    else
        snd_pcm_drain(handle);
    snd_pcm_close(handle);
}
