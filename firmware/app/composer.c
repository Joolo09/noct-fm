/* SPDX-License-Identifier: MPL-2.0
 * Four-bar composer: prepared in the UI thread; playback only reads a plan.
 * Melody, bass and chords have independent note ownership.
 *
 * NOCT 0.8 phrase model (all integer, seeded, reproducible after SAVE):
 *   bar 1  motif      rhythm R, contour M, strong beats on chord tones
 *   bar 2  answer     R varied by one hit, M sequenced onto the next chord
 *   bar 3  contrast   rhythm R2, new contour, a step higher (the climax)
 *   bar 4  cadence    M, then a stepwise approach that lands on the tonic
 * Leaps are followed by a step back; skipped pentatonic degrees never sound.
 */
enum { GM_MELODY, GM_CHORDS, GM_BOTH, GM_BASS, GM_FULL };
enum { ST_HOLD, ST_PULSE, ST_OFFBEAT, ST_BROKEN };
static const char *const GEN_MODE_NAME[NGMODES]={"MELODY","CHORDS","BOTH","BASS","FULL"};
static const char *const STYLE_NAME[NSTYLES]={"HOLD","PULSE","OFFBEAT","BROKEN"};
static const char *const SWING_NAME[NSWING]={"STRAIGHT","LIGHT 58%","TRIPLET 67%","HARD 75%"};
static const uint8_t SWING_TICKS[NSWING]={0,1,2,3};   /* delay of odd sixteenths, in 24 PPQN ticks */
#define GEN_HAS_MELODY(m) ((m)==GM_MELODY || (m)==GM_BOTH || (m)==GM_FULL)
#define GEN_HAS_CHORDS(m) ((m)==GM_CHORDS || (m)==GM_BOTH || (m)==GM_FULL)
#define GEN_HAS_BASS(m) ((m)==GM_BASS || (m)==GM_FULL)
typedef struct {
    uint8_t melody[64],bass[64];
    int8_t accent[64];                 /* velocity offsets, 0 when HUMAN is 0 */
    uint8_t chords[4][4],degree[4];
    uint16_t rhythm[4],bass_rhythm[4];
    uint8_t mode,gate,voices,style,swing,minor;
} composition_t;
static struct {
    composition_t plan[2];
    volatile uint8_t active,pending;
    uint8_t enabled,running,step,preview,wait_bar,resume_chord;
    uint8_t notes[4],count,melody_note,bass_note;
    uint16_t cursor,chord_gate,melody_gate,bass_gate;
    uint32_t history[8];               /* previous seeds for undo (UI thread only) */
    uint8_t history_n;
} generator;
static const uint8_t PROGRESSIONS[2][NPROGS][4]={
    {{0,5,3,4},{0,3,5,4},{0,4,5,3},{1,4,0,0},{5,3,0,4},{0,2,3,4},{3,4,2,5},{0,3,0,4}},
    {{0,5,2,6},{0,3,5,4},{0,6,5,6},{0,2,6,3},{0,3,4,0},{5,6,0,0},{0,5,3,4},{0,6,3,5}}};
static uint32_t music_random(uint32_t *state)
{*state^=*state<<13;*state^=*state>>17;*state^=*state<<5;return *state;}
static unsigned music_pick(uint32_t *state,unsigned n){return n?music_random(state)%n:0u;}
static int scale_pitch(int degree,const uint8_t *scale)
{
    int octave=0;while(degree<0){degree+=7;octave--;}
    return 12*(octave+degree/7)+scale[degree%7];
}
/* Preserve the requested octave unless a pitch actually leaves MIDI range. */
static int music_midi_pitch(int pitch)
{while(pitch<0)pitch+=12;while(pitch>127)pitch-=12;return pitch;}
static int music_abs(int v){return v<0?-v:v;}
static void generator_melody_off(void)
{if(generator.melody_note!=255u)bus_off(generator.melody_note);generator.melody_note=255;generator.melody_gate=0;}
static void generator_bass_off(void)
{if(generator.bass_note!=255u)bus_off(generator.bass_note);generator.bass_note=255;generator.bass_gate=0;}
static void generator_chord_off(void)
{for(int i=0;i<generator.count;i++)bus_off(generator.notes[i]);generator.count=0;generator.chord_gate=0;}
static void generator_off(void){generator_melody_off();generator_bass_off();generator_chord_off();}
static void generator_apply(void)
{if(generator.pending){generator.active=(uint8_t)(generator.pending-1u);generator.pending=0;}}

