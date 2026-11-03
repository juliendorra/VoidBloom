#!/usr/bin/env python3
"""Exercise the real portable rumble state machine with a controlled clock."""
from pathlib import Path
import os
import json
import hashlib
import shutil
import subprocess

game=Path(__file__).resolve().parents[1]
out=game/'out/rumble';out.mkdir(parents=True,exist_ok=True)
(out/'Arduino.h').write_text('#include <stdint.h>\nuint32_t millis();\n')
(out/'test.cpp').write_text(r'''
#include <cassert>
#include "Rumble.h"
#include "Haptics.h"
#include <initializer_list>
static uint32_t clockMs;
uint32_t millis() { return clockMs; }
int main() {
 using namespace rumble;
 begin(); assert(level()==0 && remaining()==0);
 pulse(180,70); assert(level()==180 && remaining()==70);
 clockMs=69; assert(level()==180 && remaining()==1);
 clockMs=70; assert(level()==0 && remaining()==0);
 pulse(200,65535); assert(remaining()==500);
 clockMs+=500; assert(level()==0);
 pulse(220,80,2); pulse(110,100,1); assert(level()==220 && remaining()==80);
 pulse(120,40,2); assert(level()==120 && remaining()==40);
 setEnabled(false); assert(level()==0); pulse(200,50); assert(level()==0);
 setEnabled(true); pulse(255,40); assert(level()==255); stop(); assert(level()==0);
 pulse(0,40); assert(level()==0); pulse(255,0); assert(level()==0);
 for(unsigned i=0;i<7;++i) { play(Effect(i)); assert(level()>0 && remaining()<=500); clockMs+=501; assert(level()==0); }
 play(Effect(255)); assert(level()==0);
 clockMs=0xfffffff0u; update(); pulse(180,40); clockMs=24; assert(level()==0);
 // Exercise actual game events through the hardware-independent mapper.
 using namespace bloom;
 static Game g;
 g.begin(12345);g.transition=0;haptics::begin();haptics::update(g);assert(level()==0);
 g.update(Bomb);assert(g.events&Burst);haptics::update(g);assert(level()==255 && remaining()==35);
 // The bomb envelope follows every rendered blast frame, including the
 // quiet gaps. It must finish with the visual, rather than act like a hit.
 const uint32_t blastStart=clockMs;
 for(unsigned frame=0;frame<=BOMB_FRAMES;++frame) {
  if(frame) {clockMs=blastStart+frame*1000/60;g.update(0);haptics::update(g);}
  assert(g.bombFlash==BOMB_FRAMES-frame);
  if(frame==0)assert(level()==255);
  if(frame>=7 && frame<=16)assert(level()==0);
  if(frame==17)assert(level()==235);
  if(frame==20)assert(level()==190);
  if(frame==29)assert(level()>0);
  if(frame==30)assert(level()==0);
 }
 // A stalled game loop also expires the short refresh pulse on its own.
 g.begin(12345);g.transition=0;haptics::begin();g.update(Bomb);haptics::update(g);
 clockMs+=35;assert(level()==0 && g.bombFlash==BOMB_FRAMES);
 // Pause freezes the visual and silences rumble; resume follows its age.
 g.events=0;g.bombFlash=13;haptics::update(g);assert(level()==235);
 g.screen=Paused;haptics::update(g);assert(level()==0);clockMs+=200;
 g.screen=Play;haptics::update(g);assert(level()==235 && g.bombFlash==13);
 // Even a zero-strength trough must not interrupt a higher-priority hit.
 g.events=Hurt;haptics::update(g);assert(level()==255);
 clockMs+=16;g.events=0;g.bombFlash=23;haptics::update(g);assert(level()==255);
 g.bombFlash=0;haptics::update(g);assert(level()==255);
 for(auto screen:{Title,Help,Paused}) {
  g.screen=screen;g.events=Burst;haptics::update(g);assert(level()==0);
 }
 g.begin(12345);g.phase=2;g.wave=5;g.startWave();g.transition=0;
 haptics::begin();g.update(Focus);assert(g.events&GunSwitch);g.muted=true;
 haptics::update(g);assert(level()==105 && remaining()==35); // sound mute is independent
 clockMs+=35;assert(level()==0);
 g.begin(12345);haptics::begin();g.damage(g.enemies[0],1);
 assert((g.events&Kill) && !(g.events&BossDown));haptics::update(g);assert(level()==90 && remaining()==22);
 clockMs+=10;g.events=0;g.damage(g.enemies[1],1);haptics::update(g);assert(remaining()==12);
 g.invincible=0;g.hit();haptics::update(g);assert(level()==255 && remaining()==80);
 clockMs+=10;g.events=Kill|GunSwitch;haptics::update(g);assert(level()==255 && remaining()==70);
 clockMs+=70;assert(level()==0);
 g.events=Shot|Graze|Beat;haptics::update(g);assert(level()==0);
 g.begin(12345);g.wave=4;g.phase=1;g.startWave();g.transition=0;haptics::begin();
 g.damage(g.enemies[0],10000);assert(g.events&BossDown);haptics::update(g);assert(level()==240 && remaining()==240);
 haptics::begin();g.update(0);assert(g.screen==Evolution && (g.events&Evolve));
 haptics::update(g);assert(level()==180 && remaining()==110);
 clockMs+=110;g.events=0;haptics::update(g);assert(level()==0);
 g.begin(12345);g.lives=1;g.invincible=0;haptics::begin();g.hit();assert(g.screen==Over);
 haptics::update(g);assert(level()==255);clockMs+=20;g.update(0);haptics::update(g);assert(remaining()==60);
 g.screen=Title;haptics::update(g);assert(level()==0);
 g.screen=Play;g.events=Clear;haptics::update(g);assert(level()==155 && remaining()==70);
 g.events=Hurt|BossDown|Burst;haptics::update(g);assert(level()==255 && remaining()==80);
 g.screen=Paused;haptics::update(g);clockMs+=1000;assert(level()==0);

}
''')
cxx=os.environ.get('CHSIM_CXX') or shutil.which('clang++') or shutil.which('c++')
subprocess.run([cxx,'-std=c++17','-DCHSIM','-fsanitize=undefined,bounds','-fno-sanitize-recover=all',
               '-I'+str(out),'-I'+str(game/'src/rumble'),str(out/'test.cpp'),str(game/'src/rumble/Rumble.cpp'),str(game/'Haptics.cpp'),str(game/'Game.cpp'),'-I'+str(game),'-o',str(out/'test')],check=True)
