// Reproducible human-bounded heuristic pilots. Rules run at their real 60 Hz;
// only wall-clock execution is accelerated. No invulnerability, stage jumps,
// health changes, bullet clears or future game states are used by a pilot.
#include "../../Game.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <algorithm>
using namespace bloom;
namespace {
static Game g,perceived,older;
struct Wave {unsigned entries=0,clears=0,deaths=0,body=0,bolt=0,bombs=0,frames=0,peak=0;unsigned long bullets=0;} stats[14];
unsigned randomState;
unsigned rnd(unsigned n) {randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState%n;}
int absval(int v) {return v<0?-v:v;}
float bounded(float v,float a,float b) {return v<a?a:v>b?b:v;}
struct Decision {unsigned keys=Fire;float danger=0;};
Decision choose(Game &s,unsigned id,unsigned delay,int ownX,int ownY,bool allowFocus=true,int patrolY=0) {
    // Extrapolate the delayed visible trajectories by the known observation
    // lag. The pilot knows its own movement, like a person holding a D-pad.
    s.px=ownX;s.py=ownY;
    for(auto &b:s.bullets)if(b.kind) {b.x+=b.vx*delay;b.y+=b.vy*delay;}
    float enemyVX[ENEMIES]={},enemyVY[ENEMIES]={};
    for(unsigned i=0;i<ENEMIES;++i) {
        auto &e=s.enemies[i];const auto &before=older.enemies[i];
        if(e.kind && e.kind==before.kind && e.seed==before.seed) {
            enemyVX[i]=bounded(float(e.x-before.x)/Q/delay,-3,3);
            enemyVY[i]=bounded(float(e.y-before.y)/Q/delay,-1,1);
            e.x+=int(enemyVX[i]*Q*delay);e.y+=int(enemyVY[i]*Q*delay);
        }
    }
    float target=64,lowest=-1000;
    for(const auto &e:s.enemies)if(e.kind && e.y>lowest) {lowest=e.y;target=float(e.x)/Q;}
    float best=1e20f;Decision result;
    // A player can anticipate visible linear trajectories, but not future
    // enemy decisions. Candidate directions are ordinary D-pad inputs.
    for(int dy=s.phase?-1:0;dy<=int(s.phase!=0);++dy)for(int dx=-1;dx<=1;++dx)
    for(unsigned focus=0;focus<(allowFocus?2u:1u);++focus) {
        float speed=(focus?40.f:100.f)/Q;if(dx && dy)speed*=.707f;
        float cost=0,danger=0;
        const float x0=float(s.px)/Q,y0=float(s.py)/Q;
        for(unsigned t=0;t<=40;t+=4) {
            float x=bounded(x0+dx*speed*t,PLAYER_MIN_X,PLAYER_MAX_X);
            float y=bounded(y0+dy*speed*t,PLAYER_MIN_Y,PLAYER_MAX_Y);
            for(const auto &b:s.bullets)if(b.kind) {
                float bx=float(b.x+b.vx*int(t))/Q,by=float(b.y+b.vy*int(t))/Q;
                if(bx<-5 || bx>133 || by<10 || by>128)continue;
                float ax=bx-x,ay=by-y,d2=ax*ax+ay*ay;
                float r=(b.kind&127)==2?5.5f:4.5f;
                float threat=d2<r*r?3500.f/(1+t*.1f):d2<400?40.f/(d2+4):0;
                danger+=threat;
            }
            for(unsigned i=0;i<ENEMIES;++i)if(s.enemies[i].kind && s.enemies[i].y>50*Q && t<=24) {
                const auto &e=s.enemies[i];
                float ax=float(e.x)/Q+enemyVX[i]*t-x,ay=float(e.y)/Q+enemyVY[i]*t-y;
                if(ax*ax+ay*ay<196)danger+=1500;
            }
        }
        float endX=bounded(x0+dx*speed*12,PLAYER_MIN_X,PLAYER_MAX_X);
        float endY=bounded(y0+dy*speed*12,PLAYER_MIN_Y,PLAYER_MAX_Y);
        int targetY=patrolY?patrolY:109+(id%3)*3;
        cost=danger*4+absval(int(endX-target))*.24f+fabsf(endY-targetY)*.25f;
        if(endY<90)cost+=(90-endY)*3;
        cost+=focus?.16f:0;cost+=dx==0&&dy==0?0:.08f;
        // Avoid prolonged edge camping, where half of the escape directions vanish.
        if(endX<12 || endX>116)cost+=1.5f;
        if(cost<best) {
            best=cost;result.danger=danger;result.keys=Fire|(focus?Focus:0)|
                (dx<0?Left:dx>0?Right:0)|(dy<0?Up:dy>0?Down:0);
        }
    }
    bool panic=false;
    for(const auto &b:s.bullets)if(b.kind) {
        float ax=float(b.x-s.px)/Q,ay=float(b.y-s.py)/Q;
        float vx=float(b.vx)/Q,vy=float(b.vy)/Q;
        float near=-(ax*vx+ay*vy)/(vx*vx+vy*vy+.001f);
        float ex=ax+vx*bounded(near,0,12),ey=ay+vy*bounded(near,0,12);
        if(near>=0 && near<12 && ex*ex+ey*ey<36)panic=true;
    }
    for(unsigned i=0;i<ENEMIES;++i)if(s.enemies[i].kind) {
        float ax=float(s.enemies[i].x-s.px)/Q+enemyVX[i]*10;
        float ay=float(s.enemies[i].y-s.py)/Q+enemyVY[i]*10;
        if(ax*ax+ay*ay<100)panic=true;
    }
    if(s.bombs && !s.invincible && (panic||result.danger>1000))result.keys|=Bomb;
    return result;
}
}
int main(int argc,char **argv) {
    if(argc==3 && !strcmp(argv[1],"--readme-run")) {
        // One ordinary campaign, replayed from the title screen by the
        // simulator. Patrol between vertical bands to demonstrate free
        // flight; the normal benchmark's pilot preferences stay unchanged.
        unsigned seed=unsigned(strtoul(argv[2],nullptr,0));
        g.begin(seed);perceived=g;older=g;randomState=seed;
        puts("[");
        for(unsigned t=0;t<60*360 && g.screen!=Over && g.wave<13;t+=6) {
            static Game observation;observation=perceived;
            int targetY=g.phase?((g.age/120)%2?84:108):0;
            unsigned keys=choose(perceived,0,6,g.px,g.py,true,targetY).keys;
            older=observation;perceived=g;
            if(rnd(40)==0)keys&=~(Left|Right|Up|Down);
            if(g.screen==Evolution)keys=0;
            unsigned events=0;
            for(unsigned i=0;i<6;++i) {g.update(keys);events|=g.events;}
            unsigned flight=0;for(const auto &e:g.enemies)if(e.kind && e.flight>flight)flight=e.flight;
            printf("%s{\"keys\":%u,\"screen\":%u,\"wave\":%u,\"phase\":%u,\"lives\":%u,\"score\":%lu,\"tick\":%lu,\"bombs\":%u,\"x\":%d,\"y\":%d,\"bullets\":%u,\"enemies\":%u,\"transition\":%u,\"invincible\":%u,\"focus\":%u,\"events\":%u,\"flight\":%u}",
                t?",\n":"",keys,g.screen,g.wave,g.phase,g.lives,(unsigned long)g.score,(unsigned long)g.tick,g.bombs,
                g.px,g.py,g.liveBullets(),g.liveEnemies(),g.transition,g.invincible,unsigned(g.focus),events,flight);
        }
        puts("\n]");return g.wave>=13?0:1;
    }
    if(argc==5 && !strcmp(argv[1],"--capture")) {
        // Export normal controls from the same delayed-observation pilot.
        // The simulator replays these and verifies that state stays in sync.
        unsigned wave=unsigned(atoi(argv[2])),warmup=unsigned(atoi(argv[3])),length=unsigned(atoi(argv[4]));
        if(wave<3 || wave>12 || length%6)return 1;
        g.begin(12345);g.wave=wave;g.phase=Game::phaseFor(wave);g.startWave();g.transition=0;g.invincible=0;
        for(unsigned t=0;t<warmup;++t)g.update(0);
        perceived=g;older=g;
        puts("[");
        for(unsigned t=0;t<length;t+=6) {
            bool showcaseBoss=wave%4==0 && t<480;
            static Game observation;observation=perceived;
            unsigned keys=choose(perceived,wave,6,g.px,g.py,!showcaseBoss).keys;
            older=observation;perceived=g;
            if(showcaseBoss)keys&=~(Fire|Focus|Bomb);
            if(wave==10 && t<150)keys&=~Bomb;
            if(wave==10 && t==150)keys|=Bomb;
            for(unsigned frame=0;frame<6;++frame)g.update(keys);
            printf("%s{\"keys\":%u,\"wave\":%u,\"lives\":%u,\"x\":%d,\"y\":%d,\"bullets\":%u}",
                t?",\n":"",keys,g.wave,g.lives,g.px,g.py,g.liveBullets());
            if(g.screen!=Play || g.wave!=wave)return 1;
        }
        puts("\n]");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"--pressure")) {
        // Isolate encounter pressure from weapon strength and pilot skill.
        // This fixture uses invulnerability ONLY for passive measurement;
        // it is separate from every no-cheat campaign below.
        const unsigned waves[]={1,2,3,5,7,9,4,8,12};
        printf("{\"fixture\":\"20 seeds per encounter, passive stationary target, no fire; invulnerability only for pressure measurement\",\"encounters\":[");
        for(unsigned j=0;j<sizeof(waves)/sizeof(waves[0]);++j) {
            unsigned wave=waves[j],peak=0,frames=0;unsigned long sum=0;
            for(unsigned seed=1;seed<=20;++seed) {
                g.begin(1000+seed);g.wave=wave;g.phase=Game::phaseFor(wave);g.startWave();g.transition=0;
                for(unsigned t=0;t<(wave%4==0?3600u:900u) && g.wave==wave;++t) {
                    g.invincible=60000;g.update(0);unsigned n=g.liveBullets();
                    sum+=n;++frames;peak=std::max(peak,n);
                }
            }
            printf("%s{\"wave\":%u,\"phase\":%u,\"boss\":%s,\"bullet_limit\":%u,\"mean_bullets\":%.3f,\"peak_bullets\":%u}",j?",":"",wave,Game::phaseFor(wave),wave%4==0?"true":"false",g.bulletLimit(),double(sum)/frames,peak);
        }
        puts("]}");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"--boss-dodge")) {
        // A separate staged encounter: survive multiple full boss cycles
        // with ordinary movement alone. No fire, focus, bombs or cheats.
        unsigned survivors=0,noHit=0,totalHits=0,peak=0;
        const unsigned count=24;
        for(unsigned id=0;id<count;++id) {
            g.begin(0xD0D600+id*7919);g.wave=4;g.phase=Game::phaseFor(4);g.startWave();g.transition=0;
            perceived=g;older=g;unsigned cadence=6+(id%3)*3,keys=0,hits=0;
            for(unsigned t=0;t<3600 && g.screen==Play;++t) {
                if(t%cadence==0) {
                    static Game observation;observation=perceived;
                    keys=choose(perceived,id,cadence,g.px,g.py,false).keys&~(Fire|Focus|Bomb);
                    older=observation;perceived=g;
                }
                unsigned lives=g.lives;g.update(keys);hits+=g.lives<lives;
                peak=std::max(peak,g.liveBullets());
            }
            survivors+=g.screen==Play;noHit+=!hits;totalHits+=hits;
        }
        printf("{\"fixture\":\"staged wave 4; 60 seconds; 100-200 ms delayed observations and input cadence; movement only, no fire/focus/bombs or invulnerability\",\"pilots\":%u,\"survived\":%u,\"no_hit\":%u,\"total_hits\":%u,\"peak_bullets\":%u}\n",count,survivors,noHit,totalHits,peak);
        return 0;
    }
    const unsigned count=argc>1?unsigned(atoi(argv[1])):48;
    unsigned finished=0,evolved=0;unsigned totalDeaths=0;unsigned highest=1;
    printf("{\"model\":\"60 Hz rules; 100-200 ms delayed observations, 100-200 ms input cadence; ordinary D-pad/fire/focus/bomb; no cheats\",\"target_wave\":13,\"pilots\":[\n");
    for(unsigned id=0;id<count;++id) {
        randomState=0xCAB000+id*7919;g.begin(randomState);perceived=g;older=g;
        unsigned cadence=6+(id%3)*3;unsigned keys=Fire,frames=0,deaths=0,bombs=0,lastWave=1;
        stats[1].entries++;
        for(;frames<60*360 && g.screen!=Over && g.wave<13;++frames) {
            if(frames%cadence==0) {
                static Game observation;observation=perceived;
                Decision d=choose(perceived,id,cadence,g.px,g.py);older=observation;perceived=g;keys=d.keys;
                // Small lapses in aim, never a change to game state.
                if(rnd(40)==0)keys&=~(Left|Right|Up|Down);
                if(g.screen==Evolution)keys=0;
            }
            unsigned before=g.lives,charges=g.bombs,wave=g.wave;
            g.update(keys);
            if(g.lives<before) {++deaths;++stats[wave].deaths;stats[wave].body+=(g.events&BodyImpact)!=0;stats[wave].bolt+=(g.events&BulletImpact)!=0;}
            if(g.bombs<charges) {++bombs;++stats[wave].bombs;}
            if(g.screen==Play && !g.transition) {
                auto &s=stats[wave];++s.frames;unsigned n=g.liveBullets();s.bullets+=n;if(n>s.peak)s.peak=n;
            }
            if(g.wave!=lastWave) {stats[lastWave].clears++;stats[g.wave].entries++;lastWave=g.wave;}
        }
        bool win=g.wave>=13;finished+=win;evolved+=g.wave>=9;totalDeaths+=deaths;
        if(g.wave>highest)highest=g.wave;
        printf("%s{\"id\":%u,\"seed\":%u,\"reaction_ms\":%u,\"wave\":%u,\"seconds\":%.2f,\"deaths\":%u,\"bombs\":%u,\"score\":%lu,\"completed\":%s}",
            id?",\n":"",id,0xCAB000+id*7919,cadence*1000/60,g.wave,frames/60.,deaths,bombs,(unsigned long)g.score,win?"true":"false");
    }
    printf("\n],\"completed\":%u,\"full_evolution\":%u,\"highest_wave\":%u,\"total_deaths\":%u,\"waves\":[\n",finished,evolved,highest,totalDeaths);
    for(unsigned wave=1;wave<=12;++wave) {
        auto &s=stats[wave];
        printf("%s{\"wave\":%u,\"phase\":%u,\"entries\":%u,\"clears\":%u,\"deaths\":%u,\"body_impacts\":%u,\"bullet_impacts\":%u,\"bombs\":%u,\"mean_bullets\":%.2f,\"peak_bullets\":%u,\"play_seconds\":%.2f}",wave>1?",\n":"",wave,Game::phaseFor(wave),s.entries,s.clears,s.deaths,s.body,s.bolt,s.bombs,s.frames?double(s.bullets)/s.frames:0,s.peak,s.frames/60.);
    }
    printf("\n]}\n");
    return 0;
}
