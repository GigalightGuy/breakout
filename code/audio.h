#ifndef BREAKOUT_AUDIO_H_
#define BREAKOUT_AUDIO_H_

#include "base.h"

void audioInit();
void audioDeinit();

static float sawtooth(float t, float f) {
    float amplitude = 2.0f*(t*f - floorf(0.5f + t*f));
    return amplitude;
}


#endif // BREAKOUT_AUDIO_H_