subprocess.run([str(out/'test')],check=True)
print('PASS: Pinball rumble driver timing, cap, priorities and wrap; real game events, two bomb ripples synchronized to the blast, pause/resume and stalled-loop expiry; kill rate limit; mute independence; pause/menu stop; no shot/graze/beat buzz')

(out/'validation.json').write_text(json.dumps({
    'driver_source': 'Pinball/src/rumble (unchanged)',
    'driver_sha256': hashlib.sha256((game/'src/rumble/Rumble.cpp').read_bytes()).hexdigest(),
    'boot_off': True, 'pulse_cap_ms': 500, 'priority_preemption': True,
    'clock_wrap': True, 'game_event_routing': True, 'kill_throttle_ms': 90, 'bomb_ripple_frames': 30, 'bomb_ripple_peaks': [255,235], 'bomb_off_gap_frames': 10, 'hit_pulse_ms': 80, 'bomb_quiet_gaps': True, 'bomb_frame_sync_and_pause_resume': True, 'stalled_loop_expiry_ms': 35,
    'pause_and_menu_immediate_stop': True, 'sound_mute_independent': True,
    'final_hit_pulse_completes': True, 'ordinary_shooting_graze_music_no_pulse': True,
    'sanitizers': ['undefined', 'bounds']}, indent=2)+'\n')
