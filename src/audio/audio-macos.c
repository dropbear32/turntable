/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "audio-macos.h"

#include "../util/atomic.h"
#include "audio.h"

#include <AudioToolbox/AudioToolbox.h>
#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_AQ_NUMPACKETS 16384
#define DEFAULT_AQ_BUFSIZE    65536
#define AQBUF_COUNT           4

struct AudioQueueState {
    AudioFileID audioFile;
    SInt64 currentPacket;
    UInt32 const numPackets;
    AudioQueueBufferRef buffers[AQBUF_COUNT];
    UInt32 const bufferSize;
    bool isPlaying;
};

OSStatus read_data(void *inClientData, SInt64 inPosition, UInt32 requestCount, void *buffer, UInt32 *actualCount) {
    struct audio_data actualData = *(struct audio_data *)inClientData;
    size_t len = actualData.len;
    if (len < (uintmax_t)inPosition) {
        *actualCount = 0;
        // Apple's documentation is very unclear on what, if any, errors should be returned when.
        return noErr;
    }

    UInt32 possible = 0;
    if ((uintmax_t)(inPosition + requestCount) > len) {
        possible = len - inPosition;
    } else {
        possible = requestCount;
    }

    memcpy(buffer, actualData.data + inPosition, possible);
    *actualCount = possible;

    return noErr;
}

SInt64 get_size(void *inClientData) {
    return (*(struct audio_data *)inClientData).len;
}

void audioQueueCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    struct AudioQueueState *state = (struct AudioQueueState *)inUserData;

    UInt32 bytes = state->bufferSize;
    UInt32 numPackets = state->numPackets;
    AudioFileReadPacketData(state->audioFile, false, &bytes, NULL, state->currentPacket, &numPackets, inBuffer->mAudioData);

    if (numPackets > 0) {
        inBuffer->mAudioDataByteSize = bytes;
        inBuffer->mPacketDescriptionCount = numPackets;
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
        state->currentPacket += numPackets;
    } else {
        AudioQueueStop(inAQ, false);
    }
}

void start_platform_audio(void *param) {
    struct audio_data *audio = (struct audio_data *)param;
    if (audio == NULL || audio->data == NULL || audio->len == 0) {
        fprintf(stderr, "no data\n");
        return;
    }

    AudioFileID audioFile;
    AudioFileOpenWithCallbacks(audio, read_data, NULL, get_size, NULL, kAudioFileWAVEType, &audioFile);

    AudioStreamBasicDescription fileFormat;
    UInt32 fileFormatSize = sizeof(fileFormat);
    AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat, &fileFormatSize, &fileFormat);

    // Setup queue
    AudioQueueRef audioQueue;
    struct AudioQueueState audioQueueState = { audioFile, 0, DEFAULT_AQ_NUMPACKETS, { 0 }, DEFAULT_AQ_BUFSIZE, false };
    AudioQueueNewOutput(&fileFormat, audioQueueCallback, &audioQueueState, NULL, NULL, 0, &audioQueue);
    for (int i = 0; i < AQBUF_COUNT; i++) {
        AudioQueueAllocateBufferWithPacketDescriptions(audioQueue, audioQueueState.bufferSize, 0, &audioQueueState.buffers[i]);
    }
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_Volume, 1.0);
    // End setup queue

    // Start queue
    for (int i = 0; i < AQBUF_COUNT; i++) {
        audioQueueCallback(&audioQueueState, audioQueue, audioQueueState.buffers[i]);
    }
    // End start queue

    UInt32 audioQueueIsRunning = 1;
    UInt32 audioQueueHadStarted = 0;
    UInt32 audioQueueIsRunningSize = sizeof(audioQueueIsRunning);

    while (true) {
        if (AudioState.flags.should_play) {
            if (!audioQueueState.isPlaying) {
                AudioQueueStart(audioQueue, NULL);
                audioQueueState.isPlaying = true;
            }
        } else {
            if (audioQueueState.isPlaying) {
                AudioQueuePause(audioQueue);
                audioQueueState.isPlaying = false;
            }
        }

        AudioQueueGetProperty(audioQueue,
                              kAudioQueueProperty_IsRunning,
                              audioQueueHadStarted ? &audioQueueIsRunning : &audioQueueHadStarted,
                              &audioQueueIsRunningSize);

        if (AudioState.flags.should_stop || !audioQueueIsRunning) {
            break;
        }

        usleep(10 * 1000); // 10ms
    }

    AudioQueueStop(audioQueue, true);
    for (int i = 0; i < AQBUF_COUNT; i++) {
        AudioQueueFreeBuffer(audioQueue, audioQueueState.buffers[i]);
    }
    AudioQueueDispose(audioQueue, true);
    AudioFileClose(audioFile);
}
