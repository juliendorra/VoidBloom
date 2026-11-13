// An original evolving arcade shooter for the 128x128 CHGame handheld.
#include <CHGame.h>
#include <CHSave.h>
#include "Game.h"
#include "View.h"
#include "Haptics.h"
#include "HapticProbe.h"
#include "src/rumble/Rumble.h"

bloom::Game game;
struct Record { uint32_t high=0;uint8_t muted=0; } record;
static bool savePending=false;
constexpr auto SAVE_ID = chsave::id("com.juliendorra.voidbloom");
static chsave::Store gameSaves;
enum Sound : uint8_t { FireSound,KillSound,HurtSound,BombSound,EvolveSound,ClearSound };
AUDIO_STEPS(FIRE_SOUND)={AUDIO_STEP(2400,1000,18)};
AUDIO_STEPS(KILL_SOUND)={AUDIO_STEP(1100,320,35)};
AUDIO_STEPS(HURT_SOUND)={AUDIO_STEP(700,100,160),AUDIO_REST(25),AUDIO_STEP(400,80,180)};
AUDIO_STEPS(BOMB_SOUND)={AUDIO_STEP(160,3200,80),AUDIO_STEP(3200,100,300)};
AUDIO_STEPS(EVOLVE_SOUND)={AUDIO_STEP(1040,0,75),AUDIO_STEP(1320,0,75),AUDIO_STEP(1560,0,75),AUDIO_STEP(2080,0,200)};
AUDIO_STEPS(CLEAR_SOUND)={AUDIO_STEP(1560,0,70),AUDIO_STEP(2080,0,100),AUDIO_STEP(2640,0,150)};
const audio::Effect SOUNDS[]={AUDIO_EFFECT(FIRE_SOUND,audio::SOFT),AUDIO_EFFECT(KILL_SOUND,1|audio::SOFT),
    AUDIO_EFFECT(HURT_SOUND,5),AUDIO_EFFECT(BOMB_SOUND,4),AUDIO_EFFECT(EVOLVE_SOUND,6),AUDIO_EFFECT(CLEAR_SOUND,3)};

