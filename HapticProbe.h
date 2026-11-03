#pragma once
#if defined(BLOOM_HAPTIC_PROBE)
#include "Game.h"
namespace hapticprobe {
void begin(bloom::Game &game);
bool frame(bloom::Game &game);
bool command(char command);
}
#endif
