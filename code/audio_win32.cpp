#include "audio.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")

static IMMDevice*          device;
static IAudioClient*       audioClient;
static IAudioRenderClient* renderClient;

static u32 bufferFrameCount;
static u64 bufferDurationInSeconds;
static WAVEFORMATEX mixFormat;

static constexpr i64 REFTIMES_PER_SEC          = 10000000;
static constexpr i64 REQUESTED_BUFFER_DURATION = 2*REFTIMES_PER_SEC;

void audioInit() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    HRESULT hr;
    
    IMMDeviceEnumerator* enumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    ASSERT(SUCCEEDED(hr));

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    ASSERT(SUCCEEDED(hr));
    
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 
                          NULL, (void**)&audioClient);
    ASSERT(SUCCEEDED(hr));

    mixFormat.wFormatTag      = WAVE_FORMAT_PCM;
    mixFormat.nChannels       = 2;
    mixFormat.nSamplesPerSec  = 44100;
    mixFormat.wBitsPerSample  = 16;
    mixFormat.nBlockAlign     = (mixFormat.nChannels * mixFormat.wBitsPerSample)/8;
    mixFormat.nAvgBytesPerSec = mixFormat.nSamplesPerSec * mixFormat.nBlockAlign;

    DWORD initStreamFlags = AUDCLNT_STREAMFLAGS_RATEADJUST |
                            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                            AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, initStreamFlags, 
                                 REQUESTED_BUFFER_DURATION, 0, 
                                 &mixFormat, NULL);
    ASSERT(SUCCEEDED(hr));

    hr = audioClient->GetBufferSize(&bufferFrameCount);
    ASSERT(SUCCEEDED(hr));

    hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient);
    ASSERT(SUCCEEDED(hr));

    bufferDurationInSeconds = (double)REFTIMES_PER_SEC * 
                              bufferFrameCount / mixFormat.nSamplesPerSec;


    Arena backing = {};
    backing.capacity = (usize)16<<20;
    backing.memory   = (u8*)VirtualAlloc(NULL, backing.capacity, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    ASSERT(backing.memory != NULL);
    Arena permanent = {};
    permanent.capacity = (usize)8<<20;
    permanent.memory   = (u8*)allocate(&backing, permanent.capacity);
    ASSERT(permanent.memory != NULL);
    Arena scratch = {};
    scratch.capacity = (usize)4<<20;
    scratch.memory   = (u8*)allocate(&backing, scratch.capacity);
    ASSERT(scratch.memory != NULL);
    AudioTrack* woohAudio = readWaveFile(&permanent, &scratch, "data/sounds/wooh.wav");


    hr = audioClient->Start();
    ASSERT(SUCCEEDED(hr));

    constexpr float   TONE_HZ = 440;
    constexpr int16_t PCM_VOLUME_NORMALIZE_FACTOR = 32000;
    double playBackTime = 0.0;

    u32 audioTrackFrame = 0;
    while (true) {
        u32 bufferPadding;
        hr = audioClient->GetCurrentPadding(&bufferPadding);
        ASSERT(SUCCEEDED(hr));
        u32 frameCount = bufferFrameCount - bufferPadding;

        i16* buffer;
        hr = renderClient->GetBuffer(frameCount, (BYTE**)&buffer);
        ASSERT(SUCCEEDED(hr));

        // db          = 20*log_10(v)
        // volumeLevel = 10^(db/20)
        // good db range [-80,0]
        float db          = -16;
        float volumeLevel = powf(10.f, db/20.f);
        (void)volumeLevel;
        (void)TONE_HZ;
        (void)PCM_VOLUME_NORMALIZE_FACTOR;
        (void)playBackTime;

        for (u32 frameIndex = 0; frameIndex < frameCount; frameIndex++) {
            #if 0
            float amplitudeL = sineWave(playBackTime, TONE_HZ);
            float amplitudeR = sineWave(playBackTime, TONE_HZ);
            i16 yL = (i16)(volumeLevel * PCM_VOLUME_NORMALIZE_FACTOR * amplitudeL);
            i16 yR = (i16)(volumeLevel * PCM_VOLUME_NORMALIZE_FACTOR * amplitudeR);
            #endif
            // assuming wave sample rate is 44100 HZ
            i16 yL = woohAudio->sampledData[audioTrackFrame++];
            i16 yR = woohAudio->sampledData[audioTrackFrame++];

            if (audioTrackFrame >= woohAudio->size/2) {
                audioTrackFrame -= woohAudio->size/2;
            }

            *buffer++ = yL; // left
            *buffer++ = yR; // right

            playBackTime += 1.0/mixFormat.nSamplesPerSec;
        }
        hr = renderClient->ReleaseBuffer(frameCount, 0);
        ASSERT(SUCCEEDED(hr));

        Sleep(500);
    }
}

void audioDeinit() {

    audioClient->Stop();

    renderClient->Release();
    audioClient->Release();
    device->Release();
    CoUninitialize();
}
