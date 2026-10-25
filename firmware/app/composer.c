/* SPDX-License-Identifier: MPL-2.0
 * Four-bar composer: prepared in the UI thread; playback only reads a plan.
 * Melody and simultaneous diatonic triads have independent note ownership.
 */
static const char *const GEN_MODE_NAME[3]={"MELODY","CHORDS","BOTH"};
typedef struct {
    uint8_t melody[64],chords[4][3],mode,gate;
    uint16_t rhythm[4];
} composition_t;
static struct {
    composition_t plan[2];
    volatile uint8_t active,pending;
    uint8_t enabled,running,step,preview,wait_bar,resume_chord;
    uint8_t notes[3],count,melody_note;
    uint16_t cursor,chord_gate,melody_gate;
} generator;
static uint32_t music_random(uint32_t *state)
{*state^=*state<<13;*state^=*state>>17;*state^=*state<<5;return *state;}
static int scale_pitch(int degree,const uint8_t *scale)
{
    int octave=0;while(degree<0){degree+=7;octave--;}
    return 12*(octave+degree/7)+scale[degree%7];
}
static void generator_melody_off(void)
{if(generator.melody_note!=255u)bus_off(generator.melody_note);generator.melody_note=255;generator.melody_gate=0;}
static void generator_chord_off(void)
{for(int i=0;i<generator.count;i++)bus_off(generator.notes[i]);generator.count=0;generator.chord_gate=0;}
static void generator_off(void){generator_melody_off();generator_chord_off();}
static void generator_apply(void)
{if(generator.pending){generator.active=(uint8_t)(generator.pending-1u);generator.pending=0;}}
static void generator_regenerate(void)
{
    static const uint8_t scales[2][7]={{0,2,4,5,7,9,11},{0,2,3,5,7,8,10}};
    static const uint8_t progressions[2][4][4]={
        {{0,5,3,4},{0,3,5,4},{0,4,5,3},{1,4,0,0}},
        {{0,5,2,6},{0,3,5,4},{0,6,5,6},{0,2,6,3}}};
    uint32_t f=plat_lock();generator.pending=0;
    int dest=1-generator.active;plat_unlock(f);
    composition_t *p=&generator.plan[dest];
    const uint8_t *scale=scales[GEN_SCALE!=0];
    const uint8_t *progression=progressions[GEN_SCALE!=0][proj.gen_progression];
    uint32_t random=proj.gen_seed;int motif[16],previous[3],base,range=GEN_OCTAVES*7;
    base=(int)GEN_ROOT+12*proj.octave;
    while(base<12)base+=12;while(base>72)base-=12;
    p->mode=proj.gen_mode;p->gate=GEN_GATE;
    motif[0]=0;
    for(int i=1;i<16;i++)motif[i]=clamp(motif[i-1]+(int)(music_random(&random)%3u)-1,-3,3);
    previous[0]=base-12;previous[1]=base-8;previous[2]=base-5;
    for(int bar=0;bar<4;bar++){
        int degree=progression[bar],best=10000,chosen[3]={0,0,0};
        /* Anchors plus syncopated motif, with varied gaps rather than arp pulses. */
        uint8_t positions[14]={2,3,6,7,10,11,12,14,1,4,5,9,13,15};
        for(int i=7;i>0;i--){int j=(int)(music_random(&random)%(unsigned)(i+1));uint8_t t=positions[i];positions[i]=positions[j];positions[j]=t;}
        for(int i=13;i>8;i--){int j=8+(int)(music_random(&random)%(unsigned)(i-7));uint8_t t=positions[i];positions[i]=positions[j];positions[j]=t;}
        p->rhythm[bar]=GEN_DENSITY?1u:0u;
        if(GEN_DENSITY>1)p->rhythm[bar]|=1u<<8;
        for(int i=2;i<GEN_DENSITY;i++)p->rhythm[bar]|=(uint16_t)(1u<<positions[i-2]);
        /* Choose the nearest inversion, preserving ascending voice order. */
        for(int inv=0;inv<3;inv++)for(int oct=-2;oct<=0;oct++){
            int candidate[3],cost=0;
            for(int v=0;v<3;v++){
                int member=v+inv;
                candidate[v]=base+scale_pitch(degree+2*(member%3),scale)+12*(member/3+oct);
                if(GEN_OCTAVES>1 && v==2)candidate[v]+=12;
                if(GEN_OCTAVES==3 && v==0)candidate[v]-=12;
                if(candidate[v]<0 || candidate[v]>115){cost=10000;break;}
                int distance=candidate[v]-previous[v];cost+=distance<0?-distance:distance;
            }
            if(cost<best){best=cost;memcpy(chosen,candidate,sizeof chosen);}
        }
        for(int v=0;v<3;v++){p->chords[bar][v]=(uint8_t)chosen[v];previous[v]=chosen[v];}
        for(int step=0;step<16;step++){
            int d=degree+motif[step];
            if(!(step%4)){
                int nearest=d,cost=100;
                for(int voice=0;voice<3;voice++)for(int oct=-1;oct<=1;oct++){
                    int candidate=degree+2*voice+7*oct,distance=candidate-d;
                    if(distance<0)distance=-distance;
                    if(distance<cost){nearest=candidate;cost=distance;}
                }
                d=nearest;
            }
            if(GEN_OCTAVES==3 && bar==2)d+=7;
            if(bar==3 && step>=12)d=step==12?1:0;
            while(d<0)d+=7;while(d>=range)d-=7;
            int pitch=base+scale_pitch(d,scale)+(p->mode==2?12:0);
            /* Pentatonic melody uses minor harmony but omits degrees 2 and 6. */
            if(GEN_SCALE==2 && (d%7==1 || d%7==5))pitch=base+scale_pitch(d-1,scale)+(p->mode==2?12:0);
            p->melody[bar*16+step]=(uint8_t)clamp(pitch,0,127);
        }
    }
    f=plat_lock();generator.pending=(uint8_t)(dest+1);
    if(!generator.running)generator_apply();plat_unlock(f);
}
static void generator_step(void)
{
    unsigned step=generator.cursor/6u,local=step%16u;
    if(!local)generator_apply();
    const composition_t *p=&generator.plan[generator.active];
    generator.step=(uint8_t)step;
    if(!local && p->mode!=0){
        generator_chord_off();generator.count=3;
        for(int i=0;i<3;i++){generator.notes[i]=p->chords[step/16u][i];bus_on(generator.notes[i],proj.vel);}
        generator.chord_gate=(uint16_t)(96u*p->gate/100u);
    }
    if(p->mode!=1 && (p->rhythm[step/16u]>>local&1u)){
        unsigned distance=1;
        while(distance<64u){unsigned next=(step+distance)%64u;
            if(p->rhythm[next/16u]>>(next%16u)&1u)break;distance++;}
        generator_melody_off();generator.melody_note=p->melody[step];
        bus_on(generator.melody_note,proj.vel);
        generator.melody_gate=(uint16_t)(distance*6u*p->gate/100u);
        if(!generator.melody_gate)generator.melody_gate=1;
    }
}
static void generator_clock(void)
{
    if(!generator.enabled || !generator.running)return;
    if(generator.wait_bar){
        if(arp_engine.ticks%96u)return;
        generator.wait_bar=0;generator.cursor=(uint16_t)(arp_engine.ticks%384u);
    }
    if(generator.resume_chord){
        const composition_t *p=&generator.plan[generator.active];
        unsigned elapsed=generator.cursor%96u,duration=96u*p->gate/100u;
        generator.resume_chord=0;
        if(elapsed && elapsed<duration && p->mode!=0){
            generator.count=3;
            for(int i=0;i<3;i++){generator.notes[i]=p->chords[generator.cursor/96u][i];bus_on(generator.notes[i],proj.vel);}
            generator.chord_gate=(uint16_t)(duration-elapsed+1u);
        }
    }
    if(generator.chord_gate && !--generator.chord_gate)generator_chord_off();
    if(generator.melody_gate && !--generator.melody_gate)generator_melody_off();
    if(generator.cursor%6u==0)generator_step();
    generator.cursor=(uint16_t)((generator.cursor+1u)%384u);
}
static void generator_toggle(void)
{
    uint32_t f=plat_lock();
    generator_off();generator.resume_chord=0;generator.enabled=!generator.enabled;
    generator.running=generator.enabled;
    generator.preview=(uint8_t)(generator.enabled && !arp_engine.remote_running);
    generator.wait_bar=(uint8_t)(generator.enabled && arp_engine.remote_running && arp_engine.ticks%96u!=0);
    generator.cursor=arp_engine.remote_running?(uint16_t)(arp_engine.ticks%384u):0;
    if(!generator.enabled)generator_apply();
    plat_unlock(f);
}
