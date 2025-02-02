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

AudioContext* audioInit(Arena* audioMem, Arena* tempMem) {
    (void)tempMem;

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

    AudioContext* audioCtx = push(audioMem, AudioContext);
    audioCtx->submittedFrameCount   = 0;
    audioCtx->submitAheadFrameCount = (usize)(0.066 * 44100);
    audioCtx->volumeLevel           = dbToAmplitudeMultiplier(0);
    audioCtx->playBackTime          = 0;
    audioCtx->audioMixToSubmit      = pushCount(audioMem, i16, 2*audioCtx->submitAheadFrameCount);

    hr = audioClient->Start();
    ASSERT(SUCCEEDED(hr));

    return audioCtx;
}

void fillAudioBuffer(AudioContext* audioCtx) {
    u32 framesToSubmitCount = audioCtx->submitAheadFrameCount - audioCtx->submittedFrameCount;
    if (framesToSubmitCount <= 0) {
        return;
    }

    HRESULT hr;
    u32 bufferPadding;
    hr = audioClient->GetCurrentPadding(&bufferPadding);
    ASSERT(SUCCEEDED(hr));
    u32 availableFrameCount = bufferFrameCount - bufferPadding;
    if (framesToSubmitCount > availableFrameCount) {
        framesToSubmitCount = availableFrameCount;
    }

    i16* buffer;
    hr = renderClient->GetBuffer(framesToSubmitCount, (BYTE**)&buffer);
    ASSERT(SUCCEEDED(hr));

    u32 submitFrameStart = audioCtx->submittedFrameCount;
    u32 submitFrameEnd   = audioCtx->submittedFrameCount + framesToSubmitCount;
    for (u32 frameIndex = submitFrameStart; frameIndex < submitFrameEnd; frameIndex++) {
        // assuming wave sample rate is 44100 HZ
        #if 1
        i16 yL = (i16)(audioCtx->volumeLevel * (float)audioCtx->audioMixToSubmit[2*frameIndex]);
        i16 yR = (i16)(audioCtx->volumeLevel * (float)audioCtx->audioMixToSubmit[2*frameIndex+1]);
        #else
        i16 yL = (i16)(audioCtx->volumeLevel * 30000 * sineWave(audioCtx->playBackTime, 440));
        i16 yR = (i16)(audioCtx->volumeLevel * 30000 * sineWave(audioCtx->playBackTime, 440));
        #endif

        *buffer++ = yL; // left
        *buffer++ = yR; // right

        audioCtx->playBackTime += 1.0/mixFormat.nSamplesPerSec;
    }
    hr = renderClient->ReleaseBuffer(framesToSubmitCount, 0);
    ASSERT(SUCCEEDED(hr));

    audioCtx->submittedFrameCount += framesToSubmitCount;
}

void audioDeinit(AudioContext* audioCtx) {
    (void)audioCtx;

    audioClient->Stop();

    renderClient->Release();
    audioClient->Release();
    device->Release();
    CoUninitialize();
}
