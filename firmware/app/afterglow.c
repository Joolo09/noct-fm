/* SPDX-License-Identifier: MPL-2.0
 * NOCT / AFTERGLOW. Integer pixel scenes, driven by MIDI and clock telemetry.
 * Two 240px strips; no full-screen buffer, assets or allocation in the timer.
 */
static const int8_t AG_SIN[64]={0,12,25,37,49,60,71,81,90,98,106,112,117,122,125,126,127,126,125,122,117,112,106,98,90,81,71,60,49,37,25,12,0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-122,-125,-126,-127,-126,-125,-122,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12};
static const uint16_t AG_ACCENT[PAGE_COUNT]={RGB(118,241,212),RGB(255,161,205),RGB(190,160,255),RGB(255,190,121),RGB(116,211,255),RGB(122,238,221)};
static struct {
    uint32_t serial,page_at,last;
    uint8_t page,pulse,energy[12];
} glow;
static int ag_sin(unsigned a,int radius){return AG_SIN[a&63u]*radius/127;}
static uint16_t ag_mix(uint16_t a,uint16_t b,unsigned n)
{
    if(n>255u)n=255u;
    unsigned r=(((a>>11)&31u)*(255u-n)+((b>>11)&31u)*n)/255u;
    unsigned g=(((a>>5)&63u)*(255u-n)+((b>>5)&63u)*n)/255u;
    unsigned v=((a&31u)*(255u-n)+(b&31u)*n)/255u;
    return (uint16_t)(r<<11|g<<5|v);
}
static void ag_disc(int x,int y,int radius,uint16_t color)
{
    for(int dy=-radius;dy<=radius;dy++){
        int w=radius;
        while(w*w+dy*dy>radius*radius)w--;
        cv_rect(x-w,y+dy,2*w+1,1,color);
    }
}
static void ag_ring(int x,int y,int rx,int ry,unsigned spin,uint16_t color,int half)
{
    for(unsigned i=0;i<64;i+=2){
        if(half && ((i<32u)!=(half==1)))continue;
        int dx=ag_sin(i+16u,rx),dy=ag_sin(i,ry);
        int nx=ag_sin(i+18u,rx),ny=ag_sin(i+2u,ry);
        cv_line(x+dx,y+dy-dx/5,x+nx,y+ny-nx/5,color);
        if((i+spin)%16u==0)cv_rect(x+dx,y+dy-dx/5,2,1,color);
    }
}
static void ag_tick(uint32_t now,int page)
{
    if(glow.page!=(unsigned)page){glow.page=(uint8_t)page;glow.page_at=now;}
    if(glow.serial!=visual_note_serial){glow.serial=visual_note_serial;glow.pulse=255;}
    else glow.pulse=(uint8_t)(glow.pulse>22u?glow.pulse-22u:0u);
    for(int pc=0;pc<12;pc++){
        int held=0;
        for(int n=pc;n<128;n+=12)held|=ui_sounding(n);
        glow.energy[pc]=(uint8_t)(held?255:glow.energy[pc]>16u?glow.energy[pc]-16u:0u);
    }
    glow.last=now;
}
static void ag_world(uint32_t now,uint16_t accent)
{
    unsigned t=now/90u;
    for(int y=0;y<106;y++){
        uint16_t top=RGB(9,10,27),bottom=RGB(34,23,61);
        cv_rect(0,y,240,1,ag_mix(top,bottom,(unsigned)y*2u));
    }
    /* Stars have individual slow phases, with a drifting far layer. */
    for(unsigned i=0;i<53;i++){
        int x=(int)((i*83u+17u+(i%3u==0?now/800u:0u))%240u),y=(int)((i*37u+7u)%77u);
        int brightness=AG_SIN[(i*9u+t/3u)&63u]+128;
        uint16_t c=ag_mix(RGB(37,42,74),RGB(181,188,239),(unsigned)brightness/2u);
        cv_rect(x,y,1+(i%19u==0),1,c);
        if(i%19u==0 && brightness>185){cv_rect(x,y-1,1,3,c);cv_rect(x-1,y,3,1,c);}
    }
    /* Two flowing aurora ribbons behind the city. */
    for(int layer=0;layer<2;layer++)for(int x=0;x<240;x+=5){
        int y=47+layer*5+ag_sin((unsigned)(x/5)+t/3u+layer*14u,7);
        cv_line(x,y,x+5,48+layer*5+ag_sin((unsigned)(x/5+1)+t/3u+layer*14u,7),ag_mix(RGB(18,22,44),accent,(unsigned)(23+layer*15)));
    }
    for(int x=0;x<240;x+=2){
        int a=(x+29)%94,y=73-(a<47?a:94-a)/4;
        cv_rect(x,y,2,82-y,RGB(23,24,45));
    }
    for(int i=0;i<25;i++){
        int x=i*11-5,h=5+(i*17)%19;
        cv_rect(x,82-h,8, h,RGB(12,17,32));
        for(int w=0;w<2;w++)for(int row=0;row<3;row++)if(row*5+4<h){
            unsigned pc=(unsigned)(i+row*3+w)%12u;
            cv_rect(x+2+w*3,84-h+row*5,1,2,ag_mix(RGB(46,47,77),accent,25u+glow.energy[pc]/2u));
        }
    }
    cv_rect(0,82,240,1,ag_mix(RGB(90,64,115),accent,50u+glow.pulse/3u));
    for(int i=-5;i<=5;i++)cv_line(120+i*11,83,120+i*45,105,RGB(33,35,65));
    for(int i=0;i<5;i++){
        int p=(i*6+(int)(now/100u))%29,y=83+p*p/36;
        cv_rect(0,y,240,1,RGB(42,37,73));
    }
    /* A quiet comet every 11 seconds, plus a short note-triggered spark. */
    unsigned comet=(now%11000u)/12u;
    if(comet<100u){int x=20+(int)comet*2,y=7+(int)comet/4;
        cv_line(x-18,y-5,x,y,ag_mix(RGB(24,29,52),accent,80));cv_rect(x,y,2,1,K_TEXT);}
    if(glow.pulse>80u){int r=(255-glow.pulse)/12;int x=166+(visual_last_note%7)*5;
        cv_rect(x-r,12+r,2,1,accent);cv_rect(x+r,12-r,1,2,accent);}
}
static void ag_planet(uint32_t now,uint16_t accent)
{
    int x=187,y=33;unsigned spin=now/100u;
    ag_ring(x,y,31+glow.pulse/50,29,spin,ag_mix(RGB(17,18,39),accent,38u+glow.pulse/5u),0);
    ag_ring(x,y,43,10,spin,ag_mix(RGB(35,29,62),accent,85),2);
    ag_ring(x,y,39,8,spin,ag_mix(RGB(35,29,62),accent,60),2);
    for(int dy=-21;dy<=21;dy++){
        int w=21;while(w*w+dy*dy>441)w--;
        unsigned shade=(unsigned)(dy+21)*3u+55u;
        cv_rect(x-w,y+dy,w*2+1,1,ag_mix(RGB(64,49,111),accent,shade));
        if((dy+22)%7<2)cv_rect(x-w,y+dy,w*2+1,1,ag_mix(RGB(56,39,95),accent,shade/2u));
    }
    /* The unlit hemisphere and tiny cloudlets remain crisp at native pixels. */
    for(int dy=-20;dy<=20;dy++){
        int w=20;while(w*w+dy*dy>400)w--;
        int start=x-w,end=x-8+ag_sin((unsigned)(dy+32)*2u,3);
        if(end>start)cv_rect(start,y+dy,end-start,1,RGB(38,31,71));
    }
    cv_rect(x+4,y-8,6,2,ag_mix(accent,K_TEXT,70));cv_rect(x+10,y-2,3,1,accent);
    ag_ring(x,y,43,10,spin,ag_mix(accent,K_TEXT,35),1);
    ag_ring(x,y,39,8,spin,ag_mix(accent,K_TEXT,15),1);
    int dx=ag_sin(spin+16u,43),dy=ag_sin(spin,10);
    ag_disc(x+dx,y+dy-dx/5,2,K_TEXT);
}
static void ag_constellation(uint32_t now,uint16_t accent)
{
    int x[12],y[12],previous=-1,first=-1;
    ag_ring(186,36,31,31,0,RGB(56,39,77),0);
    for(int i=0;i<12;i++){
        unsigned a=(unsigned)(i*64/12)+now/1000u;
        x[i]=186+ag_sin(a+16u,28);y[i]=36+ag_sin(a,28);
        if(glow.energy[i]>100u){
            if(previous>=0)cv_line(x[previous],y[previous],x[i],y[i],ag_mix(RGB(40,32,66),accent,160));
            if(first<0)first=i;
            previous=i;
        }
    }
    if(first>=0 && first!=previous)cv_line(x[first],y[first],x[previous],y[previous],accent);
    for(int i=0;i<12;i++){
        unsigned e=glow.energy[i];
        if(e){ag_disc(x[i],y[i],4,ag_mix(RGB(34,28,57),accent,e/3u));ag_disc(x[i],y[i],2,ag_mix(RGB(54,43,78),accent,e));}
        else cv_rect(x[i],y[i],2,2,RGB(90,73,117));
    }
    ag_disc(186,36,3,ag_mix(RGB(69,42,94),K_TEXT,glow.pulse/2u));
}
static void ag_roll(uint16_t accent)
{
    const composition_t *p=&generator.plan[generator.active];unsigned bar=generator.step/16u;
    int low=127,high=0,previous_x=-1,previous_y=0;
    for(unsigned i=0;i<16;i++)if(p->rhythm[bar]>>i&1u){
        int pitch=p->melody[bar*16u+i];if(pitch<low)low=pitch;if(pitch>high)high=pitch;
    }
    cv_rect(8,60,224,43,RGB(12,16,33));
    for(int row=0;row<5;row++)cv_rect(9,64+row*8,222,1,RGB(29,32,54));
    if(p->mode!=0)for(int v=0;v<3;v++){
        int y=p->mode==1?69+v*12:88+v*5;
        cv_rect(10,y,218,3,ag_mix(RGB(36,30,61),accent,p->mode==1?155:55));
        cv_rect(10,y,3,4,accent);
        if(p->mode==1){char note[12];note_text(note,p->chords[bar][v]);cv_text(195,y-5,&FONT_XS,note,K_TEXT);}
    }
    for(int i=0;i<16;i++){
        int x=10+i*14,hit=(p->rhythm[bar]>>i)&1u;
        if(i%4==0)cv_rect(x-1,61,1,41,RGB(43,39,69));
        if(p->mode!=1 && hit){
            int y=65+(high-p->melody[bar*16u+(unsigned)i])*(p->mode==2?19:31)/(high-low+1);
            if(previous_x>=0)cv_line(previous_x+7,previous_y+1,x,y+1,ag_mix(RGB(27,26,48),accent,65));
            cv_rect(x-1,y-1,11,6,ag_mix(RGB(18,19,39),accent,35));
            cv_rect(x,y,9,4,accent);cv_rect(x,y,9,1,ag_mix(accent,K_TEXT,65));
            previous_x=x;previous_y=y;
        }
        if(generator.running && (unsigned)i==generator.step%16u){
            cv_rect(x-1,61,1,41,K_HOT);cv_rect(x-3,60,5,2,K_HOT);
            cv_rect(x,99,9,3,ag_mix(accent,K_TEXT,50));
        }
    }
    cv_rect(8,60,224,1,ag_mix(RGB(39,34,62),accent,75));
}
static void ag_rhythm(uint16_t accent)
{
    const composition_t *p=&generator.plan[generator.active];unsigned bar=generator.step/16u;
    ag_ring(188,30,28,28,0,RGB(62,44,69),0);
    for(unsigned i=0;i<16;i++){
        unsigned a=i*4u;
        int x=188+ag_sin(a+16u,27),y=30+ag_sin(a,27);
        int hit=(p->rhythm[bar]>>i)&1u,playing=generator.running && generator.step%16u==i;
        if(playing)ag_disc(x,y,4,ag_mix(RGB(40,27,48),accent,110));
        cv_rect(x-1,y-1,3,3,playing?K_TEXT:hit?accent:RGB(66,47,66));
    }
    ag_disc(188,30,7+glow.pulse/55,ag_mix(RGB(57,32,59),accent,70u+glow.pulse/3u));
    cv_line(182,30,194,30,K_TEXT);cv_line(188,24,188,36,K_TEXT);
}
static void ag_clock(uint16_t accent)
{
    unsigned phase=arp_engine.ticks%96u;
    ag_ring(188,36,31,31,0,RGB(55,44,80),0);
    for(unsigned i=0;i<24;i++){
        unsigned angle=i*64u/24u;
        int x=188+ag_sin(angle+16,29),y=36+ag_sin(angle,29);
        uint16_t color=i<=phase%24u?accent:RGB(66,58,92);
        cv_rect(x,y,i%6u?2:3,i%6u?2:3,color);
    }
    unsigned hand=phase*64u/24u;
    cv_line(188,36,188+ag_sin(hand+16u,22),36+ag_sin(hand,22),K_TEXT);
    ag_disc(188,36,3,accent);
    for(unsigned i=0;i<4;i++)cv_rect(164+(int)i*13,78,9,3,i==phase/24u?accent:RGB(47,38,68));
}
static void ag_arp(uint16_t accent)
{
    for(unsigned i=0;i<9;i++){
        unsigned p=(arp_engine.position+i)%9u;
        int x=143+(int)i*9,y=68-(int)i*6;
        uint16_t c=proj.arp_on && p==0?K_TEXT:ag_mix(RGB(28,33,55),accent,(unsigned)(45+i*17));
        cv_rect(x,y,7,4,c);
        if(i)cv_line(x-2,y+6,x+2,y+6,RGB(67,66,105));
    }
    for(int i=0;i<16;i++){
        int x=9+i*14,h=4+(i*7)%14;
        if(arp_engine.count){int n=arp_engine.order[i%arp_engine.count];h=4+(n%12)*2;}
        int active=proj.arp_on && arp_engine.count && (unsigned)i==arp_engine.position%16u;
        cv_rect(x,101-h,10,h,ag_mix(RGB(31,34,58),accent,active?230:60));
        cv_rect(x,101-h,10,1,active?K_TEXT:accent);
    }
}
static void ag_scene(uint32_t now,int page,uint16_t accent)
{
    char text[64],*p;
    ag_world(now,accent);
    if(page==PAGE_DAW){
        ag_planet(now,accent);
        cv_text(8,8,&FONT_L,"NOCT",K_TEXT);cv_text(10,43,&FONT_XS,"A F T E R G L O W",accent);
        sounding_text(text);cv_text(10,59,&FONT_XS,text,K_TEXT);
        noct_logo(12,75,1,0);cv_text(34,78,&FONT_XS,"FL STUDIO",K_HOT);
        noct_logo(131,76,1,1);cv_text(159,78,&FONT_XS,"LIVE",K_MINT);
    }else if(page==PAGE_CHORD){
        ag_constellation(now,accent);
        cv_text(9,4,&FONT_XS,"CHORD CONSTELLATION",accent);
        cv_rect(191,3,39,14,RGB(13,19,33));
        noct_center(210,3,&FONT_XS,CHORD_ON?"ON":"OFF",CHORD_ON?K_MINT:K_DIM);
        cv_text(8,22,&FONT_M,CHORD_ON?CHORD_NAME[CHORD_TYPE]:"CHORDS OFF",K_TEXT);
        sounding_text(text);cv_text(9,54,&FONT_XS,text,K_TEXT);
        for(int i=0;i<24;i++){
            unsigned e=glow.energy[(i+5)%12];int black=((0x054Au>>((i+5)%12))&1u)!=0;
            cv_rect(9+i*9,84,7,black?12:19,ag_mix(black?RGB(38,30,56):RGB(75,64,96),accent,e));
        }
        cv_text(9,71,&FONT_XS,CHORD_ON?"SEL AGAIN: OFF / KNOB: TYPE":"SEL: ENABLE CHORDS",K_DIM);
    }else if(page==PAGE_GEN || page==PAGE_FEEL){
        if(page==PAGE_GEN)ag_planet(now,accent);else ag_rhythm(accent);
        cv_text(9,3,&FONT_XS,page==PAGE_GEN?"COMPOSE  /  1 OF 2":"RHYTHM  /  2 OF 2",accent);
        if(page==PAGE_GEN){
            cv_text(8,19,&FONT_B,proj.gen_mode==2?"MELODY + CHORDS":GEN_MODE_NAME[proj.gen_mode],K_TEXT);
            p=note_text(text,music_midi_pitch(GEN_ROOT+12*proj.octave));p=put_s(p," / ");put_s(p,SCALE_NAME[GEN_SCALE]);
            cv_text(9,39,&FONT_XS,text,K_TEXT);
        }else{
            put_u(text,GEN_DENSITY);cv_text(8,17,&FONT_L,text,K_TEXT);
            cv_text(50,33,&FONT_XS,"NOTES / BAR",accent);
        }
        ag_roll(accent);
        for(unsigned i=0;i<4;i++)cv_rect(170+(int)i*15,52,10,3,i==generator.step/16u?K_HOT:RGB(65,47,96));
    }else if(page==PAGE_ARP){
        ag_arp(accent);
        cv_text(9,4,&FONT_XS,"ARPEGGIATOR",accent);cv_text(8,22,&FONT_M,ARP_NAME[proj.arp_mode],K_TEXT);
        p=put_s(text,RATE_NAME[proj.arp_rate]);p=put_s(p," / ");put_s(p,proj.arp_on?"ON":"OFF");
        cv_text(9,55,&FONT_XS,text,K_TEXT);cv_text(9,72,&FONT_XS,proj.arp_latch?"LATCH ON":"LATCH OFF",K_DIM);
    }else{
        ag_clock(accent);cv_text(9,4,&FONT_XS,"CLOCK / ORBIT",accent);
        p=put_u(text,proj.bpm10/10u);put_u(put_s(p,"."),proj.bpm10%10u);
        cv_text(8,22,&FONT_L,text,K_TEXT);cv_text(9,60,&FONT_XS,CLOCK_NAME[proj.clock],accent);
        int waiting=clock_external()&&now-arp_engine.last_clock>500u;
        cv_rect(7,84,226,20,RGB(13,19,33));
        cv_text(12,88,&FONT_XS,waiting?"WAITING FOR MIDI CLOCK":clock_external()?"MIDI CLOCK CONNECTED":arp_engine.daw_tempo_valid?"DAW BPM / INTERNAL GRID":"INTERNAL CLOCK",waiting?K_HOT:accent);
    }
    /* A narrow sweep signals page changes, without moving labels or values. */
    if(now-glow.page_at<300u){int x=(int)((now-glow.page_at)*240u/300u);
        cv_rect(x,0,1,106,ag_mix(RGB(34,24,55),accent,75));cv_rect(x+1,0,1,106,RGB(35,28,61));}
}
static void noct_draw(void)
{
    uint32_t now=plat_ms();int page=ui.page;char label[24],value[32],text[64],*p;
    if(now-ui.sig[0]<50u && ui.frame)return;
    ui.sig[0]=now;ag_tick(now,page);uint16_t accent=AG_ACCENT[page];
    cv_begin(240,144,RGB(9,11,23));
    cv_text(8,1,&FONT_B,"NOCT",K_TEXT);cv_text(53,3,&FONT_XS,"FM",accent);
    cv_rect(75,7,3,3,lit(PORT_USB,0)||lit(PORT_USB,1)?K_MINT:RGB(45,47,72));
    p=put_u(text,proj.bpm10/10u);if(proj.bpm10%10u)put_u(put_s(p,"."),proj.bpm10%10u);
    cv_text(90,3,&FONT_XS,text,K_DIM);
    const char *status=run_label();cv_rect(181,1,51,16,ag_mix(RGB(17,21,35),accent,35));
    noct_center(206,2,&FONT_XS,status,accent);if(noct_recording)cv_rect(172,7,4,4,K_HOT);
    static const char *const tabs[5]={"DAW","CHRD","GEN","ARP","SYNC"};
    int selected=page==PAGE_DAW?0:page==PAGE_CHORD?1:page==PAGE_ARP?3:page==PAGE_SYNC?4:2;
    for(int i=0;i<5;i++){
        int x=8+i*45;cv_rect(x,21,43,15,i==selected?ag_mix(RGB(16,20,36),accent,28):RGB(11,13,26));
        noct_center(x+21,21,&FONT_XS,tabs[i],i==selected?accent:RGB(116,122,153));
        if(i==1)cv_rect(x+36,26,3,3,CHORD_ON?K_MINT:RGB(45,47,72));
        if(i==selected){cv_rect(x,36,43,1,accent);cv_rect(x+10,37,23,1,ag_mix(RGB(25,21,45),accent,85));}
    }
    cv_band(38,106);ag_scene(now,page,accent);cv_blit(0,0);plat_yield();
    cv_begin(240,96,RGB(9,11,23));
    for(int i=0;i<4;i++){
        int x=8+(i%2)*114,y=(i/2)*38,level,hot=ui.touched==i&&now<ui.touch_until;
        knob_text(page,i,label,value,&level);
        uint16_t edge=hot?K_HOT:ag_mix(RGB(38,36,60),accent,28);
        cv_rect(x,y+1,110,35,RGB(19,21,39));cv_rect(x,y+1,110,1,edge);
        cv_rect(x,y+1,1,35,edge);cv_rect(x+109,y+1,1,35,RGB(31,30,52));
        cv_rect(x+1,y+2,12,12,ag_mix(RGB(23,24,41),accent,30));
        put_u(text,(unsigned)i+1u);cv_text(x+4,y+2,&FONT_XS,text,hot?K_HOT:accent);
        cv_text(x+18,y+2,&FONT_XS,label,K_DIM);cv_text(x+7,y+16,&FONT_B,value,hot?K_HOT:K_TEXT);
        if(level>=0)for(unsigned j=0;j<12;j++){
            unsigned a=22u+j*5u;int dx=ag_sin(a+16,10),dy=ag_sin(a,10);
            cv_rect(x+94+dx,y+21+dy,2,2,j*127u/11u<=(unsigned)level?accent:RGB(47,43,67));
        }
        if(hot){cv_rect(x+97,y+33,12,2,K_HOT);cv_rect(x+107,y+28,2,7,K_HOT);}
    }
    cv_rect(8,79,224,1,RGB(43,35,65));
    if(now<ui.msg_until)put_s(text,ui.message);
    else if(generator.pending)put_s(text,"NEXT BAR / HOME: DAW");
    else if(page==PAGE_GEN)put_s(text,"PRESET: RHYTHM  EDIT: NEW");
    else if(page==PAGE_FEEL)put_s(text,"SEQ: RUN/STOP  EDIT: NEW");
    else if(page==PAGE_DAW)put_s(text,KNOB_MAP?"MIDI MAP / HOLD HOME: MIXER":"MIXER / HOLD HOME: MIDI MAP");
    else put_s(text,"PRESET: PAGES  HOME: DAW");
    cv_text(8,82,&FONT_XS,text,now<ui.msg_until?K_HOT:K_DIM);cv_blit(0,144);
}
