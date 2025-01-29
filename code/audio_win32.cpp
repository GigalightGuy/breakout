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

static constexpr i64 REFTIMES_PER_SEC      = 10000000;

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

    hr = audioClient->Start();
    ASSERT(SUCCEEDED(hr));

    constexpr float   TONE_HZ = 440;
    constexpr int16_t TONE_VOLUME = 3000;
    double playBackTime = 0.0;

    while (true) {
        u32 bufferPadding;
        hr = audioClient->GetCurrentPadding(&bufferPadding);
        ASSERT(SUCCEEDED(hr));
        u32 frameCount = bufferFrameCount - bufferPadding;

        i16* buffer;
        hr = renderClient->GetBuffer(frameCount, (BYTE**)&buffer);
        ASSERT(SUCCEEDED(hr));

        for (u32 frameIndex = 0; frameIndex < frameCount; frameIndex++) {
            float amplitudeL = sinf(playBackTime * F_TAU*TONE_HZ);
            //float amplitudeR = sinf(playBackTime * F_TAU*(TONE_HZ + 0.5f*TONE_HZ * sinf(F_TAU * playBackTime)));
            float amplitudeR = sawtooth(playBackTime, 220);
            int16_t yL = (int16_t)(0 * amplitudeL);
            int16_t yR = (int16_t)(TONE_VOLUME * amplitudeR);

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
