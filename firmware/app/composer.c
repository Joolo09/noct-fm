/* SPDX-License-Identifier: MPL-2.0
 * Four-bar composer: prepared in the UI thread; playback only reads a plan.
 * Melody, bass and chords have independent note ownership.
 *
 * Phrase model (all integer, seeded, reproducible after SAVE):
 *   bar 1  motif      bar 2  answer      bar 3  contrast / climax      bar 4  cadence on the tonic
 * Eight melody characters (flow, arpeggio, riff, sequence, pedal, leaps, call and response; AUTO picks
 * one per phrase), 24 progressions per scale family with half-bar chord changes, five chord colours.
 * ALGORITHM browses 1000 variations of the current seed; turning back returns the same phrases.
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
enum { CO_TRIAD, CO_SEVENTH, CO_ADD9, CO_SUS2, CO_MIXED };
enum { ML_AUTO, ML_FLOW, ML_ARP, ML_RIFF, ML_SEQ, ML_PEDAL, ML_LEAPS, ML_CALL };
static const char *const COLOR_NAME[NCOLORS]={"TRIADS","7THS","ADD9","SUS2","MIXED"};
static const char *const MELODY_NAME[NMELODY]={"AUTO","FLOW","ARPEGGIO","RIFF","SEQUENCE","PEDAL","LEAPS","CALL+RESP"};
static const int8_t COLOR_TONES[4][4]={{0,2,4,0},{0,2,4,6},{0,2,4,8},{0,1,4,0}};   /* scale degrees above the root */
static const uint8_t COLOR_COUNT[4]={3,4,4,3};
#define GEN_VAR ((unsigned)proj.gen_var_lo|(unsigned)proj.gen_var_hi<<8)
typedef struct {
    uint8_t melody[64],bass[64];
    int8_t accent[64];                 /* velocity offsets, 0 when HUMAN is 0 */
    uint8_t chords[8][4],degree[8],color[8],voices[8];   /* per half bar (equal halves when a bar has one chord) */
    uint16_t rhythm[4],bass_rhythm[4];
    uint8_t split;                     /* bit per bar: the chord changes at beat 3 */
    uint8_t mode,gate,style,swing,minor,character,progression;
} composition_t;
static struct {
    composition_t plan[2];
    volatile uint8_t active,pending;
    uint8_t enabled,running,step,preview,wait_bar,resume_chord;
    uint8_t notes[4],count,melody_note,bass_note;
    uint16_t cursor,chord_gate,melody_gate,bass_gate;
    uint32_t history[8];               /* previous seeds for undo (UI thread only) */
    uint16_t history_var[8];
    uint8_t history_n;
} generator;
/* Degrees per half bar (0 = I/i). The first eight keep one chord per bar (as in 0.8.0). */
static const uint8_t PROGRESSIONS[2][NPROGS][8]={
    {{0,0,5,5,3,3,4,4},{0,0,3,3,5,5,4,4},{0,0,4,4,5,5,3,3},{1,1,4,4,0,0,0,0},
     {5,5,3,3,0,0,4,4},{0,0,2,2,3,3,4,4},{3,3,4,4,2,2,5,5},{0,0,3,3,0,0,4,4},
     {0,0,4,4,5,5,3,4},{0,4,5,3,0,4,5,3},{0,0,5,5,1,1,4,4},{1,1,4,4,0,0,5,5},
     {0,0,3,3,0,4,5,4},{0,2,5,5,3,1,4,4},{5,5,4,4,3,3,4,4},{0,0,4,4,3,3,3,4},
     {3,3,0,0,4,4,5,5},{0,5,3,4,0,5,3,4},{2,2,5,5,1,1,4,4},{0,0,1,1,2,2,3,4},
     {5,5,1,1,4,4,0,0},{3,3,4,4,0,0,0,4},{0,3,0,4,5,3,1,4},{5,3,0,4,5,3,4,4}},
    {{0,0,5,5,2,2,6,6},{0,0,3,3,5,5,4,4},{0,0,6,6,5,5,6,6},{0,0,2,2,6,6,3,3},
     {0,0,3,3,4,4,0,0},{5,5,6,6,0,0,0,0},{0,0,5,5,3,3,4,4},{0,0,6,6,3,3,5,5},
     {0,0,3,3,6,6,2,2},{5,5,6,6,0,0,0,6},{0,0,6,6,5,5,4,4},{0,5,6,0,0,5,6,4},
     {0,0,2,2,3,3,5,5},{3,3,5,5,6,6,0,0},{0,0,6,6,2,2,5,5},{0,0,3,3,0,0,3,4},
     {5,5,2,2,6,6,0,0},{0,0,4,4,5,5,3,3},{0,0,5,5,3,3,6,6},{1,1,4,4,0,0,0,0},
     {0,0,2,2,6,6,5,5},{0,5,0,5,3,6,3,6},{5,5,3,3,0,0,6,6},{0,0,6,6,5,5,6,4}}};
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
static char *roman(char *out,unsigned scale,int degree)
{
    static const char *const upper[7]={"I","II","III","IV","V","VI","VII"};
    static const char *const lower[7]={"i","ii","iii","iv","v","vi","vii"};
    int q=degree_quality(scale,degree);const char *r=q==1||q==2?lower[degree%7]:upper[degree%7];
    while(*r)*out++=*r++;
    if(q==2)*out++='o';
    if(q==3)*out++='+';
    return out;
}
/* Roman numerals of a progression, e.g. "i-VI-III-VII"; a bar with two chords reads "V/vi". */
static char *harmony_text(char *out,unsigned scale,unsigned prog)
{
    const uint8_t *d=PROGRESSIONS[SCALE_MINOR[scale%NGSCALES]][prog%NPROGS];
    for(int bar=0;bar<4;bar++){
        if(bar)*out++='-';
        out=roman(out,scale,d[bar*2]);
        if(d[bar*2+1]!=d[bar*2]){*out++='/';out=roman(out,scale,d[bar*2+1]);}
    }
    *out=0;
    return out;
}
static const char *harmony_now(void)
{
    static char text[48];char *p=text;
    if(proj.gen_progression>=NPROGS){
        const char *a="AUTO ";while(*a)*p++=*a++;
        harmony_text(p,GEN_SCALE,generator.plan[generator.pending?generator.pending-1u:generator.active].progression);
    }else harmony_text(text,GEN_SCALE,proj.gen_progression);
    return text;
}

