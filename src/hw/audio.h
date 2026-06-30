#pragma once
#include <stdint.h>

bool hwAudioInit();
void hwBeep(uint16_t freqHz, uint16_t durMs);
void hwAudioSetVolume(uint8_t level);   // 0=mute .. 4=loud
