#ifndef BREAKOUT_AUDIO_H_
#define BREAKOUT_AUDIO_H_

#include "base.h"

void audioInit();
void audioDeinit();

struct WaveHeader {
    u32 chunkId;   // "RIFF"
    u32 chunkSize; // size of whole file minus 8 bytes
    u32 waveId;    // "WAVE"
};

/*
 * 0x0001 WAVE_FORMAT_PCM         - PCM
 * 0x0003 WAVE_FORMAT_IEEE_FLOAT  - IEEE float
 * 0x0006 WAVE_FORMAT_ALAW        - 8-bit ITU-T G.711 A-law
 * 0x0007 WAVE_FORMAT_MULAW       - 8-bit ITU-T G.711 mu-law
 * 0xffff WAVE_FORMAT_EXTENSIBLE  - determined by subFormat
 */
struct WaveFmtChunk {
    u32 chunkId;            // "fmt "
    u32 chunkSize;          // 16, 18 or 40
    u16 formatTag;
    u16 channels;
    u32 samplesPerSec;
    u32 avgBytesPerSec;
    u16 blockAlign;
    u16 bitsPerSample;

    // extension fields
    //u16 cbSize;
    //u16 validBitsPerSample;
    //u32 channelMask;
    //u8  subFormat[16];
};

struct WaveDataChunk {
    u32 chunkId;     // "data"
    u32 chunkSize;
    u8* sampledData;
    // Has 1 padding byte if n is odd
};

b32 readWaveFile(const char* fileName) {
    // TODO(pedro s.): read PCM chunk data
    // Probably start off only accepting PCM 16-bit for now
    return true;
}

static float sawtoothWave(float t, float f) {
    float amplitude = 2.0f*(t*f - floorf(0.5f + t*f));
    return amplitude;
}

static float sineWave(float t, float f) {
    float amplitude = sinf(t*f * F_TAU);
    return amplitude;
}

#endif // BREAKOUT_AUDIO_H_
