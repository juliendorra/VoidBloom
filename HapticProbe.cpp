#if defined(BLOOM_HAPTIC_PROBE)
#include <CHGame.h>
#include "HapticProbe.h"
#include "Haptics.h"
#include "View.h"
#include "src/rumble/Rumble.h"
namespace hapticprobe {
static bool active;
static uint8_t effect;
static unsigned motorOutput() {
#ifdef CHSIM
    return rumble::level();
#else
    return unsigned(TIM2->CH4CVR)*255/1000;
#endif
}
static void center(int y,const char *s,unsigned color) {text35((128-text35Width(s))/2,y,s,color);}
void begin(bloom::Game &game) {
    active=true;effect=0;game.begin();game.wave=5;game.phase=2;game.startWave();
    for(auto &e:game.enemies)e.kind=0;
    game.spawned=game.waveTotal;game.remaining=1;game.transition=0;game.invincible=0;
    audio::setOn(false);rumble::stop();
}
bool frame(bloom::Game &game) {
    if(!active)return false;
    game.update(0);
    if(chgame.justPressed(START_BUTTON)) {
        active=false;game.begin();game.screen=bloom::Title;game.previous=bloom::Start;
        haptics::begin();audio::setOn(!game.muted);
        gfx_wait();dbg::markRenderStart();view::draw(game);dbg::markRenderEnd();gfx_flushAsync();return true;
    }
    if(chgame.justPressed(A_BUTTON)) {
        haptics::begin();game.bombFlash=0;game.invincible=0;game.lives=3;
        game.hit();effect=1;
    } else if(chgame.justPressed(B_BUTTON)) {
        haptics::begin();game.bombs=3;game.bomb();effect=2;
    }
    game.invincible=0;haptics::update(game);audio::setOn(false);audio::update();
    gfx_wait();dbg::markRenderStart();view::draw(game);
    gfx_fillRect(0,0,128,42,0);
    center(3,"RUMBLE COMPARISON",3);center(14,"A: HIT   B: BOMB",4);
    center(24,"SOUND MUTED",2);center(34,"START: RETURN TO GAME",2);
    const char *label=effect==1?"HIT":effect==2?"BOMB":"PRESS A OR B";
    gfx_fillRect(4,47,120,29,0);
    gfx_textScaled((128-gfx_textWidthScaled(label,effect?2:1))/2,50,label,3,effect?2:1);
    center(69,effect==1?"80 MS KNOCK":effect==2?"500 MS / TWO WAVES":"COMPARE THE MOTOR",4);
    const unsigned strength=motorOutput();
    gfx_fillRect(7,80,114,21,0);center(81,strength?"MOTOR OUTPUT ON":"MOTOR OUTPUT OFF",strength?6:4);
    gfx_rect(13,91,102,7,2);gfx_fillRect(14,92,strength*100/255,5,4);
    dbg::markRenderEnd();gfx_flushAsync();return true;
}
bool command(char command) {
    if(command!='V')return false;
    uint32_t values[]={rumble::level(),rumble::remaining(),0,0,0,0};
#ifndef CHSIM
    values[2]=TIM2->CH4CVR;values[3]=TIM2->DMAINTENR&1;
    values[4]=(AFIO->PCFR1>>18)&7;values[5]=GPIOC->CFGLR&15;
#endif
    char line[100];char *p=fmtStr(line,"BLOOMOUTPUT");
    for(uint32_t n:values) {*p++=' ';p=fmtInt(p,n);}
    fmtStr(p,"\n");dbg::print(line);return true;
}
}
#endif
