/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "audio-windows.h"

#include "../util/atomic.h"
#include "../util/sleep.h"
#include "audio.h"

#include <mmreg.h>
#include <stdio.h>

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    (void)hwo;
    (void)dwInstance;
    (void)dwParam1;
    (void)dwParam2;

    switch (uMsg) {
    case WOM_DONE:
        atomic_write(&AudioState.flags.should_stop, true);
        break;
    default:
        break;
    }
}

void start_platform_audio(void *param) {
    struct audio_data *audio = (struct audio_data *)param;

    HWAVEOUT handle;
    WAVEFORMATEXTENSIBLE format;
    memcpy(&format, audio->data + 20, sizeof(format.Format));
    if (format.Format.wBitsPerSample <= 16) {
        format.Format.wFormatTag = WAVE_FORMAT_PCM;
    } else {
        format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    }
    format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    waveOutOpen(&handle, WAVE_MAPPER, (WAVEFORMATEX *)&format, (DWORD_PTR)waveOutProc, 0, WAVE_FORMAT_DIRECT | CALLBACK_FUNCTION);

    WAVEHDR wh;
    wh.lpData = audio->data;
    wh.dwBufferLength = audio->len;
    wh.dwLoops = 1;
    wh.dwFlags = 0;
    waveOutPrepareHeader(handle, &wh, sizeof(WAVEHDR));
    waveOutPause(handle);
    waveOutWrite(handle, &wh, sizeof(WAVEHDR));

    bool audioIsPlaying = false;
    while (true) {
        if (AudioState.flags.should_play) {
            if (!audioIsPlaying) {
                waveOutRestart(handle);
                audioIsPlaying = true;
            }
        } else {
            if (audioIsPlaying) {
                waveOutPause(handle);
                audioIsPlaying = false;
            }
        }

        if (AudioState.flags.should_stop) {
            break;
        }

        msleep(10);
    }

    waveOutReset(handle);
    waveOutUnprepareHeader(handle, &wh, sizeof(WAVEHDR));
    waveOutClose(handle);
}
