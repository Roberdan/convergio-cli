/**
 * CONVERGIO EDUCATION - VOICE AUDIO (CoreAudio)
 *
 * Audio capture and playback using macOS CoreAudio.
 * No external dependencies - uses built-in frameworks.
 *
 * ADR: ADR-003-voice-cli-conversational-ux.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifdef CONVERGIO_VOICE_ENABLED

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include <pthread.h>
#include <stdbool.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define VOICE_SAMPLE_RATE       24000.0
#define VOICE_CHANNELS          1
#define VOICE_BITS_PER_SAMPLE   16
#define VOICE_BUFFER_SIZE       4800    // 100ms at 24kHz
#define VOICE_NUM_BUFFERS       3

// ============================================================================
// TYPES
// ============================================================================

typedef void (*VoiceAudioCallback)(const int16_t *samples, size_t count, void *user_data);

typedef struct {
    // Capture (microphone)
    AudioQueueRef capture_queue;
    AudioQueueBufferRef capture_buffers[VOICE_NUM_BUFFERS];
    bool capturing;

    // Playback (speaker)
    AudioQueueRef playback_queue;
    AudioQueueBufferRef playback_buffers[VOICE_NUM_BUFFERS];
    bool playing;

    // Ring buffer for playback
    int16_t *playback_ring;
    size_t ring_size;
    size_t ring_read;
    size_t ring_write;
    pthread_mutex_t ring_mutex;

    // Callbacks
    VoiceAudioCallback on_audio_captured;
    void *capture_user_data;

    // State
    bool initialized;

} VoiceAudio;

static VoiceAudio g_audio = {0};

// Forward declarations
void voice_audio_stop_capture(void);
void voice_audio_stop_playback(void);

// ============================================================================
// AUDIO FORMAT
// ============================================================================

static AudioStreamBasicDescription voice_audio_format(void) {
    AudioStreamBasicDescription format = {0};
    format.mSampleRate = VOICE_SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = VOICE_BITS_PER_SAMPLE;
    format.mChannelsPerFrame = VOICE_CHANNELS;
    format.mBytesPerFrame = (VOICE_BITS_PER_SAMPLE / 8) * VOICE_CHANNELS;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame;
    return format;
}

// ============================================================================
// CAPTURE CALLBACK
// ============================================================================

static void capture_callback(void *user_data,
                             AudioQueueRef queue,
                             AudioQueueBufferRef buffer,
                             const AudioTimeStamp *start_time,
                             UInt32 num_packets,
                             const AudioStreamPacketDescription *packet_descs) {
    VoiceAudio *audio = (VoiceAudio *)user_data;

    if (audio->on_audio_captured && buffer->mAudioDataByteSize > 0) {
        size_t sample_count = buffer->mAudioDataByteSize / sizeof(int16_t);
        audio->on_audio_captured((int16_t *)buffer->mAudioData, sample_count, audio->capture_user_data);
    }

    // Re-enqueue buffer for more recording
    if (audio->capturing) {
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    }
}

// ============================================================================
// PLAYBACK CALLBACK
// ============================================================================

static void playback_callback(void *user_data,
                              AudioQueueRef queue,
                              AudioQueueBufferRef buffer) {
    VoiceAudio *audio = (VoiceAudio *)user_data;

    pthread_mutex_lock(&audio->ring_mutex);

    size_t frames_needed = buffer->mAudioDataBytesCapacity / sizeof(int16_t);
    size_t frames_available = (audio->ring_write >= audio->ring_read)
        ? (audio->ring_write - audio->ring_read)
        : (audio->ring_size - audio->ring_read + audio->ring_write);

    size_t frames_to_copy = (frames_available < frames_needed) ? frames_available : frames_needed;

    int16_t *output = (int16_t *)buffer->mAudioData;

    for (size_t i = 0; i < frames_to_copy; i++) {
        output[i] = audio->playback_ring[audio->ring_read];
        audio->ring_read = (audio->ring_read + 1) % audio->ring_size;
    }

    // Fill remaining with silence
    for (size_t i = frames_to_copy; i < frames_needed; i++) {
        output[i] = 0;
    }

    pthread_mutex_unlock(&audio->ring_mutex);

    buffer->mAudioDataByteSize = (UInt32)(frames_needed * sizeof(int16_t));

    if (audio->playing) {
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool voice_audio_init(void) {
    if (g_audio.initialized) return true;

    // Request microphone permission
    @autoreleasepool {
        if (@available(macOS 10.14, *)) {
            AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
            if (status == AVAuthorizationStatusNotDetermined) {
                dispatch_semaphore_t sema = dispatch_semaphore_create(0);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                    dispatch_semaphore_signal(sema);
                }];
                dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
            }
        }
    }

    // Initialize ring buffer for playback
    g_audio.ring_size = VOICE_SAMPLE_RATE * 5;  // 5 seconds buffer
    g_audio.playback_ring = calloc(g_audio.ring_size, sizeof(int16_t));
    if (!g_audio.playback_ring) return false;

    pthread_mutex_init(&g_audio.ring_mutex, NULL);

    g_audio.initialized = true;
    return true;
}

void voice_audio_cleanup(void) {
    if (!g_audio.initialized) return;

    voice_audio_stop_capture();
    voice_audio_stop_playback();

    pthread_mutex_destroy(&g_audio.ring_mutex);
    free(g_audio.playback_ring);

    g_audio.initialized = false;
}

bool voice_audio_start_capture(VoiceAudioCallback callback, void *user_data) {
    if (!g_audio.initialized || g_audio.capturing) return false;

    g_audio.on_audio_captured = callback;
    g_audio.capture_user_data = user_data;

    AudioStreamBasicDescription format = voice_audio_format();

    OSStatus status = AudioQueueNewInput(&format,
                                         capture_callback,
                                         &g_audio,
                                         NULL,
                                         kCFRunLoopCommonModes,
                                         0,
                                         &g_audio.capture_queue);

    if (status != noErr) {
        fprintf(stderr, "[Voice Audio] Failed to create capture queue: %d\n", (int)status);
        return false;
    }

    // Allocate and enqueue buffers
    UInt32 buffer_size = VOICE_BUFFER_SIZE * sizeof(int16_t);
    for (int i = 0; i < VOICE_NUM_BUFFERS; i++) {
        status = AudioQueueAllocateBuffer(g_audio.capture_queue, buffer_size, &g_audio.capture_buffers[i]);
        if (status != noErr) {
            fprintf(stderr, "[Voice Audio] Failed to allocate capture buffer: %d\n", (int)status);
            return false;
        }
        AudioQueueEnqueueBuffer(g_audio.capture_queue, g_audio.capture_buffers[i], 0, NULL);
    }

    g_audio.capturing = true;
    status = AudioQueueStart(g_audio.capture_queue, NULL);
    if (status != noErr) {
        fprintf(stderr, "[Voice Audio] Failed to start capture: %d\n", (int)status);
        g_audio.capturing = false;
        return false;
    }

    return true;
}

void voice_audio_stop_capture(void) {
    if (!g_audio.capturing) return;

    g_audio.capturing = false;
    AudioQueueStop(g_audio.capture_queue, true);
    AudioQueueDispose(g_audio.capture_queue, true);
    g_audio.capture_queue = NULL;
}

bool voice_audio_start_playback(void) {
    if (!g_audio.initialized || g_audio.playing) return false;

    AudioStreamBasicDescription format = voice_audio_format();

    OSStatus status = AudioQueueNewOutput(&format,
                                          playback_callback,
                                          &g_audio,
                                          NULL,
                                          kCFRunLoopCommonModes,
                                          0,
                                          &g_audio.playback_queue);

    if (status != noErr) {
        fprintf(stderr, "[Voice Audio] Failed to create playback queue: %d\n", (int)status);
        return false;
    }

    // Allocate buffers
    UInt32 buffer_size = VOICE_BUFFER_SIZE * sizeof(int16_t);
    for (int i = 0; i < VOICE_NUM_BUFFERS; i++) {
        status = AudioQueueAllocateBuffer(g_audio.playback_queue, buffer_size, &g_audio.playback_buffers[i]);
        if (status != noErr) {
            fprintf(stderr, "[Voice Audio] Failed to allocate playback buffer: %d\n", (int)status);
            return false;
        }
        // Prime with silence
        memset(g_audio.playback_buffers[i]->mAudioData, 0, buffer_size);
        g_audio.playback_buffers[i]->mAudioDataByteSize = buffer_size;
        AudioQueueEnqueueBuffer(g_audio.playback_queue, g_audio.playback_buffers[i], 0, NULL);
    }

    g_audio.playing = true;
    status = AudioQueueStart(g_audio.playback_queue, NULL);
    if (status != noErr) {
        fprintf(stderr, "[Voice Audio] Failed to start playback: %d\n", (int)status);
        g_audio.playing = false;
        return false;
    }

    return true;
}

void voice_audio_stop_playback(void) {
    if (!g_audio.playing) return;

    g_audio.playing = false;
    AudioQueueStop(g_audio.playback_queue, true);
    AudioQueueDispose(g_audio.playback_queue, true);
    g_audio.playback_queue = NULL;

    // Clear ring buffer
    pthread_mutex_lock(&g_audio.ring_mutex);
    g_audio.ring_read = 0;
    g_audio.ring_write = 0;
    pthread_mutex_unlock(&g_audio.ring_mutex);
}

void voice_audio_play(const int16_t *samples, size_t count) {
    if (!g_audio.playing || !samples || count == 0) return;

    pthread_mutex_lock(&g_audio.ring_mutex);

    for (size_t i = 0; i < count; i++) {
        size_t next_write = (g_audio.ring_write + 1) % g_audio.ring_size;
        if (next_write == g_audio.ring_read) {
            // Buffer full, skip oldest sample
            g_audio.ring_read = (g_audio.ring_read + 1) % g_audio.ring_size;
        }
        g_audio.playback_ring[g_audio.ring_write] = samples[i];
        g_audio.ring_write = next_write;
    }

    pthread_mutex_unlock(&g_audio.ring_mutex);
}

void voice_audio_clear_playback(void) {
    pthread_mutex_lock(&g_audio.ring_mutex);
    g_audio.ring_read = 0;
    g_audio.ring_write = 0;
    pthread_mutex_unlock(&g_audio.ring_mutex);
}

bool voice_audio_is_capturing(void) {
    return g_audio.capturing;
}

bool voice_audio_is_playing(void) {
    return g_audio.playing;
}

#endif /* CONVERGIO_VOICE_ENABLED */
