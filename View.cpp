#include <CHGame.h>
#include <CHGfx_font.h>
#include "View.h"

namespace view {
namespace {
enum Color { Black,Grid,Dim,White,Cyan,Pale,Pink,Rose,Gold,Orange,Violet,Shadow,Red,Green,Navy,Blue };
unsigned applied=99;
void palette(unsigned phase,bool awakening=false) {
    unsigned key=phase+(awakening?8:0);
    if(applied==key)return;applied=key;
    const uint16_t neon[16]={0x001,0x113,0x456,0xFFF,0x3DE,0xBFF,0xF48,0xFAD,
        0xFE8,0xF94,0xA6E,0x213,0xF55,0x8FA,0x124,0x58B};
    uint16_t pal[16];
    for(unsigned i=0;i<16;++i)pal[i]=phase?neon[i]:(i==Black?0x000:i==Grid?0x111:i==Dim?0x555:i==Shadow||i==Navy?0x222:0xDDD);
    if(awakening) {
        pal[Cyan]=0x6CD;pal[Pale]=0xACD;pal[Pink]=0xD8B;
        pal[Orange]=0xDC9;pal[Gold]=0xDC9;
    }
    // pal::init converts the library's RGB444 palette convention to RGB565.
    pal::init(pal);pal::setCycling(false);
}
void center(int y,const char *s,unsigned c) {text35((128-text35Width(s))/2,y,s,c);}
void number(int x,int y,unsigned n,unsigned c) {char text[12];fmtInt(text,n);text35(x,y,text,c);}
void big(int y,const char *s,unsigned c,unsigned scale) {
    gfx_textScaled((128-gfx_textWidthScaled(s,scale))/2,y,s,c,scale);
}
bool tutorial(const bloom::Game &g) {
    return g.screen==bloom::Evolution && (g.wave==3 || g.gunNotice==bloom::FocusUnlocked);
}
unsigned sceneTick(const bloom::Game &g) {
    // Only the words animate over the new wave's frozen playfield.
    return tutorial(g)?g.tick-((g.wave==3?135:180)-g.transition):g.tick;
}
void zoomWords(int cy,const char *s,unsigned color,int scale) {
    unsigned length=0;while(s[length])++length;
    int width=((int(length)*6-1)*scale+128)/256;
    int x=(128-width)/2,y=cy-(7*scale+128)/512;
    // Fractional nearest-neighbor scaling of the existing CHGfx glyphs.
    // Paint the outline first so adjacent letter blocks never erase ink.
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<length;++i) {
        const uint8_t *glyph=chgfx_font5x7+(s[i]-32)*5;
        for(int col=0;col<5;++col)for(int row=0;row<7;++row)if(glyph[col]&(1u<<row)) {
            int left=x+((int(i)*6+col)*scale+128)/256;
            int right=x+((int(i)*6+col+1)*scale+128)/256;
            int top=y+(row*scale+128)/256,bottom=y+((row+1)*scale+128)/256;
            gfx_fillRect(left-(pass?0:1),top-(pass?0:1),right-left+(pass?0:2),bottom-top+(pass?0:2),pass?color:Black);
        }
    }
}
void tutorialWords(const bloom::Game &g) {
    int elapsed=(g.wave==3?135:180)-g.transition;
    int t=elapsed<90?elapsed:90;
    int zoom=288-32*t*(180-t)/8100;
    if(g.wave==3) {
        zoomWords(49,"MOVE UP",Cyan,512*zoom/256);
        zoomWords(73,"AND DOWN!",White,512*zoom/256);
    } else {
        zoomWords(49,"FOCUS FIRE",Cyan,450*zoom/256);
        zoomWords(73,"WITH SELECT!",White,380*zoom/256);
    }
}
void stars(const bloom::Game &g,bool moving=true) {
    gfx_clear(Black);
    const unsigned t=moving?sceneTick(g):0;
    for(unsigned i=0;i<32;++i) {
        int x=(i*37+19)%128,y=(i*i*13+(t/(i%3+1)))%116+10;
        gfx_pixel(x,y,i%7?Grid:Dim);
        if(g.phase>=2 && i%4==0)gfx_vline(x,y,2+g.phase,Grid);
    }
    if(g.phase>=2) {
        for(unsigned j=0;j<3;++j) {
            int radius=(j*30+t/5)%94;
            int lastX=0,lastY=0;
            for(unsigned k=0;k<=8;++k) {
                unsigned a=k*4+t/80;
                int x=64+bloom::sine(a+8)*radius/64,y=53+bloom::sine(a)*radius/64;
                if(k)gfx_line(lastX,lastY,x,y,Grid);
                lastX=x;lastY=y;
            }
        }
    }
}
void alien(int x,int y,unsigned variant,unsigned color,bool anim,unsigned turn=0) {
    static const uint16_t pixels[3][7]={
        {0x044,0x028,0x07C,0x0D6,0x1FF,0x155,0x082},
        {0x038,0x07C,0x0FE,0x0AA,0x0FE,0x044,0x082},
        {0x082,0x044,0x1FF,0x1AB,0x0FE,0x054,0x0AA}
    };
    for(int row=0;row<7;++row) {
        unsigned bits=pixels[variant%3][row];
        if(anim && row==6)bits=variant%2?0x044:0x145;
        for(int col=0;col<9;++col)if(bits&(1u<<col)) {
            int dx=col-4,dy=row-3;
            if(turn==1)gfx_pixel(x-dy,y+dx,color);
            else if(turn==2)gfx_pixel(x-dx,y-dy,color);
            else if(turn==3)gfx_pixel(x+dy,y-dx,color);
            else gfx_pixel(x+dx,y+dy,color);
        }
    }
}
void ship(int x,int y,unsigned phase,bool focused,unsigned tick,unsigned color=White) {
    if(phase) {
        gfx_line(x-4,y+3,x-4,y+6,phase>=3?Pink:Cyan);
        gfx_line(x+4,y+3,x+4,y+6,phase>=3?Pink:Cyan);
    }
    gfx_hline(x-4,y+3,9,color);gfx_hline(x-3,y+1,7,color);
    gfx_vline(x-1,y-3,7,color);gfx_vline(x+1,y-3,7,color);
    gfx_vline(x,y-5,9,color);
    gfx_pixel(x,y,phase?Cyan:Black);
    gfx_vline(x,y+4,2+tick%3,phase?Orange:Dim);
    if(focused) {
        gfx_circle(x,y,6,Cyan);gfx_pixel(x,y,White);
        gfx_pixel(x-7,y,Dim);gfx_pixel(x+7,y,Dim);
    }
}
void boss(const bloom::Enemy &e,const bloom::Game &g) {
    int x=e.x/bloom::Q,y=e.y/bloom::Q;
    unsigned color=e.flash?White:g.phase>=3?Pink:Cyan;
    for(unsigned i=0;i<8;++i) {
        unsigned a=i*4+e.age/12;
        int dx=bloom::sine(a+8),dy=bloom::sine(a);
        gfx_line(x+dx*6/64,y+dy*4/64,x+dx*15/64,y+dy*9/64,color);
        gfx_rect(x+dx*15/64-2,y+dy*9/64-2,5,5,color);
    }
    gfx_fillCircle(x,y,6,Shadow);gfx_circle(x,y,6,color);
    gfx_rect(x-3,y-3,7,7,color);gfx_pixel(x,y,White);
}
void field(const bloom::Game &g) {
    gfx_setClip(0,bloom::FIELD_TOP,128,bloom::FIELD_BOTTOM-bloom::FIELD_TOP+1);
    for(const auto &e:g.enemies)if(e.kind) {
        int x=e.x/bloom::Q,y=e.y/bloom::Q;
        if(e.kind==bloom::Boss)boss(e,g);
        else if(e.kind==bloom::Invader) {
            unsigned color=e.flash?White:e.flight==bloom::Charging?(e.age/4%2?Pink:White):
                e.flight==bloom::Diving?Pink:e.flight==bloom::Looping?Cyan:
                e.flight==bloom::Returning?Pale:White;
            alien(x,y,e.seed/5,color,(e.age/24)&1,e.flight==bloom::Looping?(e.age/12)%4:0);
            if(g.wave==2 && !e.flash)gfx_pixel(x,y,Cyan);
            if(e.flight>bloom::Charging) {
                int sx=23+(e.seed%5)*20+g.formation/bloom::Q,sy=24+(e.seed/5)*13+g.formationY/bloom::Q;
                gfx_pixel(sx-2,sy,Dim);gfx_pixel(sx+2,sy,Dim);
            }
            if(e.flight==bloom::Diving) {
                gfx_vline(x,y-8,3,Cyan);
                if(e.age<20) {
                    int tx=e.target/bloom::Q;
                    gfx_vline(tx-3,109,5,Pink);gfx_vline(tx+3,109,5,Pink);
                }
            }
        }
        else if(e.kind==bloom::Turret) {
            gfx_rect(x-5,y-5,11,11,e.flash?White:Violet);
            gfx_rect(x-3,y-3,7,7,e.flash?White:Pink);gfx_pixel(x,y,White);
        } else {
            unsigned c=e.flash?White:e.kind==bloom::Weaver?Pink:Cyan;
            gfx_line(x,y+5,x-5,y-3,c);gfx_line(x-5,y-3,x,y,c);
            gfx_line(x,y,x+5,y-3,c);gfx_line(x+5,y-3,x,y+5,c);
            gfx_pixel(x,y+2,White);
        }
    }
    for(const auto &b:g.shots)if(b.kind) {
        int x=b.x/bloom::Q,y=b.y/bloom::Q;
        gfx_vline(x,y,4,g.phase||g.wave==2?Cyan:White);gfx_pixel(x,y,White);
    }
    for(const auto &s:g.sparks)if(s.life && !tutorial(g)) {
        unsigned color=s.life>14?White:s.color==3?Pink:Gold;
        gfx_pixel(s.x/bloom::Q,s.y/bloom::Q,color);
        if(s.life>20)gfx_pixel(s.x/bloom::Q+1,s.y/bloom::Q,color);
    }
    // Hostile bullets are drawn after effects and shots so their silhouettes
    // remain visible. Bright center, distinct shape, dark outline.
    for(const auto &b:g.bullets)if(b.kind) {
        int x=b.x/bloom::Q,y=b.y/bloom::Q;
        unsigned kind=b.kind&127,color=kind==2?Pink:kind==3?Gold:Orange;
        if(kind==2) {
            gfx_fillCircle(x,y,3,Black);gfx_circle(x,y,2,color);gfx_pixel(x,y,White);
        } else if(kind==3) {
            gfx_hline(x-2,y,5,Black);gfx_vline(x,y-3,7,Black);
            gfx_vline(x,y-2,5,color);gfx_pixel(x,y,White);
        } else {
            gfx_fillRect(x-1,y-2,3,5,Black);gfx_vline(x,y-1,3,color);gfx_pixel(x,y,White);
        }
    }
    if(!g.invincible || (g.invincible/5)%2==0)
        ship(g.px/bloom::Q,g.py/bloom::Q,g.phase,g.focus,sceneTick(g));
    if(g.bombFlash) {
        int radius=(bloom::BOMB_FRAMES-g.bombFlash)*6;
        gfx_circle(g.px/bloom::Q,g.py/bloom::Q,radius,Cyan);
        gfx_circle(g.px/bloom::Q,g.py/bloom::Q,radius+3,White);
    }
    gfx_resetClip();
}
void hud(const bloom::Game &g) {
    gfx_fillRect(0,0,128,11,Black);gfx_hline(0,9,128,Grid);
    number(2,2,g.score,White);text35(81,2,"X",Dim);number(87,2,g.combo,Gold);
    text35(103,2,"W",Dim);number(109,2,g.wave,Cyan);
    gfx_fillRect(0,bloom::FIELD_BOTTOM+1,128,8,Black);gfx_hline(0,bloom::FIELD_BOTTOM+1,128,Grid);
    for(unsigned i=0;i<g.lives;++i) {
        gfx_hline(3+i*6,125,5,White);gfx_vline(5+i*6,122,4,White);
    }
    text35(37,122,"B",Dim);
    for(unsigned i=0;i<g.bombs;++i) {
        int x=45+i*6;
        gfx_circle(x,124,2,Cyan);gfx_pixel(x,124,White);
    }
    const char *name=g.wave==2?"AWAKEN":bloom::phaseName(g.phase);
    text35(126-text35Width(name),122,name,g.phase>=3?Pink:g.wave==2?Cyan:Dim);
    for(const auto &e:g.enemies)if(e.kind==bloom::Boss) {
        gfx_hline(10,13,108,Shadow);gfx_hline(10,13,int(e.hp)*108/e.maxHp,Pink);
        break;
    }
    if(g.transition && g.screen==bloom::Play) {
        gfx_fillRect(16,49,96,25,Black);gfx_hline(20,49,88,Grid);
        center(54,g.wave%4==0?"WARNING / BLOOM CORE":g.wave==2?"FORMATION UNSTABLE":"NEXT SIGNAL",g.wave%4==0?Pink:g.wave==2?Cyan:Dim);
        text35(43,65,"WAVE",White);number(65,65,g.wave,Cyan);
    }
}
void panel(int y,int h) {
    gfx_fillRect(8,y,112,h,Black);gfx_rect(8,y,112,h,Grid);
    gfx_hline(8,y,24,Cyan);gfx_hline(96,y+h-1,24,Pink);
}
void title(const bloom::Game &g) {
    // A small evolving orbit frames the title without obscuring the prompt.
    for(unsigned i=0;i<32;++i) {
        unsigned a=i+g.tick/5;
        int r=30+(bloom::sine(i*3+g.tick/6)+64)/6;
        int x=64+bloom::sine(a+8)*r/64,y=29+bloom::sine(a)*r/192;
        gfx_pixel(x,y,i&1?Pink:Cyan);
    }
    alien(64,28,(g.tick/90)%3,White,(g.tick/20)%2);
    big(43,"VOID",White,2);big(62,"BLOOM",Pink,3);
    center(91,"A SIGNAL THAT NEVER ENDS",Dim);
    if((g.tick/40)%2==0)center(105,"A / START   EVOLVE",Cyan);
    center(117,"B CONTROLS   SELECT SOUND",Dim);
    text35(2,3,"HI",Dim);number(14,3,g.best,White);
    text35(102,3,g.muted?"MUTE":"SOUND",Dim);
}
void help() {
    center(8,"EVOLVE OR FALL",Pink);
    const char *lines[]={"PAD: MOVE","A: HOLD TO FIRE","B: PANIC BOMB","SELECT: WIDE / NARROW","START: PAUSE","","WAVE 3: FREE FLIGHT","WAVE 5: GUN TOGGLE + GRAZE","WAVE 7: FIVE-WAY FIRE","WAVE 9: INFINITY","","NEAR MISSES BUILD SCORE","40 GRAZES: EXTRA BOMB"};
    for(unsigned i=0;i<13;++i)center(22+i*7,lines[i],i<5?White:i==11||i==12?Gold:Dim);
    center(121,"A / B BACK",Cyan);
}
void evolution(const bloom::Game &g) {
    if(tutorial(g)) {tutorialWords(g);return;}
    for(unsigned i=0;i<16;++i) {
        unsigned a=i*2+g.tick/3;int r=20+int(g.transition%40);
        gfx_pixel(64+bloom::sine(a+8)*r/64,59+bloom::sine(a)*r/64,i%2?Pink:Cyan);
    }
    panel(34,64);center(41,"EVOLUTION",Dim);
    big(53,bloom::phaseName(g.phase),g.phase>=3?Pink:Cyan,1);
    center(72,bloom::unlock(g.phase),White);
    center(84,g.phase==1?"UP / DOWN NOW ONLINE":g.phase==2?"DODGE CLOSE. EARN BOMBS.":g.phase==3?"SELECT: WIDE / NARROW":"SURVIVE. REPEAT. BLOOM.",Gold);
    center(111,"WEAPON +1   BOMB +1",Dim);
}
void gunNotice(const bloom::Game &g) {
    if(g.gunNotice!=bloom::SwitchedGun || !g.gunNoticeTime)return;
    int elapsed=72-g.gunNoticeTime;
    int slide=elapsed<10?(10-elapsed)*13:g.gunNoticeTime<10?-(10-g.gunNoticeTime)*13:0;
    int bounce=bloom::sine(elapsed/2)/40;
    const char *lines[]={g.focus?"NARROW":"SPREAD","FIRE!"};
    for(unsigned i=0;i<2;++i) {
        int x=(128-gfx_textWidthScaled(lines[i],2))/2-slide,y=20+i*16+bounce;
        gfx_textScaled(x+1,y+1,lines[i],Black,2);
        gfx_textScaled(x,y,lines[i],i?White:Cyan,2);
    }

}
}
void draw(const bloom::Game &g) {
    palette(g.screen==bloom::Title||g.screen==bloom::Help?4:g.phase,
        g.wave==2 && g.screen!=bloom::Title && g.screen!=bloom::Help);
    stars(g,g.screen!=bloom::Paused);
    if(g.screen==bloom::Title) {title(g);return;}
    if(g.screen==bloom::Help) {help();return;}
    field(g);hud(g);
    if(g.screen==bloom::Evolution)evolution(g);
    else if(g.screen==bloom::Play)gunNotice(g);
    else if(g.screen==bloom::Paused) {
        panel(34,61);big(42,"PAUSED",White,1);
        center(58,"START / A RESUME",Cyan);center(70,"B END RUN",Dim);
        center(82,g.muted?"SELECT: SOUND ON":"SELECT: SOUND OFF",Dim);
    } else if(g.screen==bloom::Over) {
        panel(29,75);center(37,"SIGNAL LOST",Pink);
        center(51,"SCORE",Dim);char score[12];fmtInt(score,g.score);big(60,score,White,1);
        text35(26,75,"WAVE",Dim);number(47,75,g.wave,Cyan);
        text35(69,75,"GRAZE",Dim);number(94,75,g.graze,Gold);
        center(91,"A RETRY   B TITLE",Cyan);
        center(111,"EVOLUTION IS INEVITABLE",Dim);
    }
}
}
