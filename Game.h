#pragma once
#include <stdint.h>

// Clock-free 60 Hz rules shared by the handheld, tests and native preview.
namespace bloom {
constexpr int Q = 64, BULLETS = 192, SHOTS = 40, ENEMIES = 24, SPARKS = 32;
// The HUD starts below row 119. Reserve the complete ship footprint,
// including focus markers and the longest animated exhaust plume.
constexpr int FIELD_TOP=11, FIELD_BOTTOM=119;
constexpr int SHIP_SIDE=7, SHIP_ABOVE=6, SHIP_BELOW=7;
constexpr int PLAYER_MIN_X=SHIP_SIDE, PLAYER_MAX_X=127-SHIP_SIDE;
constexpr int PLAYER_MIN_Y=58, PLAYER_MAX_Y=FIELD_BOTTOM-SHIP_BELOW;
enum Screen : uint8_t { Title, Help, Play, Paused, Evolution, Over };
enum Key : uint8_t { Fire=1, Focus=2, Up=4, Down=8, Left=16, Right=32, Start=64, Bomb=128 };
enum Kind : uint8_t { Invader=1, Scout, Weaver, Turret, Boss };
enum Flight : uint8_t { InFormation, Charging, Looping, Diving, Returning };
enum Event : uint16_t { Shot=1, Kill=2, Hurt=4, Burst=8, Evolve=16, Clear=32, Graze=64, Beat=128,
    BodyImpact=256, BulletImpact=512, FormationImpact=1024 };
struct Bullet { int16_t x=0,y=0,vx=0,vy=0; uint8_t kind=0; };
struct Enemy {
    int16_t x=0,y=0,home=0,originY=0,target=0; uint16_t age=0,hp=0,maxHp=0;
    uint8_t kind=0,seed=0,flash=0,flight=InFormation;
};
struct Spark { int16_t x=0,y=0; int8_t vx=0,vy=0; uint8_t life=0,color=0; };
struct Game {
    Bullet bullets[BULLETS], shots[SHOTS]; Enemy enemies[ENEMIES]; Spark sparks[SPARKS];
    Screen screen=Title, back=Title;
    uint32_t tick=0,score=0,best=0,rng=0xB1006,extraAt=20000;
    uint16_t wave=1,age=0,transition=0,events=0,comboClock=0,graze=0,invincible=0;
    int16_t px=64*Q,py=111*Q;
    int16_t formation=0,formationY=0;
    uint8_t phase=0,lives=3,bombs=3,combo=1,chain=0,cooldown=0,bombFlash=0,shake=0;
    uint8_t previous=0,remaining=0,spawned=0,waveTotal=0,beat=0;
    int8_t direction=1;
    bool focus=false,muted=false;
    void begin(uint32_t seed=0xB1006);
    void update(uint8_t keys);
    void startWave();
    void fire();
    void bomb();
    void hit();
    void damage(Enemy &e,unsigned amount);
    void emit(int x,int y,int vx,int vy,unsigned kind=1);
    void ring(int x,int y,unsigned count,unsigned angle,int speed,unsigned kind=2);
    void aimed(int x,int y,unsigned count,int speed);
    unsigned liveBullets() const;
    unsigned bulletLimit() const;
    unsigned liveEnemies() const;
    unsigned random(unsigned n);
    static unsigned phaseFor(unsigned wave);
private:
    void emitUnchecked(int x,int y,int vx,int vy,unsigned kind);
    void enemiesTick();
    void sortieTick(Enemy &enemy);
    void bulletsTick();
    void shotsTick();
    void effectsTick();
    void advanceWave();
    void spawn(unsigned kind,int x,int y,unsigned seed);
    void sparksAt(int x,int y,unsigned color,unsigned count);
    void points(unsigned n);
};
int sine(unsigned angle);
const char *phaseName(unsigned phase);
const char *unlock(unsigned phase);
}