#if CHGAME_DEBUG
static bool debug(char command,const char *args) {
#if defined(BLOOM_HAPTIC_PROBE)
    if(hapticprobe::command(command))return true;
#endif
#if defined(CHSIM) && defined(BLOOM_CAPTURE)
    // Empty arena for exercising controls and comparing the whole ship
    // against an unclipped reference. Never compiled into firmware.
    if(command=='E') {
        unsigned wave=3,tick=1,hidden=0;int x=64,y=85;
        sscanf(args,"%u %d %d %u %u",&wave,&x,&y,&tick,&hidden);
        if(wave<1 || wave>12 || x<0 || x>127 || y<11 || y>119)return false;
        game.begin(12345);game.wave=wave;game.phase=bloom::Game::phaseFor(wave);game.startWave();
        for(auto &e:game.enemies)e.kind=0;
        game.spawned=game.waveTotal;game.remaining=1;game.transition=0;
        game.px=x*bloom::Q;game.py=y*bloom::Q;game.tick=tick;game.invincible=hidden?7:0;
        return true;
    }
    // Stage selection exists only in the separately built capture fixture.
    // The cartridge and ordinary simulator never include this command.
    if(command=='J') {
        unsigned wave=atoi(args);if(wave<1 || wave>999)return false;
        game.begin(12345);game.wave=wave;game.phase=bloom::Game::phaseFor(wave);
        game.startWave();game.transition=0;
        const char *mode=strchr(args,' ');
        game.invincible=mode?atoi(mode+1):60000;return true;
    }
#endif
    if(command!='H')return false;
    // Avoid variadic printf on the handheld's compact C runtime.
    const uint32_t values[]={game.screen,game.wave,game.phase,game.lives,game.score,game.tick,
        game.bombs,game.combo,uint32_t(game.px),uint32_t(game.py),game.liveBullets(),game.liveEnemies(),
        game.transition,game.invincible,game.graze,game.focus,game.muted,game.remaining,
        game.gunNotice,game.gunNoticeTime,rumble::level(),rumble::remaining()};
    char line[260];char *out=fmtStr(line,"BLOOMSTATE");
    for(uint32_t value:values) {
        *out++=' ';char digits[10];unsigned n=0;
        do {digits[n++]=char('0'+value%10);value/=10;}while(value);
        while(n)*out++=digits[--n];
    }
    *out++='\n';*out=0;
    dbg::print(line);return true;
}
#endif
void setup() {
    chgame.boot();dbg::begin("BLOOM 1.3.8");
#if CHGAME_DEBUG
    dbg::hook=debug;
#endif
    gfx_begin(GFX_DIV2,GFX_12BPP);chgame.setFrameRate(60);
    if(!gameSaves.load(SAVE_ID,1,record))record=Record{};
    game.best=record.high;game.muted=record.muted;
    audio::begin(SOUNDS,sizeof SOUNDS/sizeof SOUNDS[0],!game.muted);
    haptics::begin();
#if defined(BLOOM_HAPTIC_PROBE)
    hapticprobe::begin(game);
#endif
}
static void sound() {
    using namespace bloom;
    audio::setOn(!game.muted && game.screen!=Paused && game.screen!=Help);
    if(game.events&Evolve)audio::sfx(EvolveSound);
    else if(game.events&Hurt)audio::sfx(HurtSound);
    else if(game.events&Burst)audio::sfx(BombSound);
    else if(game.events&Clear)audio::sfx(ClearSound);
    else if(game.events&Kill)audio::sfx(KillSound);
    else if(game.events&GunSwitch)audio::blip(game.focus?1760:1320,90,true);
    else {
        // Original ascending pulse motif gains subdivisions with evolution.
        static const uint16_t notes[]={262,0,392,0,311,0,466,0,262,392,523,392,311,466,622,466};
        unsigned index=game.phase<2?(game.beat&7):game.beat;
        // A held fire button must not erase a coincident music beat. The
        // priority-1, full-width note survives priority-0 gun clicks;
        // kills and important combat cues can still interrupt it.
        if((game.events&Beat) && notes[index])
            audio::note(notes[index]*(game.phase>=3?4:2),game.phase?38:55,1);
        else if(game.events&Shot)audio::sfx(FireSound);
    }
    audio::update();
}
void loop() {
    rumble::update();
    dbg::poll();if(!chgame.nextFrame())return;
    chgame.pollButtons();
#if defined(BLOOM_HAPTIC_PROBE)
    if(hapticprobe::frame(game))return;
#endif
    uint8_t keys=0;
    if(chgame.pressed(A_BUTTON))keys|=bloom::Fire;
    if(chgame.pressed(B_BUTTON))keys|=bloom::Bomb;
    if(chgame.pressed(UP_BUTTON))keys|=bloom::Up;
    if(chgame.pressed(DOWN_BUTTON))keys|=bloom::Down;
    if(chgame.pressed(LEFT_BUTTON))keys|=bloom::Left;
    if(chgame.pressed(RIGHT_BUTTON))keys|=bloom::Right;
    if(chgame.pressed(START_BUTTON))keys|=bloom::Start;
    if(chgame.pressed(SELECT_BUTTON))keys|=bloom::Focus;
    const auto previous=game.screen;game.update(keys);sound();haptics::update(game);
    if((game.screen==bloom::Over && previous!=bloom::Over) ||
       (game.screen==bloom::Title && previous==bloom::Paused) || record.muted!=game.muted) {
        record.high=game.best;record.muted=game.muted;savePending=true;
    }
    gfx_wait();
    if(savePending) {gameSaves.store(SAVE_ID,1,record);savePending=false;}
    dbg::markRenderStart();view::draw(game);dbg::markRenderEnd();gfx_flushAsync();
}