/* Triad quality of a scale degree: 0 major, 1 minor, 2 diminished, 3 augmented. */
static int degree_quality(unsigned scale,int degree)
{
    const uint8_t *st=SCALE_STEPS[scale%NGSCALES];
    int third=scale_pitch(degree+2,st)-scale_pitch(degree,st),fifth=scale_pitch(degree+4,st)-scale_pitch(degree,st);
    if(fifth==6)return 2;
    if(fifth==8)return 3;
    return third==3;
}
/* Roman numerals of a progression in a scale, e.g. "i-VI-III-VII". */
static char *harmony_text(char *out,unsigned scale,unsigned prog)
{
    static const char *const upper[7]={"I","II","III","IV","V","VI","VII"};
    static const char *const lower[7]={"i","ii","iii","iv","v","vi","vii"};
    const uint8_t *degrees=PROGRESSIONS[SCALE_MINOR[scale%NGSCALES]][prog%NPROGS];
    for(int i=0;i<4;i++){
        int q=degree_quality(scale,degrees[i]);const char *r=q==1||q==2?lower[degrees[i]]:upper[degrees[i]];
        if(i)*out++='-';
        while(*r)*out++=*r++;
        if(q==2)*out++='o';
        if(q==3)*out++='+';
    }
    *out=0;
    return out;
}
static const char *harmony_now(void)
{static char text[32];harmony_text(text,GEN_SCALE,proj.gen_progression);return text;}
/* ---- composition (UI thread) */
/* Exactly `density` hits in a bar: the downbeat, beat 3, then weighted picks. `contrast` favours offbeats. */
static uint16_t compose_rhythm(uint32_t *rnd,int density,int contrast)
{
    static const uint8_t weight[2][16]={
        {0,3,7,5,9,3,7,5,0,3,7,5,9,3,7,5},
        {0,4,9,7,6,4,9,6,0,4,9,7,6,4,9,7}};
    uint16_t bits=0;
    if(density<=0)return 0;
    if(density>16)density=16;
    bits|=1u;
    if(density>1)bits|=1u<<8;
    for(int n=density>1?2:1;n<density;n++){
        unsigned total=0,r;
        for(int i=0;i<16;i++)if(!(bits>>i&1u))total+=weight[contrast][i]?weight[contrast][i]:1u;
        r=music_pick(rnd,total);
        for(int i=0;i<16;i++)if(!(bits>>i&1u)){
            unsigned w=weight[contrast][i]?weight[contrast][i]:1u;
            if(r<w){bits|=(uint16_t)(1u<<i);break;}
            r-=w;
        }
    }
    return bits;
}
/* The answer: one non-anchor hit moves to a free neighbour; the count stays exact. */
static uint16_t compose_vary(uint32_t *rnd,uint16_t bits)
{
    int movable[16],n=0;
    for(int i=1;i<16;i++)if(i!=8 && (bits>>i&1u))movable[n++]=i;
    if(!n)return bits;
    int from=movable[music_pick(rnd,(unsigned)n)];
    int dir=music_pick(rnd,2)?1:-1;
    for(int k=0;k<2;k++,dir=-dir){
        int to=from+dir;
        if(to>0 && to<16 && to!=8 && !(bits>>to&1u))return (uint16_t)((bits&~(1u<<from))|(1u<<to));
    }
    return bits;
}
/* Contour: degree moves per hit; mostly steps, some leaps, a leap is answered by a step back. */
static void compose_contour(uint32_t *rnd,int8_t *moves,int rise)
{
    static const int8_t choice[12]={-1,1,-1,1,-2,2,0,1,-1,3,-3,2};
    int last=0;
    for(int i=0;i<16;i++){
        int m=choice[music_pick(rnd,12)];
        if(last>=3 || last<=-3)m=last>0?-1:1;
        if(rise && i<4 && m<0)m=-m;
        moves[i]=(int8_t)m;last=m;
    }
}
static int chord_tone_near(int d,int root,int voices)
{
    int best=d,cost=100;
    for(int v=0;v<voices;v++)for(int oct=-2;oct<=2;oct++){
        int candidate=root+2*v+7*oct,distance=music_abs(candidate-d);
        if(distance<cost || (distance==cost && candidate<best)){best=candidate;cost=distance;}
    }
    return best;
}
static int fold_degree(int d,int range)
{
    while(d<0 || d>range){
        if(d<0)d=-d;
        if(d>range)d=2*range-d;
    }
    return d;
}
static void generator_regenerate(void)
{
    uint32_t f=plat_lock();generator.pending=0;
    int dest=1-generator.active;plat_unlock(f);
    composition_t *p=&generator.plan[dest];
    unsigned scale_index=GEN_SCALE<NGSCALES?GEN_SCALE:0u;
    const uint8_t *scale=SCALE_STEPS[scale_index];
    uint8_t skip=SCALE_SKIPS(scale_index);
    const uint8_t *progression=PROGRESSIONS[SCALE_MINOR[scale_index]][proj.gen_progression%NPROGS];
    uint32_t random=proj.gen_seed?proj.gen_seed:978437u;
    int base=60,range=GEN_OCTAVES*7,voices=proj.gen_color?4:3,previous[4];
    int transpose=(int)GEN_ROOT-60+12*proj.octave,density=GEN_DENSITY;
    int lift=(proj.gen_mode==GM_BOTH || proj.gen_mode==GM_FULL)?12:0;
    int8_t motif[16],contrast[16];
    uint16_t motif_rhythm,contrast_rhythm;
    memset(p,0,sizeof *p);
    p->mode=proj.gen_mode;p->gate=GEN_GATE;p->voices=(uint8_t)voices;
    p->style=proj.gen_style;p->swing=proj.gen_swing;p->minor=SCALE_MINOR[scale_index];
    motif_rhythm=compose_rhythm(&random,density,0);
    contrast_rhythm=compose_rhythm(&random,density,1);
    compose_contour(&random,motif,0);
    compose_contour(&random,contrast,1);
    p->rhythm[0]=motif_rhythm;
    p->rhythm[1]=compose_vary(&random,motif_rhythm);
    p->rhythm[2]=contrast_rhythm;
    p->rhythm[3]=motif_rhythm;
    previous[0]=base-12;previous[1]=base-8;previous[2]=base-5;previous[3]=base-1;
    int degree_now=chord_tone_near(range/2,progression[0],3);
    int approach=SCALE_MINOR[scale_index]?1:(music_pick(&random,2)?1:-1);
    for(int bar=0;bar<4;bar++){
        int degree=progression[bar],best=10000,chosen[4]={0,0,0,0};
        p->degree[bar]=(uint8_t)degree;
        /* Chords: the inversion nearest the previous one, ascending voices. */
        for(int inv=0;inv<voices;inv++)for(int oct=-2;oct<=0;oct++){
            int candidate[4],cost=0;
            for(int v=0;v<voices;v++){
                int member=v+inv;
                candidate[v]=base+scale_pitch(degree+2*(member%voices),scale)+12*(member/voices+oct);
                if(GEN_OCTAVES>1 && v==voices-1)candidate[v]+=12;
                if(GEN_OCTAVES==3 && v==0)candidate[v]-=12;
                if(candidate[v]<0 || candidate[v]>115){cost=10000;break;}
                cost+=music_abs(candidate[v]-previous[v]);
            }
            if(cost<best){best=cost;memcpy(chosen,candidate,sizeof chosen);}
        }
        for(int v=0;v<voices;v++){p->chords[bar][v]=(uint8_t)music_midi_pitch(chosen[v]+transpose);previous[v]=chosen[v];}
        /* Melody: motif in bars 1, 2 and 4, contrast in bar 3. */
        const int8_t *moves=bar==2?contrast:motif;
        int hits[16],count=0;
        for(int s=0;s<16;s++)if(p->rhythm[bar]>>s&1u)hits[count++]=s;
        if(bar==1 || bar==3)degree_now=chord_tone_near(degree_now,degree,3);
        if(bar==2)degree_now=fold_degree(chord_tone_near(degree_now+2,degree,3),range);
        for(int h=0;h<count;h++){
            int s=hits[h],d=h?degree_now+moves[h]:degree_now;
            if(!(s%4))d=chord_tone_near(d,degree,voices);
            if(bar==3 && (h==count-1 || h==count-2)){
                /* The cadence: the tonic nearest the line, approached by step. */
                int tonic=(range>=7 && music_abs(degree_now-7)<music_abs(degree_now))?7:0;
                d=h==count-1?tonic:tonic+approach;
            }
            d=fold_degree(d,range);
            while(d>0 && (skip>>(d%7)&1u))d--;
            degree_now=d;
            p->melody[bar*16+s]=(uint8_t)music_midi_pitch(base+scale_pitch(d,scale)+lift+transpose);
        }
        /* Bass: the chord root low; FULL plays root / fifth / approach, BASS follows the melody's rhythm. */
        int root=base+scale_pitch(degree,scale)-24;
        while(root>base-14)root-=12;
        int fifth=root+scale_pitch(degree+4,scale)-scale_pitch(degree,scale);
        int next_root=base+scale_pitch(progression[(bar+1)&3],scale)-24;
        while(next_root>base-14)next_root-=12;
        if(p->mode==GM_BASS){
            p->bass_rhythm[bar]=p->rhythm[bar]?p->rhythm[bar]:1u;
        }else{
            p->bass_rhythm[bar]=(uint16_t)(1u|1u<<8|((density>=6 && music_pick(&random,2))?1u<<14:0u));
        }
        for(int s=0;s<16;s++)if(p->bass_rhythm[bar]>>s&1u){
            int pitch=root,last=!(p->bass_rhythm[bar]>>s>>1);
            if(!s)pitch=root;
            else if(last && s>=12 && next_root!=root)pitch=next_root+(next_root>root?-1:1);   /* chromatic approach */
            else if(s==8)pitch=music_pick(&random,3)?fifth:root+12;
            else pitch=music_pick(&random,2)?root+12:fifth;
            p->bass[bar*16+s]=(uint8_t)music_midi_pitch(pitch+transpose);
        }
    }
    /* Velocity shape: downbeats lead, offbeat sixteenths are soft, bar 3 is the peak; HUMAN adds spread. */
    if(proj.human){
        static const int8_t step_accent[16]={10,-8,0,-6,4,-8,0,-6,6,-8,0,-6,4,-8,0,-6};
        for(int i=0;i<64;i++){
            int spread=(int)proj.human,a=step_accent[i&15]*spread/40+((i>>4)==2?3:0);
            a+=(int)music_pick(&random,(unsigned)spread+1u)-spread/2;
            p->accent[i]=(int8_t)clamp(a,-40,30);
        }
    }
    f=plat_lock();generator.pending=(uint8_t)(dest+1);
    if(!generator.running)generator_apply();
    plat_unlock(f);
}
/* New melody and rhythm; the previous seed is kept for undo (SEQ + EDIT). */
static void generator_new_seed(int new_harmony)
{
    if(generator.history_n<8u)generator.history_n++;
    for(int i=7;i>0;i--)generator.history[i]=generator.history[i-1];
    generator.history[0]=proj.gen_seed;
    music_random(&proj.gen_seed);
    if(!proj.gen_seed)proj.gen_seed=978437u;
    if(new_harmony){uint32_t r=proj.gen_seed;proj.gen_progression=(uint8_t)((proj.gen_progression+1u+music_pick(&r,NPROGS-1u))%NPROGS);}
    generator_regenerate();
}
static int generator_undo(void)
{
    if(!generator.history_n)return 0;
    proj.gen_seed=generator.history[0];
    for(int i=0;i<7;i++)generator.history[i]=generator.history[i+1];
    generator.history_n--;
    generator_regenerate();
    return 1;
}

