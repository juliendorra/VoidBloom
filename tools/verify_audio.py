#!/usr/bin/env python3
"""Exercise the actual game's audio routing through the pinned SDK timer model."""
from pathlib import Path
import os, subprocess
ROOT=Path(__file__).resolve().parents[1]
SDK=Path(os.environ.get('CHGAME_ROOT', ROOT/'.deps/CHGame'))
MODEL=SDK/'tools/audio/host'
LIB=SDK/'platform/board/arduino/CHGame/libraries/CHGame/src'
if not (MODEL/'harness.cpp').exists():raise SystemExit('Set CHGAME_ROOT to the pinned CHGame SDK checkout.')
OUT=ROOT/'out/audio-check';OUT.mkdir(parents=True,exist_ok=True)
(OUT/'timer_model.h').write_text((MODEL/'harness.cpp').read_text().split('int main(',1)[0])
source=(ROOT/'VoidBloom.ino').read_text()

effects = source.split('enum Sound :', 1)[1].split('#if CHGAME_DEBUG', 1)[0]
route = source.split('static void sound() {', 1)[1].split('\n}\nvoid loop()', 1)[0]
bloom = OUT / 'bloom_audio.cpp'
bloom.write_text('''#include "timer_model.h"
#include "Game.h"
#include <cassert>
bloom::Game game;
enum Sound :''' + effects + '\nstatic void sound() {' + route + '''
}
int main() {
    audio::begin(SOUNDS, 6);
    game.screen=bloom::Play; game.beat=0;
    game.events=bloom::Shot|bloom::Beat; sound(); osSystickHandler();
    assert(cen && ccr==(arr+1)/2 && 1000000u/(arr+1)==520);
    // Continuous firing cannot cut the scheduled melody note short.
    for (unsigned i=0;i<20;++i) {
        game.events=bloom::Shot; sound(); osSystickHandler();
        assert(cen && 1000000u/(arr+1)==520);
    }
    game.events=bloom::Hurt|bloom::Beat; sound(); osSystickHandler();
    assert(cen && 1000000u/(arr+1)>=699 && 1000000u/(arr+1)<=701);
    game.screen=bloom::Paused; game.events=0; sound();
    assert(!cen && !ccr);
    game.screen=bloom::Play; game.muted=true;
    game.events=bloom::Shot|bloom::Beat; sound(); osSystickHandler();
    assert(!cen && !ccr);
    puts("PASS: Void Bloom music survives held fire; hurt overrides music; pause and mute silence the real timer");
}
''')

exe=OUT/"audio_test"
subprocess.run(["clang++","-std=c++17","-O1","-g","-fsanitize=undefined","-fno-sanitize-recover=all",f"-I{MODEL}",f"-I{LIB}",f"-I{OUT}",f"-I{ROOT}",str(bloom),str(LIB/"chgame/Audio.cpp"),"-o",str(exe)],check=True)
subprocess.run([str(exe)],check=True)