/* ---- composition (UI thread) */
enum { RH_FLOW, RH_CONTRAST, RH_GRID, RH_SYNCO, RH_SPARSE };
/* Exactly `density` hits over `steps` (16, or 8 for a riff cell): the downbeat, beat 3, then weighted picks. */
static uint16_t compose_rhythm(uint32_t *rnd,int density,int template,int steps)
{
    static const uint8_t weight[5][16]={
        {0,3,7,5,9,3,7,5,0,3,7,5,9,3,7,5},
        {0,4,9,7,6,4,9,6,0,4,9,7,6,4,9,7},
        {0,2,9,2,8,2,9,2,0,2,9,2,8,2,9,2},
        {0,3,5,9,2,7,9,3,0,3,5,9,2,7,9,3},
        {0,2,4,8,9,2,7,3,0,2,4,8,9,2,7,3}};
    uint16_t bits=0;int n=0;
    if(density<=0)return 0;
    if(density>steps)density=steps;
    bits|=1u;n=1;
    if(density>1 && steps==16){bits|=1u<<8;n=2;}
    for(;n<density;n++){
        unsigned total=0,r;
        for(int i=0;i<steps;i++)if(!(bits>>i&1u))total+=weight[template][i]?weight[template][i]:1u;
        r=music_pick(rnd,total);
        for(int i=0;i<steps;i++)if(!(bits>>i&1u)){
            unsigned w=weight[template][i]?weight[template][i]:1u;
            if(r<w){bits|=(uint16_t)(1u<<i);break;}
            r-=w;
        }
    }
    return bits;
}
/* A riff: a half-bar cell played twice (the repeat keeps the cell's first hits so the count stays exact). */
static uint16_t compose_riff(uint32_t *rnd,int density)
{
    int first=(density+1)/2,second=density-first;
    uint16_t cell=compose_rhythm(rnd,first,RH_SYNCO,8),bits=cell;
    for(int i=0;i<8 && second;i++)if(cell>>i&1u){bits|=(uint16_t)(1u<<(i+8));second--;}
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
static void compose_contour(uint32_t *rnd,int8_t *moves,int rise,int leaps)
{
    static const int8_t steps[12]={-1,1,-1,1,-2,2,0,1,-1,3,-3,2};
    static const int8_t wide[8]={3,-3,4,-4,2,-2,5,-5};
    int last=0;
    for(int i=0;i<16;i++){
        int m=leaps?wide[music_pick(rnd,8)]:steps[music_pick(rnd,12)];
        if(!leaps && (last>=3 || last<=-3))m=last>0?-1:1;
        if(rise && i<4 && m<0)m=-m;
        moves[i]=(int8_t)m;last=m;
    }
}
static int chord_tone_near(int d,int root,unsigned color)
{
    int best=d,cost=100;
    for(int v=0;v<COLOR_COUNT[color&3];v++)for(int oct=-2;oct<=2;oct++){
        int candidate=root+COLOR_TONES[color&3][v]+7*oct,distance=music_abs(candidate-d);
        if(distance<cost || (distance==cost && candidate<best)){best=candidate;cost=distance;}
    }
    return best;
}
/* The chord root's octave nearest `target` (in scale degrees). */
static int root_near(int root,int target)
{while(root+7<=target)root+=7;while(root>target)root-=7;return target-root>3?root+7:root;}
static int fold_degree(int d,int range)
{
    while(d<0 || d>range){
        if(d<0)d=-d;
        if(d>range)d=2*range-d;
    }
    return d;
}
/* MIXED: dominants take the seventh, tonics an add9 or a plain triad, the rest vary. */
static unsigned chord_color(uint32_t *rnd,int degree,int minor)
{
    if(proj.gen_color<CO_MIXED)return proj.gen_color;
    if(degree==4)return CO_SEVENTH;
    if(degree==0)return music_pick(rnd,2)?CO_ADD9:CO_TRIAD;
    if(degree==6 && !minor)return CO_TRIAD;
    {static const uint8_t pool[5]={CO_SEVENTH,CO_TRIAD,CO_ADD9,CO_SEVENTH,CO_SUS2};return pool[music_pick(rnd,5)];}
}
static uint32_t gen_effective_seed(void)
{
    uint32_t s=proj.gen_seed?proj.gen_seed:978437u;unsigned v=GEN_VAR;
    if(v){s^=v*2654435761u;music_random(&s);music_random(&s);if(!s)s=978437u;}
    return s;
}
static void generator_regenerate(void)
{
    uint32_t f=plat_lock();generator.pending=0;
    int dest=1-generator.active;plat_unlock(f);
    composition_t *p=&generator.plan[dest];
    unsigned scale_index=GEN_SCALE<NGSCALES?GEN_SCALE:0u;
    const uint8_t *scale=SCALE_STEPS[scale_index];
    uint8_t skip=SCALE_SKIPS(scale_index);
    int minor=SCALE_MINOR[scale_index];
    uint32_t random=gen_effective_seed();
    /* HARMONY AUTO (the value after the 24 progressions): each variation picks its own progression. */
    int prog=proj.gen_progression<NPROGS?proj.gen_progression:(int)music_pick(&random,NPROGS);
    const uint8_t *progression=PROGRESSIONS[minor][prog];
    int base=60,range=GEN_OCTAVES*7,previous[4];
    int transpose=(int)GEN_ROOT-60+12*proj.octave,density=GEN_DENSITY;
    int lift=(proj.gen_mode==GM_BOTH || proj.gen_mode==GM_FULL)?12:0;
    int8_t motif[16],contrast[16],cell[8];
    int character=proj.gen_melody?proj.gen_melody:1+(int)music_pick(&random,7);
    memset(p,0,sizeof *p);
    p->mode=proj.gen_mode;p->gate=GEN_GATE;p->character=(uint8_t)character;p->progression=(uint8_t)prog;
    p->style=proj.gen_style;p->swing=proj.gen_swing;p->minor=(uint8_t)minor;
    /* Rhythm by character. */
    if(character==ML_RIFF){
        uint16_t r=compose_riff(&random,density);
        p->rhythm[0]=p->rhythm[1]=p->rhythm[3]=r;p->rhythm[2]=compose_vary(&random,r);
    }else{
        int t=character==ML_ARP || character==ML_PEDAL?RH_GRID:character==ML_LEAPS?RH_SPARSE:RH_FLOW;
        uint16_t r=compose_rhythm(&random,density,t,16);
        p->rhythm[0]=p->rhythm[3]=r;
        p->rhythm[1]=character==ML_SEQ?r:compose_vary(&random,r);
        p->rhythm[2]=character==ML_SEQ?r:compose_rhythm(&random,density,t==RH_FLOW?RH_CONTRAST:t,16);
    }
    compose_contour(&random,motif,character==ML_CALL,character==ML_LEAPS);
    compose_contour(&random,contrast,1,character==ML_LEAPS);
    {
        /* riff cell: chord-tone heavy, never three equal notes in a row */
        static const int8_t cells[10]={0,2,4,1,-1,2,4,7,-3,5};
        cell[0]=0;
        for(int i=1;i<8;i++){
            cell[i]=cells[music_pick(&random,10)];
            if(i>1 && cell[i]==cell[i-1] && cell[i]==cell[i-2])cell[i]=(int8_t)(cell[i]+2);
        }
    }
    int arp_pattern=(int)music_pick(&random,4),shift=(int)music_pick(&random,3)+1;
    if(music_pick(&random,4)==0)shift=-1;
    int pedal=range>=7?7:4;
    /* Harmony: half-bar chords with their colours, voiced nearest the previous chord. */
    previous[0]=base-12;previous[1]=base-8;previous[2]=base-5;previous[3]=base-1;
    for(int half=0;half<8;half++){
        int degree=progression[half];
        p->degree[half]=(uint8_t)degree;
        if(half&1 && degree==progression[half-1]){
            p->color[half]=p->color[half-1];p->voices[half]=p->voices[half-1];
            memcpy(p->chords[half],p->chords[half-1],4);continue;
        }
        if(half&1)p->split|=(uint8_t)(1u<<(half/2));
        unsigned color=chord_color(&random,degree,minor);
        int n=COLOR_COUNT[color],best=100000,chosen[4]={0,0,0,0};
        p->color[half]=(uint8_t)color;p->voices[half]=(uint8_t)n;
        for(int inv=0;inv<n;inv++)for(int oct=-2;oct<=0;oct++){
            int candidate[4],cost=0;
            for(int v=0;v<n;v++){
                int member=v+inv;
                candidate[v]=base+scale_pitch(degree+COLOR_TONES[color][member%n],scale)+12*(member/n+oct);
                if(GEN_OCTAVES>1 && v==n-1)candidate[v]+=12;
                if(GEN_OCTAVES==3 && v==0)candidate[v]-=12;
                if(candidate[v]<0 || candidate[v]>115){cost=100000;break;}
                cost+=music_abs(candidate[v]-previous[v]);
            }
            for(int v=1;v<n && cost<100000;v++)if(candidate[v]<=candidate[v-1])cost+=1000;   /* no crossed voices */
            if(cost<best){best=cost;memcpy(chosen,candidate,sizeof chosen);}
        }
        for(int v=0;v<n;v++){p->chords[half][v]=(uint8_t)music_midi_pitch(chosen[v]+transpose);previous[v]=chosen[v];}
    }
    /* Melody. */
    int center=range/2,degree_now=chord_tone_near(center,progression[0],CO_TRIAD);
    int approach=minor?1:(music_pick(&random,2)?1:-1);
    int seq_start=root_near(progression[0],center>2?center-2:0);
    for(int bar=0;bar<4;bar++){
        const int8_t *moves=bar==2?contrast:motif;
        int hits[16],count=0,arp_base=0,riff_root=0,walk=0;
        for(int s=0;s<16;s++)if(p->rhythm[bar]>>s&1u)hits[count++]=s;
        if(bar==1 || bar==3)degree_now=chord_tone_near(degree_now,p->degree[bar*2],CO_TRIAD);
        if(bar==2)degree_now=fold_degree(chord_tone_near(degree_now+2,p->degree[bar*2],CO_TRIAD),range);
        for(int h=0;h<count;h++){
            int s=hits[h],half=bar*2+(s>=8),chord=p->degree[half];unsigned color=p->color[half];
            int n=COLOR_COUNT[color&3],d;
            switch(character){
            case ML_ARP:{
                int k;
                if(!h || (s>=8 && hits[h-1]<8))arp_base=root_near(chord,center-3+(bar==2?2:0));
                k=arp_pattern==0?h:arp_pattern==1?n-h:arp_pattern==2?(h%(2*n-2)<n?h%(2*n-2):2*n-2-h%(2*n-2)):(int)"\0\2\1\3"[h%4]+(h/4)*n;
                {int m=((k%n)+n)%n,o=(k-m)/n;d=arp_base+COLOR_TONES[color&3][m]+7*o;}
                break;}
            case ML_RIFF:
                if(!h || (s>=8 && hits[h-1]<8))riff_root=root_near(chord,center-2);
                d=riff_root+cell[s%8];
                break;
            case ML_SEQ:
                if(!h){walk=chord_tone_near(seq_start+bar*shift,chord,CO_TRIAD);d=walk;}
                else{walk+=moves[h]>2?2:moves[h]<-2?-2:moves[h];d=walk;}
                break;
            case ML_PEDAL:
                /* a held top note (tonic, then the chord tone nearest it in bar 3) against a moving line below */
                {int top=bar==2?chord_tone_near(pedal-1,chord,color):pedal;
                if(!(h&1))d=top;
                else{walk=h==1?chord_tone_near(top-3,chord,color):walk+moves[h];
                    if(walk>=top)walk=top-1-(walk-top);
                    if(walk<top-6)walk=top-6;
                    d=walk;}}
                break;
            case ML_LEAPS:
                d=chord_tone_near(h?degree_now+moves[h]:degree_now,chord,color);
                break;
            case ML_CALL:
                /* the call rises in the first half, the answer falls from a third lower */
                if(s<8)d=h?degree_now+music_abs(moves[h]):degree_now;
                else if(!h || hits[h-1]<8)d=degree_now-2;
                else d=degree_now-music_abs(moves[h]);
                if(!(s%4))d=chord_tone_near(d,chord,color);
                break;
            default:
                d=h?degree_now+moves[h]:degree_now;
                if(!(s%4))d=chord_tone_near(d,chord,color);
            }
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
    }
    /* Bass: chord roots low; FULL plays root / second chord or fifth / approach, BASS follows the melody's rhythm. */
    for(int bar=0;bar<4;bar++){
        int roots[2],fifth;
        for(int i=0;i<2;i++){roots[i]=base+scale_pitch(p->degree[bar*2+i],scale)-24;while(roots[i]>base-14)roots[i]-=12;}
        fifth=roots[0]+scale_pitch(p->degree[bar*2]+4,scale)-scale_pitch(p->degree[bar*2],scale);
        int next_root=base+scale_pitch(p->degree[((bar+1)&3)*2],scale)-24;
        while(next_root>base-14)next_root-=12;
        if(p->mode==GM_BASS)p->bass_rhythm[bar]=p->rhythm[bar]?p->rhythm[bar]:1u;
        else p->bass_rhythm[bar]=(uint16_t)(1u|1u<<8|((density>=6 && music_pick(&random,2))?1u<<14:0u));
        for(int s=0;s<16;s++)if(p->bass_rhythm[bar]>>s&1u){
            int root=roots[s>=8],pitch=root,last=!(p->bass_rhythm[bar]>>s>>1);
            if(!s || (s==8 && (p->split>>bar&1u)))pitch=root;
            else if(last && s>=12 && next_root!=root)pitch=next_root+(next_root>root?-1:1);   /* chromatic approach */
            else if(s==8)pitch=music_pick(&random,3)?fifth:root+12;
            else if(s>8 && (p->split>>bar&1u))pitch=music_pick(&random,2)?root+12:root;
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
static void history_push(void)
{
    if(generator.history_n<8u)generator.history_n++;
    for(int i=7;i>0;i--){generator.history[i]=generator.history[i-1];generator.history_var[i]=generator.history_var[i-1];}
    generator.history[0]=proj.gen_seed;generator.history_var[0]=(uint16_t)GEN_VAR;
}
static void gen_set_var(unsigned v){proj.gen_var_lo=(uint8_t)(v&255u);proj.gen_var_hi=(uint8_t)(v>>8);}
/* New melody and rhythm (variation #000 of a new seed); the previous one is kept for undo (SEQ + EDIT). */
static void generator_new_seed(int new_harmony)
{
    history_push();
    music_random(&proj.gen_seed);
    if(!proj.gen_seed)proj.gen_seed=978437u;
    gen_set_var(0);
    if(new_harmony && proj.gen_progression<NPROGS){uint32_t r=proj.gen_seed;proj.gen_progression=(uint8_t)((proj.gen_progression+1u+music_pick(&r,NPROGS-1u))%NPROGS);}
    generator_regenerate();
}
/* ALGORITHM: the next / previous variation of the same seed; the same number always gives the same phrase. */
static void generator_vary(int delta)
{
    int v=((int)GEN_VAR+delta)%NVARS;
    if(v<0)v+=NVARS;
    gen_set_var((unsigned)v);
    generator_regenerate();
}
static int generator_undo(void)
{
    if(!generator.history_n)return 0;
    proj.gen_seed=generator.history[0];gen_set_var(generator.history_var[0]);
    for(int i=0;i<7;i++){generator.history[i]=generator.history[i+1];generator.history_var[i]=generator.history_var[i+1];}
    generator.history_n--;
    generator_regenerate();
    return 1;
}

/* ---- playback (timer) */
#define PLAN_SPLIT(p,bar) ((p)->split>>((bar)&3u)&1u)
static int chord_trigger(unsigned style,unsigned local,unsigned split)
{
    if(style==ST_PULSE)return !(local%4u);
    if(style==ST_OFFBEAT)return local%4u==2u;
    if(style==ST_BROKEN)return !(local%2u);
    return !local || (split && local==8u);
}
static unsigned chord_length(unsigned style,unsigned split){return style==ST_PULSE?24u:style==ST_HOLD?(split?48u:96u):12u;}
static uint32_t gen_velocity(const composition_t *p,unsigned step,int extra)
{return (uint32_t)clamp((int)proj.vel+p->accent[step&63u]+extra,1,127);}
static void generator_chord_on(const composition_t *p,unsigned bar,unsigned local,unsigned remaining)
{
    unsigned half=bar*2u+(local>=8u);
    generator_chord_off();
    if(p->style==ST_BROKEN){
        unsigned voice=(local/2u)%p->voices[half];
        generator.notes[0]=p->chords[half][voice];generator.count=1;
    }else{
        for(int i=0;i<p->voices[half];i++)generator.notes[i]=p->chords[half][i];
        generator.count=p->voices[half];
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
    if(GEN_HAS_CHORDS(p->mode) && chord_trigger(p->style,local,PLAN_SPLIT(p,bar)))
        generator_chord_on(p,bar,local,chord_length(p->style,PLAN_SPLIT(p,bar))*p->gate/100u);
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
            unsigned split=PLAN_SPLIT(p,bar);
            while(trigger && !chord_trigger(p->style,trigger,split))trigger--;
            if(chord_trigger(p->style,trigger,split)){
                unsigned elapsed=within-trigger*6u,duration=chord_length(p->style,split)*p->gate/100u;
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