/* ---- playback (timer) */
static int chord_trigger(unsigned style,unsigned local)
{
    if(style==ST_PULSE)return !(local%4u);
    if(style==ST_OFFBEAT)return local%4u==2u;
    if(style==ST_BROKEN)return !(local%2u);
    return !local;
}
static unsigned chord_length(unsigned style){return style==ST_PULSE?24u:style==ST_HOLD?96u:12u;}
static uint32_t gen_velocity(const composition_t *p,unsigned step,int extra)
{return (uint32_t)clamp((int)proj.vel+p->accent[step&63u]+extra,1,127);}
static void generator_chord_on(const composition_t *p,unsigned bar,unsigned local,unsigned remaining)
{
    generator_chord_off();
    if(p->style==ST_BROKEN){
        unsigned voice=(local/2u)%p->voices;
        generator.notes[0]=p->chords[bar][voice];generator.count=1;
    }else{
        for(int i=0;i<p->voices;i++)generator.notes[i]=p->chords[bar][i];
        generator.count=p->voices;
    }
    for(int i=0;i<generator.count;i++)bus_on(generator.notes[i],gen_velocity(p,bar*16u+local,p->accent[bar*16u+local]?-6:0));
    generator.chord_gate=(uint16_t)remaining;
    if(!generator.chord_gate)generator.chord_gate=1;
}
static unsigned step_tick(const composition_t *p,unsigned step)
{return (step%64u)*6u+((step&1u)?SWING_TICKS[p->swing]:0u);}
/* Ticks from this hit to the next one in `rhythm`, around the four-bar loop. */
static unsigned hit_span(const composition_t *p,const uint16_t *rhythm,unsigned step)
{
    unsigned distance=1,span;
    while(distance<64u){unsigned next=(step+distance)%64u;
        if(rhythm[next/16u]>>(next%16u)&1u)break;
        distance++;}
    span=(step_tick(p,step+distance)+384u-step_tick(p,step))%384u;
    return span?span:384u;
}
static void generator_step(unsigned step)
{
    unsigned local=step%16u,bar=step/16u;
    if(!local)generator_apply();
    const composition_t *p=&generator.plan[generator.active];
    generator.step=(uint8_t)step;
    if(GEN_HAS_CHORDS(p->mode) && chord_trigger(p->style,local))
        generator_chord_on(p,bar,local,chord_length(p->style)*p->gate/100u);
    if(GEN_HAS_BASS(p->mode) && (p->bass_rhythm[bar]>>local&1u)){
        generator_bass_off();generator.bass_note=p->bass[step];
        bus_on(generator.bass_note,gen_velocity(p,step,0));
        generator.bass_gate=(uint16_t)(hit_span(p,p->bass_rhythm,step)*p->gate/100u);
        if(!generator.bass_gate)generator.bass_gate=1;
    }
    if(GEN_HAS_MELODY(p->mode) && (p->rhythm[bar]>>local&1u)){
        generator_melody_off();generator.melody_note=p->melody[step];
        bus_on(generator.melody_note,gen_velocity(p,step,0));
        generator.melody_gate=(uint16_t)(hit_span(p,p->rhythm,step)*p->gate/100u);
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
        unsigned bar=generator.cursor/96u,within=generator.cursor%96u,local=within/6u;
        generator.resume_chord=0;
        if(GEN_HAS_CHORDS(p->mode)){
            unsigned trigger=local;
            while(trigger && !chord_trigger(p->style,trigger))trigger--;
            if(chord_trigger(p->style,trigger)){
                unsigned elapsed=within-trigger*6u,duration=chord_length(p->style)*p->gate/100u;
                if(elapsed && elapsed<duration)generator_chord_on(p,bar,trigger,duration-elapsed+1u);
            }
        }
    }
    if(generator.chord_gate && !--generator.chord_gate)generator_chord_off();
    if(generator.melody_gate && !--generator.melody_gate)generator_melody_off();
    if(generator.bass_gate && !--generator.bass_gate)generator_bass_off();
    {
        const composition_t *p=&generator.plan[generator.active];
        unsigned step=generator.cursor/6u,offset=generator.cursor%6u;
        if(offset==((step&1u)?SWING_TICKS[p->swing]:0u))generator_step(step);
    }
    generator.cursor=(uint16_t)((generator.cursor+1u)%384u);
}
static void generator_toggle(void)
{
    uint32_t f=plat_lock();
    int start=!generator.enabled || !generator.running;
    generator_off();generator.resume_chord=0;generator.enabled=(uint8_t)start;
    generator.running=generator.enabled;
    generator.preview=(uint8_t)(generator.enabled && !arp_engine.remote_running);
    generator.wait_bar=(uint8_t)(generator.enabled && arp_engine.remote_running && arp_engine.ticks%96u!=0);
    generator.cursor=arp_engine.remote_running?(uint16_t)(arp_engine.ticks%384u):0;
    if(!generator.enabled)generator_apply();
    plat_unlock(f);
}
