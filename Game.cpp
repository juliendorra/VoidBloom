#include "Game.h"

namespace bloom {
namespace {
int absval(int v) { return v<0?-v:v; }
int clamp(int v,int lo,int hi) { return v<lo?lo:v>hi?hi:v; }
// Sixteen substeps between the existing sine samples keep the first loop
// smooth without floating point, new tables, or a per-frame allocation.
int curve(unsigned angle) {
    unsigned a=angle>>4,t=angle&15;
    return (sine(a)*(16-int(t))+sine(a+1)*int(t))/16;
}
bool close(int x,int y,int a,int b,int radius) {
    const int dx=(x-a)/Q,dy=(y-b)/Q;
    return dx*dx+dy*dy<=radius*radius;
}
void clear(Bullet *pool,unsigned size) { for(unsigned i=0;i<size;++i)pool[i].kind=0; }
}
int sine(unsigned a) {
    static const int8_t table[16]={0,12,24,35,45,53,59,62,64,62,59,53,45,35,24,12};
    a&=31; return a<16?table[a]:-table[a-16];
}
const char *phaseName(unsigned p) {
    static const char *const names[]={"ORIGIN","VECTOR","SURGE","NOVA","INFINITY"};
    return names[p<5?p:4];
}
const char *unlock(unsigned p) {
    static const char *const names[]={"LEFT / RIGHT + FIRE","FREE FLIGHT + TWIN SHOT","SPREAD SHOT + GRAZE","FIVE-WAY + SPIRALS","FULL POWER. NO LIMIT."};
    return names[p<5?p:4];
}
unsigned Game::phaseFor(unsigned w) { return w>8?4:(w-1)/2; }
unsigned Game::random(unsigned n) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return n?rng%n:0; }
unsigned Game::liveBullets() const { unsigned n=0;for(const auto &b:bullets)n+=b.kind!=0;return n; }
unsigned Game::bulletLimit() const {
    // Encounter budgets, independent of the fixed storage pool. Keep the
    // first boss readable; later stages gradually earn denser patterns.
    static const uint8_t limits[]={4,6,10,12,24,28,40,56,72,80,88,104};
    return wave<=12?limits[wave?wave-1:0]:128;
}
unsigned Game::liveEnemies() const { unsigned n=0;for(const auto &e:enemies)n+=e.kind!=0;return n; }
void Game::begin(uint32_t seed) {
    // Reset the existing object in place: a second pool must never live on
    // the Rev0's 2 KB stack. Keep saved settings and high score.
    for(auto &e:enemies)e.kind=0;
    for(auto &s:sparks)s.life=0;
    clear(bullets,BULLETS);clear(shots,SHOTS);
    screen=Play;tick=score=age=transition=events=comboClock=graze=0;
    extraAt=20000;rng=seed?seed:1;wave=1;phase=0;lives=3;bombs=3;combo=1;
    chain=cooldown=bombFlash=shake=previous=beat=0;focus=false;
    px=64*Q;py=111*Q;invincible=90;startWave();
}
void Game::spawn(unsigned kind,int x,int y,unsigned seed) {
    for(auto &e:enemies)if(!e.kind) {
        e=Enemy{};e.kind=kind;e.x=x*Q;e.y=y*Q;e.home=e.x;e.seed=seed;
        static const uint16_t bossHp[]={0,132,210,340,580};
        unsigned hp=kind==Boss?bossHp[phase]+(wave>12?(wave-12)*8:0):
            kind==Turret?(phase==4?22:phase==3?14:7+phase):
            kind==Weaver?(phase==4?9:2+phase):kind==Invader?1:phase==4?5:phase;
        // Endless difficulty rises in bounded steps. Counts and velocities stay safe.
        if(wave>12)hp+=kind==Boss?(wave>60?120:wave*2):(wave>30?3:wave/10);
        e.hp=e.maxHp=hp;return;
    }
}
void Game::startWave() {
    for(auto &e:enemies)e.kind=0;
    clear(bullets,BULLETS);clear(shots,SHOTS);
    age=0;spawned=0;remaining=0;formation=formationY=0;direction=1;
    if(wave%4==0) { spawn(Boss,64,-14,wave);waveTotal=1;spawned=1; }
    else if(phase==0) {
        waveTotal=15;
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<5;++col)
            spawn(Invader,23+col*20,24+row*13,row*5+col);
        spawned=waveTotal;
    } else waveTotal=12+phase*3;
    remaining=waveTotal;
    if(transition==0)transition=55;
}
void Game::points(unsigned n) {
    score+=n;if(score>best)best=score;
    if(score>=extraAt) {
        if(lives<5)++lives;
        if(bombs<5)++bombs;
        extraAt+=20000;events|=Clear;
    }
}
void Game::sparksAt(int x,int y,unsigned color,unsigned count) {
    for(auto &s:sparks)if(!s.life && count) {
        const unsigned a=random(32);const int speed=1+random(3);
        s.x=x;s.y=y;s.vx=sine(a+8)*speed/3;s.vy=sine(a)*speed/3;
        s.life=12+random(15);s.color=color;--count;
    }
}
void Game::damage(Enemy &e,unsigned amount) {
    if(!e.kind)return;
    e.flash=3;
    if(e.hp>amount) {e.hp-=amount;return;}
    const bool boss=e.kind==Boss;
    points((boss?1000:e.kind==Turret?150:60)*combo);
    if(++chain>=5) {chain=0;if(combo<8)++combo;}
    comboClock=180;
    sparksAt(e.x,e.y,boss?3:2,boss?24:5);
    e.kind=0;if(remaining)--remaining;
    events|=Kill;shake=boss?14:2;
    if(boss) {
        clear(bullets,BULLETS);if(bombs<5)++bombs;
        points(500*combo);events|=Clear;
    }
}
void Game::emit(int x,int y,int vx,int vy,unsigned kind) {
    if(liveBullets()<bulletLimit())emitUnchecked(x,y,vx,vy,kind);
}
void Game::emitUnchecked(int x,int y,int vx,int vy,unsigned kind) {
    // Never replace a live bullet: existing patterns cannot teleport.
    for(auto &b:bullets)if(!b.kind) {
        b.x=x;b.y=y;b.vx=vx;b.vy=vy;b.kind=kind;return;
    }
}
void Game::ring(int x,int y,unsigned count,unsigned angle,int speed,unsigned kind) {
    // Defer an entire volley at capacity, rather than clip one side of a
    // pattern. Existing bullets always keep their positions and velocity.
    if(!count || count>bulletLimit() || liveBullets()>bulletLimit()-count)return;
    for(unsigned i=0;i<count;++i) {
        unsigned a=angle+i*32/count;
        emitUnchecked(x,y,sine(a+8)*speed/Q,sine(a)*speed/Q,kind);
    }
}
void Game::aimed(int x,int y,unsigned count,int speed) {
    const unsigned total=(count/2)*2+1;
    if(total>bulletLimit() || liveBullets()>bulletLimit()-total)return;
    int dx=px-x,dy=py-y;
    int ax=absval(dx),ay=absval(dy),length=ax>ay?ax+ay/2:ay+ax/2;
    if(length<Q)length=Q;
    int vx=dx*speed/length,vy=dy*speed/length;
    for(int i=-int(count/2);i<=int(count/2);++i)
        emitUnchecked(x,y,vx-vy*i/3,vy+vx*i/3,phase>=3?3:1);
}
void Game::fire() {
    if(cooldown)return;
    cooldown=phase==0?13:phase<3?10:7;
    unsigned count=phase==0?1:phase==1?2:phase==2?3:5;
    for(unsigned j=0;j<count;++j)for(auto &b:shots)if(!b.kind) {
        const int offset=phase==1?(j?2:-2):0;
        b.x=px+offset*Q;b.y=py-5*Q;b.vy=-4*Q;
        b.vx=count<3?0:(int(j)-int(count/2))*(focus?18:42);
        b.kind=1;break;
    }
    events|=Shot;
}
void Game::bomb() {
    if(!bombs || screen!=Play)return;
    --bombs;points(liveBullets()*5);clear(bullets,BULLETS);
    for(auto &e:enemies)if(e.kind)damage(e,e.kind==Boss?24:20);
    invincible=100;bombFlash=30;shake=12;events|=Burst;
}
void Game::hit() {
    if(invincible || screen!=Play)return;
    sparksAt(px,py,3,16);--lives;combo=1;chain=0;comboClock=0;shake=14;
    clear(bullets,BULLETS);invincible=150;events|=Hurt;
    if(!lives) {screen=Over;transition=60;return;}
    px=64*Q;py=111*Q;
}
void Game::sortieTick(Enemy &e) {
    const int slotX=(23+int(e.seed%5)*20)*Q+formation;
    const int slotY=(24+int(e.seed/5)*13)*Q+formationY;
    if(e.flight==Charging) {
        if(e.age>=20) {
            e.flight=Looping;e.age=0;e.home=e.x;e.originY=e.y;
        }
    } else if(e.flight==Looping) {
        const int side=e.home<64*Q?1:-1;
        unsigned angle=e.age*8;
        e.x=e.home+side*(64-curve(angle+128))*12;
        e.y=e.originY-curve(angle)*12;
        if(e.age>=48) {
            e.flight=Diving;e.age=0;e.home=e.x;e.originY=e.y;
            // Lock the aim once. The dive never homes in on a dodging pilot.
            e.target=clamp(px,10*Q,118*Q);
            aimed(e.x,e.y,1,60);
        }
    } else if(e.flight==Diving) {
        const int t=e.age>72?72:e.age;
        e.x=e.home+(int(e.target)-e.home)*t/72;
        e.y=e.originY+(136*Q-int(e.originY))*t/72;
        if(e.age>=72) {
            // Re-enter only after the whole sprite has left the bottom.
            e.flight=Returning;e.age=0;e.home=e.x;e.y=-10*Q;
        }
    } else if(e.flight==Returning) {
        const int t=e.age>64?64:e.age;
        e.x=e.home+(slotX-e.home)*t/64;
        e.y=-10*Q+(slotY+10*Q)*t/64;
        if(e.age>=64) {e.flight=InFormation;e.x=slotX;e.y=slotY;e.age=0;}
    }
}
void Game::enemiesTick() {
    if(!phase) {
        unsigned left=128*Q,right=0;
        for(const auto &e:enemies)if(e.kind) {
            // Keep the empty formation slot inside the arena while its
            // invader is away, even when it is the last surviving enemy.
            const int x=e.flight>Charging?(23+int(e.seed%5)*20)*Q+formation:e.x;
            if(unsigned(x)<left)left=x;
            if(unsigned(x)>right)right=x;
        }
        int dx=direction*(11+int(15-liveEnemies())*2+(wave>1?4:0));
        int dy=0;if((direction>0 && right>119*Q)||(direction<0 && left<8*Q)) {direction=-direction;dx=-dx;dy=5*Q;}
        formation+=dx;formationY+=dy;
        for(auto &e:enemies)if(e.kind) {
            ++e.age;if(e.flash)--e.flash;
            if(e.flight>Charging) {sortieTick(e);continue;}
            e.x+=dx;e.y+=dy;
            if(e.flight==Charging)sortieTick(e);
            if(e.y>103*Q) {
                invincible=0;events|=FormationImpact;hit();
                if(screen==Play) {transition=80;startWave();}
                return;
            }
        }
        unsigned parked=0,flying=0;
        for(const auto &e:enemies)if(e.kind) {
            if(e.flight==InFormation)++parked;else ++flying;
        }
        if(wave==2 && age>=90 && (age-90)%110==0 && !flying && parked) {
            unsigned choose=random(parked);
            for(auto &e:enemies)if(e.kind && e.flight==InFormation && choose--==0) {
                e.flight=Charging;e.age=0;break;
            }
        }
        if(age>60 && age%64==0 && parked) {
            unsigned choose=random(parked);
            for(const auto &e:enemies)if(e.kind && e.flight==InFormation && choose--==0) {emit(e.x,e.y+4*Q,0,60);break;}
        }
        return;
    }
    if(spawned<waveTotal && age%(phase==1?42:32)==1) {
        unsigned kind=spawned%5==4?Turret:spawned%3==1?Weaver:Scout;
        spawn(kind,14+random(100),-8,spawned*7+wave);++spawned;
    }
    for(auto &e:enemies)if(e.kind) {
        ++e.age;if(e.flash)--e.flash;
        if(e.kind==Boss) {
            e.x=64*Q+sine(e.age/8)*28;e.y=e.age<100?(-14*Q+int(e.age)*28):30*Q;
            if(e.age<120)continue;
            const bool firstBoss=wave==4;
            const unsigned attackAge=e.age-120;
            // Two seconds of attack, then one second for the first boss's
            // old bullets to spread out before the next pattern begins.
            if(firstBoss && attackAge%180>=120)continue;
            unsigned pattern=((firstBoss?attackAge:e.age)/180)%3;
            unsigned interval=firstBoss?60:phase<3?42:phase==3?24:18;
            if(pattern==0 && e.age%interval==0)ring(e.x,e.y,firstBoss?8:phase<3?10:phase==3?16:24,e.age/5,phase<3?55:68,2);
            if(pattern==1 && e.age%(firstBoss?54:phase<3?38:22)==0)aimed(e.x,e.y,phase<3?3:phase==3?5:7,phase<3?75:90);
            if(pattern==2 && e.age%(firstBoss?18:phase<3?12:phase==3?6:5)==0) {
                ring(e.x,e.y,phase<3?2:phase==3?4:6,e.age/3,phase<3?65:80,3);
                if(e.age%60==0)aimed(e.x,e.y,3,80);
            }
            continue;
        }
        if(e.kind==Scout) {
            e.y+=phase<3?34:46;e.x=e.home+sine(e.age/4+e.seed)*12;
        } else if(e.kind==Weaver) {
            e.y+=28+phase*5;e.x=64*Q+sine(e.age/3+e.seed)*47;
        } else {
            e.y+=e.age<75?40:e.age>270?45:0;
            e.x=e.home+sine(e.age/8+e.seed)*6;
        }
        // Early flights visibly peel away before the pilot's band. Later
        // phases commit farther down-screen, gradually adding body pressure.
        const int peelAt=phase==1?72:phase==2?82:phase==3?88:92;
        if(e.kind!=Turret && e.y>peelAt*Q) {
            const int speed=phase==1?192:phase==2?160:phase==3?128:96;
            // Accumulate outward travel in home: the sine motion remains
            // smooth instead of snapping back to its original lane next tick.
            if(e.kind==Weaver && e.y-(28+phase*5)<=peelAt*Q)
                e.home=e.x-sine(e.age/3+e.seed)*16;
            int side=e.seed&1?1:-1;e.home+=side*speed;e.x+=side*speed;
            if(e.kind==Weaver)e.x=e.home+sine(e.age/3+e.seed)*16;
        }
        if(e.y>5*Q && e.y<83*Q) {
            unsigned interval=e.kind==Turret?(phase==1?95:phase==2?82:phase==3?65:54):
                (phase==1?140:phase==2?112:phase==3?88:70);
            if((e.age+e.seed)%interval==0) {
                if(e.kind==Turret && phase>=2)ring(e.x,e.y,phase>=3?10:6,e.age/4,52+phase*5);
                else aimed(e.x,e.y,phase>=3?3:1,65+phase*6);
            }
        }
        if(e.y>136*Q || e.x<-10*Q || e.x>138*Q) {e.kind=0;if(remaining)--remaining;combo=1;chain=0;}
    }
}
void Game::shotsTick() {
    for(auto &b:shots)if(b.kind) {
        b.x+=b.vx;b.y+=b.vy;
        if(b.y<8*Q || b.x<-4*Q || b.x>132*Q) {b.kind=0;continue;}
        for(auto &e:enemies)if(e.kind && e.y>10*Q) {
            const int rx=e.kind==Boss?15:5,ry=e.kind==Boss?9:5;
            if(absval(b.x-e.x)<=rx*Q && absval(b.y-e.y)<=ry*Q) {
                b.kind=0;damage(e,phase==4?2:1);break;
            }
        }
    }
}
void Game::bulletsTick() {
    for(auto &b:bullets)if(b.kind) {
        b.x+=b.vx;b.y+=b.vy;
        if(b.x<-10*Q || b.x>138*Q || b.y<-12*Q || b.y>136*Q) {b.kind=0;continue;}
        const int radius=(b.kind&127)==2?3:2;
        if(!invincible && close(b.x,b.y,px,py,radius)) {
            events|=BulletImpact;hit();break; // hit clears the pool; no stale references after respawn.
        }
        if(phase>=2 && !invincible && !(b.kind&128) && close(b.x,b.y,px,py,8)) {
            b.kind|=128;++graze;points(5*combo);events|=Graze;
            if(graze%40==0 && bombs<5)++bombs;
        }
    }
    if(!invincible)for(const auto &e:enemies)if(e.kind && close(e.x,e.y,px,py,e.kind==Boss?15:6)) {events|=BodyImpact;hit();break;}
}
void Game::effectsTick() {
    if(bombFlash)--bombFlash;if(shake)--shake;
    for(auto &s:sparks)if(s.life) {--s.life;s.x+=s.vx;s.y+=s.vy;}
}
void Game::advanceWave() {
    points(100*wave);unsigned was=phase;
    if(wave<999)++wave;phase=phaseFor(wave);
    if(phase!=was) {
        screen=Evolution;transition=135;events|=Evolve;
        if(bombs<5)++bombs;invincible=120;
    } else transition=70;
    startWave();
}
void Game::update(uint8_t keys) {
    events=0;const uint8_t pressed=keys&~previous;previous=keys;
    if(screen==Title) {
        ++tick;
        if(pressed&Focus) {back=Title;screen=Help;}
        else if(pressed&(Fire|Start))begin(rng^tick);
        else if(pressed&Bomb)muted=!muted;
        return;
    }
    if(screen==Help) {if(pressed&(Fire|Focus|Start))screen=back;return;}
    if(screen==Paused) {
        if(pressed&(Start|Fire))screen=Play;
        else if(pressed&Focus) {screen=Title;transition=0;}
        else if(pressed&Bomb)muted=!muted;
        return;
    }
    if(screen==Over) {
        effectsTick();if(transition)--transition;
        else if(pressed&(Fire|Start))begin(rng^tick);
        else if(pressed&Focus)screen=Title;
        return;
    }
    if(screen==Evolution) {
        ++tick;effectsTick();
        if(transition)--transition;
        if(!transition || (transition<75 && (pressed&Fire))) {screen=Play;transition=45;}
        return;
    }
    if(pressed&Start) {screen=Paused;return;}
    ++tick;effectsTick();
    if(transition) {--transition;return;}
    ++age;focus=(keys&Focus)!=0;if(invincible)--invincible;if(cooldown)--cooldown;
    if(comboClock) {if(!--comboClock) {combo=1;chain=0;}}
    int dx=((keys&Right)!=0)-((keys&Left)!=0),dy=phase?((keys&Down)!=0)-((keys&Up)!=0):0;
    int speed=focus?40:100;if(dx && dy)speed=speed*181/256;
    px=clamp(px+dx*speed,PLAYER_MIN_X*Q,PLAYER_MAX_X*Q);
    py=clamp(py+dy*speed,PLAYER_MIN_Y*Q,PLAYER_MAX_Y*Q);
    if(keys&(Fire|Focus))fire();
    enemiesTick();
    if(screen!=Play || transition)return;
    shotsTick();
    // Resolve bombs before hostile bullets: a same-frame bomb saves the pilot.
    if(pressed&Bomb)bomb();
    bulletsTick();
    if(screen==Play && spawned==waveTotal && !remaining)advanceWave();
    unsigned step=phase==0?24:phase==1?18:phase==2?15:12;
    if(tick%step==0) {beat=(beat+1)&15;events|=Beat;}
}
}
