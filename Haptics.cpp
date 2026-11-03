#include <Arduino.h>
#include "Haptics.h"
#include "src/rumble/Rumble.h"

namespace haptics {
static uint32_t lastKill;
static bool bombActive;
void begin() {
    rumble::begin();lastKill=millis()-90;bombActive=false;
}
void update(const bloom::Game &game) {
    using namespace bloom;
    // A final hit can finish its short pulse on the result screen.
    // Menus and pause always cut the output immediately.
    const bool enabled=game.screen==Play || game.screen==Evolution || game.screen==Over;
    rumble::setEnabled(enabled);rumble::update();
    if(!enabled) {bombActive=false;return;}
    if(!game.bombFlash && bombActive) {
        // Respect a higher-priority injury pulse when the blast ends.
        rumble::pulse(0,0,3);bombActive=false;
    }
    const unsigned events=game.events;
    if(events&Hurt) {rumble::pulse(255,80,4);return;}
    if(game.bombFlash) {
        // Two broad ripples with a full 167 ms coast-down gap.
        // The visual counter is the clock: pause and evolution stay in sync.
        static const uint8_t envelope[30]={
            255,255,235,210,175,130,80,
            0,0,0,0,0,0,0,0,0,0,
            235,235,215,190,160,130,100,80,65,50,35,20,10
        };
        unsigned frame=game.bombFlash>=BOMB_FRAMES?0:(BOMB_FRAMES-game.bombFlash)*30/BOMB_FRAMES;
        bombActive=true;
        // A stalled renderer cannot leave the motor running. Zero-strength
        // troughs use the priority check too, so they cannot cancel damage.
        rumble::pulse(envelope[frame],35,3);return;
    }
    if(events&BossDown)rumble::pulse(240,240,3);
    else if(events&Evolve)rumble::pulse(180,110,2);
    else if(events&Clear)rumble::pulse(155,70,1);
    else if(events&GunSwitch)rumble::pulse(105,35,1);
    else if((events&Kill) && uint32_t(millis()-lastKill)>=90) {
        lastKill=millis();rumble::pulse(90,22,0);
    }
}
}
