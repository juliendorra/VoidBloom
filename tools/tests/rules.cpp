#include "../../Game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>
using namespace bloom;
static Game g;
static void ready(unsigned wave=1) {
    g.begin(12345);g.wave=wave;g.phase=Game::phaseFor(wave);g.startWave();g.transition=0;
}
static void empty() {for(auto &e:g.enemies)e.kind=0;for(auto &b:g.bullets)b.kind=0;}
static void frames(unsigned n,unsigned keys=0) {while(n--)g.update(keys);}
static void check() {
    assert(g.liveBullets()<=g.bulletLimit() && g.liveEnemies()<=ENEMIES);
    assert(g.phase<=4 && g.lives<=5 && g.bombs<=5 && g.combo<=8);
    assert(g.px>=PLAYER_MIN_X*Q && g.px<=PLAYER_MAX_X*Q && g.py>=PLAYER_MIN_Y*Q && g.py<=PLAYER_MAX_Y*Q);
    assert(g.px/Q-SHIP_SIDE>=0 && g.px/Q+SHIP_SIDE<=127);
    assert(g.py/Q-SHIP_ABOVE>=FIELD_TOP && g.py/Q+SHIP_BELOW<=FIELD_BOTTOM);
    for(const auto &b:g.bullets)if(b.kind)assert(b.x>=-12*Q && b.x<=140*Q && b.y>=-14*Q && b.y<=138*Q);
}
int main() {
    static_assert(sizeof(Game)<4096,"Keep native state within the handheld RAM budget");
    ready();int y=g.py;frames(30,Up|Fire);assert(g.py==y && g.phase==0);
    frames(120,Left);assert(g.px==7*Q);frames(120,Right);assert(g.px==120*Q);
    ready(3);frames(20,Up);assert(g.py<111*Q);frames(100,Up|Left);assert(g.py==58*Q && g.px==7*Q);
    frames(140,Down|Right);assert(g.py==112*Q && g.px==120*Q);
    for(unsigned wave:{1u,3u,5u,7u,9u})for(unsigned focus:{0u,unsigned(Focus)})
    for(unsigned keys:{unsigned(Left),unsigned(Right),unsigned(Up),unsigned(Down),
                       unsigned(Up|Left),unsigned(Up|Right),unsigned(Down|Left),unsigned(Down|Right)}) {
        ready(wave);empty();g.spawned=g.waveTotal;g.remaining=1;frames(240,keys|focus);check();
        assert(g.px==((keys&Left)?7:(keys&Right)?120:64)*Q);
        assert(g.py==(wave==1?111:(keys&Up)?58:(keys&Down)?112:111)*Q);
    }
    ready(3);frames(10,Left);int fast=64*Q-g.px;
    ready(3);frames(10,Left|Focus);assert(64*Q-g.px<fast);
    // Wave 2 introduces one readable sortie before free flight. It must
    // leave smoothly, loop above the slot, lock its dive, and return home.
    ready(2);g.invincible=60000;y=g.py;frames(89,Up);assert(g.py==y);
    for(const auto &e:g.enemies)assert(e.flight==InFormation);
    frames(1);int flyer=-1;
    for(unsigned i=0;i<ENEMIES;++i)if(g.enemies[i].flight==Charging)flyer=i;
    assert(flyer>=0);auto &f=g.enemies[flyer];
    frames(20);assert(f.flight==Looping);int loopY=f.y,previousX=f.x,previousY=f.y;
    int highest=loopY;
    for(unsigned i=0;i<48;++i) {
        frames(1);assert(f.kind==Invader);
        assert((f.x-previousX)*(f.x-previousX)+(f.y-previousY)*(f.y-previousY)<4*Q*Q);
        previousX=f.x;previousY=f.y;if(f.y<highest)highest=f.y;
        unsigned active=0;for(const auto &e:g.enemies)active+=e.kind && e.flight!=InFormation;
        assert(active==1);
    }
    assert(highest<loopY-10*Q && f.flight==Diving);
    int locked=f.target;frames(30,Right);assert(g.px!=locked && f.target==locked);
    frames(42);assert(f.flight==Returning && f.y<0);
    frames(64);assert(f.flight==InFormation);
    assert(f.x==(23+int(f.seed%5)*20)*Q+g.formation);
    assert(f.y==(24+int(f.seed/5)*13)*Q+g.formationY);
    // Shooting an airborne invader counts as a kill; a bomb also clears it.
    ready(2);g.invincible=60000;frames(120);
    for(auto &e:g.enemies)if(e.flight==Looping) {
        unsigned left=g.remaining;g.damage(e,1);assert(!e.kind && g.remaining==left-1);break;
    }
    ready(2);g.invincible=60000;frames(160);g.bomb();assert(!g.liveEnemies() && !g.remaining);
    for(unsigned phase=0;phase<5;++phase) {
        ready(phase*2+1);g.fire();unsigned count=0;
        for(const auto &b:g.shots)count+=b.kind!=0;
        assert(count==(phase==0?1:phase==1?2:phase==2?3:5));
    }
    ready();g.invincible=0;g.update(Start);assert(g.screen==Paused);
    static Game snapshot;memcpy(&snapshot,&g,sizeof g);
    frames(100,Up|Bomb|Left);assert(g.tick==snapshot.tick && g.age==snapshot.age && g.bombs==snapshot.bombs);
    assert(memcmp(g.bullets,snapshot.bullets,sizeof g.bullets)==0);
    g.update(0);g.update(Start);assert(g.screen==Play);
    ready(5);empty();g.invincible=0;g.emit(g.px+6*Q,g.py,0,0);
    frames(1);assert(g.graze==1 && g.lives==3);unsigned score=g.score;
    frames(25);assert(g.graze==1 && g.score==score);
    // A shot overlaps the collision core, rather than the cosmetic wing.
    g.emit(g.px,g.py,0,0);frames(1);assert(g.lives==2 && g.invincible && !g.liveBullets());
    frames(10);assert(g.lives==2);
    ready(5);empty();g.invincible=0;g.graze=39;g.bombs=0;
    g.emit(g.px+6*Q,g.py,0,0);frames(1);assert(g.bombs==1 && g.graze==40);
    ready(4);g.enemies[0].y=30*Q;g.emit(g.px,g.py,0,0);g.invincible=0;
    unsigned hp=g.enemies[0].hp;g.update(Bomb);
    assert(g.bombs==2 && !g.liveBullets() && g.lives==3 && g.enemies[0].hp==hp-24);
    frames(20,Bomb);assert(g.bombs==2);g.update(0);g.update(Bomb);assert(g.bombs==1);
    // All emission routes obey encounter limits. Full pools never replace
    // a projectile, and a rejected volley never leaves a partial pattern.
    unsigned previousLimit=0;
    for(unsigned wave=1;wave<=40;++wave) {
        ready(wave);empty();unsigned limit=g.bulletLimit();
        assert(limit>=previousLimit && limit<=BULLETS);previousLimit=limit;
        for(unsigned i=0;i<limit-1;++i)g.emit(20*Q,30*Q,0,0);
        static Bullet before[BULLETS];memcpy(before,g.bullets,sizeof before);
        g.ring(40*Q,30*Q,2,0,60);g.aimed(40*Q,30*Q,3,60);
        assert(memcmp(before,g.bullets,sizeof before)==0);
        g.emit(20*Q,30*Q,0,0);assert(g.liveBullets()==limit);
        memcpy(before,g.bullets,sizeof before);
        for(unsigned i=0;i<BULLETS+50;++i)g.emit(40*Q,40*Q,50,50);
        assert(memcmp(before,g.bullets,sizeof before)==0);
        g.bomb();assert(!g.liveBullets());
        g.ring(40*Q,30*Q,4,0,60);assert(g.liveBullets()==4);
    }
    ready(4);assert(g.bulletLimit()==12);
    // Each first-boss pattern has a full second with no new shots.
    g.invincible=60000;
    for(unsigned pattern=0;pattern<3;++pattern) {
        for(auto &b:g.bullets)b.kind=0;
        g.enemies[0].age=239+pattern*180;
        frames(60);assert(!g.liveBullets());
    }
    ready();empty();g.invincible=0;g.hit();g.invincible=0;g.hit();g.invincible=0;g.hit();assert(g.screen==Over);
    frames(65);g.update(Fire);assert(g.screen==Play && g.lives==3 && g.wave==1 && g.phase==0 && g.score==0);
    ready(2);g.spawned=g.waveTotal;empty();g.remaining=0;g.update(0);
    assert(g.wave==3 && g.phase==1 && g.screen==Evolution);
    unsigned clock=g.age;frames(60);assert(g.age==clock);frames(150);assert(g.screen==Play);
    // Direct damage is only used here to prove every transition and capped
    // endless scaling independently of the button-driven campaign below.
    ready();
    for(unsigned wave=1;wave<=40;++wave) {
        g.transition=0;g.screen=Play;g.spawned=g.waveTotal;g.remaining=1;
        for(auto &e:g.enemies)if(e.kind)g.damage(e,20000);
        g.remaining=0;g.update(0);assert(g.wave==wave+1 && g.phase==Game::phaseFor(wave+1));
        check();
    }
    // All boss pattern families must actually emit, in every late phase.
    for(unsigned wave:{4u,8u,12u,40u}) {
        ready(wave);g.invincible=60000;
        unsigned peak=0;
        for(unsigned i=0;i<3600;++i) {g.update(0);if(g.liveBullets()>peak)peak=g.liveBullets();check();}
        assert(peak>=(wave==4?8u:40u) && peak<=g.bulletLimit());
        printf("Boss wave %u: peak %u live bullets\n",wave,peak);
    }
    // Deterministic ordinary movement/fire can finish the intro and then
    // survive a long run with test-only invulnerability. No forced clears.
    ready();unsigned last=1;
    for(unsigned i=0;i<100000 && g.wave<21;++i) {
        g.invincible=60000;
        int target=64*Q,lowest=-32000;
        for(const auto &e:g.enemies)if(e.kind && e.y>lowest) {lowest=e.y;target=e.x;}
        unsigned keys=Fire;
        if(target>g.px+Q)keys|=Right;else if(target<g.px-Q)keys|=Left;
        g.update(keys);check();
        if(g.wave!=last) {printf("Cleared wave %u with fire / movement, score %lu\n",last,(unsigned long)g.score);last=g.wave;}
    }
    assert(g.wave>=21);
    printf("PASS: movement locks, wave-two loop/dive/return, focus, all weapon tiers, freeze/resume, graze, bomb priority/edge, deaths, evolution, per-wave bullet caps, whole volleys and boss recovery; state %lu bytes\n",(unsigned long)sizeof g);
}
