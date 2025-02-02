#ifndef BREAKOUT_AUDIO_H_
#define BREAKOUT_AUDIO_H_

#include "base.h"

struct AudioContext {
    float volumeLevel;

    i16*  audioMixToSubmit;
    usize submittedFrameCount;
    usize submitAheadFrameCount;

    double playBackTime;
};

AudioContext* audioInit(Arena* audioMem, Arena* tempMem);
void audioDeinit(AudioContext* audioCtx);

struct AudioTrack {
    u32  sampleRate;
    u32  channelCount;
    i16* sampledData;
    u64  frameCount;
};


struct WaveHeader {
    u32 chunkId;   // "RIFF"
    u32 chunkSize; // size of whole file minus 8 bytes
    u32 waveId;    // "WAVE"
};

static constexpr u16 WAVE_FORMAT_PCM        = 0x0001; // PCM
static constexpr u16 WAVE_FORMAT_IEEE_FLOAT = 0x0003; // IEEE float
static constexpr u16 WAVE_FORMAT_ALAW       = 0x0006; // 8-bit ITU-T G.711 A-law
static constexpr u16 WAVE_FORMAT_MULAW      = 0x0007; // 8-bit ITU-T G.711 mu-law
static constexpr u16 WAVE_FORMAT_EXTENSIBLE = 0xffff; // determined by subFormat

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
    u32 chunkSize;   // n
    // sampledData
    // 1 padding byte if n is odd
};

void fillAudioBuffer(AudioContext* audioCtx);

static float dbToAmplitudeMultiplier(float db) {
    if (db > 10.0f) {
        db = 10.0f;
    } else if (db < -60.0f) {
        db = -60.0f;
    }
    return powf(10.f, db/20.f);
}

#define MAGICWORD(a, b, c, d) ((u32)(a&0xff)|((u32)(b&0xff)<<8)|((u32)(c&0xff)<<16)|((u32)(d&0xff)<<24))

static AudioTrack* readWaveFile(Arena* arena, Arena* scratch, const char* fileName) {
    FILE* file;
    if (fopen_s(&file, fileName, "rb") != 0) {
        LOG("Error opening %s\n", fileName);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8* buffer = (u8*)allocate(scratch, (usize)size, alignof(WaveHeader));
    fread(buffer, size, 1, file);

    WaveHeader* header = (WaveHeader*)buffer;
    ASSERT(header->chunkId     == MAGICWORD('R','I','F','F'));
    ASSERT(header->waveId      == MAGICWORD('W','A','V','E'));
    ASSERT(header->chunkSize+8 == (u32)size);

    WaveFmtChunk* fmtChunk = (WaveFmtChunk*)(buffer+sizeof(WaveHeader));
    ASSERT(fmtChunk->chunkId   == MAGICWORD('f','m','t',' '));
    ASSERT(fmtChunk->chunkSize == 16);
    ASSERT(fmtChunk->formatTag == WAVE_FORMAT_PCM);

    WaveDataChunk* dataChunk = (WaveDataChunk*)(buffer+sizeof(WaveHeader)+sizeof(WaveFmtChunk));
    ASSERT(dataChunk->chunkId  == MAGICWORD('d','a','t','a'));

    AudioTrack* track = (AudioTrack*)allocate(arena, sizeof(AudioTrack)+dataChunk->chunkSize, alignof(AudioTrack));
    *track = {};
    track->sampleRate   = fmtChunk->samplesPerSec;
    track->channelCount = fmtChunk->channels;
    track->sampledData  = (i16*)((u8*)track+sizeof(AudioTrack));
    memcpy(track->sampledData, (u8*)dataChunk+sizeof(WaveDataChunk), dataChunk->chunkSize);
    track->frameCount   = dataChunk->chunkSize / (2*fmtChunk->channels);

    fclose(file);
    return track;
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
