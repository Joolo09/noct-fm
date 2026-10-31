/* SPDX-License-Identifier: MPL-2.0
 * Shared note ownership: keyboard, chords, arp and generator.
 * NOCT 0.8: shared scales (keyboard scale lock, in-key chords and the composer),
 * chord voicings and the learned MEMORY chord.
 */
#define MAX_VOICING 5
#define CHORD_ON (proj.rsv[1])
#define CHORD_TYPE (proj.rsv[2])
#define KNOB_MAP (proj.rsv[3])
#define GEN_SCALE (proj.rsv[4])
#define GEN_DENSITY (proj.rsv[5])
#define GEN_ROOT (proj.rsv[6])
#define GEN_OCTAVES (proj.rsv[7])
#define GEN_GATE (proj.rsv[8])
#define CLK_AUTO CLK_INT_OUT
enum { CH_MAJOR, CH_MINOR, CH_SUS2, CH_SUS4, CH_MAJ7, CH_MIN7, CH_DOM7, CH_POWER, CH_KEY, CH_KEY7, CH_CUSTOM };
static const char *const CHORD_NAME[NCHORDS]={"MAJOR","MINOR","SUS2","SUS4","MAJ7","MIN7","DOM7","POWER","IN KEY","IN KEY 7TH","MEMORY"};
static const char *const VOICING_NAME[NVOICINGS]={"CLOSE","INV 1","INV 2","OPEN","DROP 2","+ BASS"};
static const uint8_t STRUM_MS[NSTRUMS]={0,6,12,18,25,35,50,70};

/* Scales: seven steps (pentatonics keep their parent scale for harmony and skip two degrees in melodies/keys). */
static const char *const SCALE_NAME[NGSCALES]={"MAJOR","MINOR","MIN PENTA","DORIAN","HARM MINOR","MAJ PENTA","MIXOLYDIAN","PHRYGIAN"};
static const uint8_t SCALE_STEPS[NGSCALES][7]={
    {0,2,4,5,7,9,11},{0,2,3,5,7,8,10},{0,2,3,5,7,8,10},{0,2,3,5,7,9,10},
    {0,2,3,5,7,8,11},{0,2,4,5,7,9,11},{0,2,4,5,7,9,10},{0,1,3,5,7,8,10}};
/* MIN PENTA skips degrees 2 and 6, MAJ PENTA degrees 4 and 7 (in melodies and scale-locked keys). */
#define SCALE_SKIPS(s) ((s)==2u?(uint8_t)((1u<<1)|(1u<<5)):(s)==5u?(uint8_t)((1u<<3)|(1u<<6)):(uint8_t)0)
static const uint8_t SCALE_MINOR[NGSCALES]={0,1,1,1,1,0,0,1};

/* 12-bit pitch-class mask of the playable notes, relative to the root. */
static uint32_t scale_mask(unsigned scale)
{
    uint32_t mask=0;uint8_t skip=SCALE_SKIPS(scale);
    for(int d=0;d<7;d++)if(!(skip>>d&1u))mask|=1u<<SCALE_STEPS[scale][d];
    return mask;
}
/* Snap down to the nearest note of the COMPOSE key (root and scale). */
static int key_snap(int note)
{
    if(!proj.key_scale)return note;
    uint32_t mask=scale_mask(GEN_SCALE);int rel=(note-(int)GEN_ROOT)%12;
    if(rel<0)rel+=12;
    for(int down=0;down<12;down++){
        int r=rel-down;if(r<0)r+=12;
        if(mask>>r&1u)return note-down;
    }
    return note;
}
/* Stacked thirds of the key on the scale degree at or below note (a key outside the scale plays the
 * chord of the degree below it, so every key gives an in-key chord). Intervals are relative to note. */
static int key_chord(int note,int voices,int8_t *intervals)
{
    const uint8_t *steps=SCALE_STEPS[GEN_SCALE];int rel=(note-(int)GEN_ROOT)%12,degree=0;
    if(rel<0)rel+=12;
    for(int d=0;d<7;d++)if(steps[d]<=rel)degree=d;
    for(int v=0;v<voices;v++){
        int d=degree+2*v;
        intervals[v]=(int8_t)(steps[d%7]+12*(d/7)-rel);
    }
    return voices;
}
static int chord_intervals(int note,int8_t *out)
{
    static const int8_t fixed[8][4]={{0,4,7,0},{0,3,7,0},{0,2,7,0},{0,5,7,0},{0,4,7,11},{0,3,7,10},{0,4,7,10},{0,7,12,0}};
    static const uint8_t counts[8]={3,3,3,3,4,4,4,3};
    unsigned type=CHORD_TYPE;
    if(type<8u){for(unsigned i=0;i<counts[type];i++)out[i]=fixed[type][i];return counts[type];}
    if(type==CH_KEY)return key_chord(note,3,out);
    if(type==CH_KEY7)return key_chord(note,4,out);
    int n=clamp(proj.custom_n,2,MAX_VOICING);
    for(int i=0;i<n;i++)out[i]=(int8_t)proj.custom[i];
    return n;
}
static void sort_notes(int *p,int n)
{for(int i=1;i<n;i++)for(int j=i;j>0 && p[j-1]>p[j];j--){int t=p[j];p[j]=p[j-1];p[j-1]=t;}}
static void apply_voicing(int *p,int *n,int root)
{
    int v=proj.chord_voicing;
    if(*n<2)return;
    if(v==1){p[0]+=12;sort_notes(p,*n);}
    else if(v==2 && *n>=3){p[0]+=12;p[1]+=12;sort_notes(p,*n);}
    else if(v==3 && *n>=3){p[1]+=12;sort_notes(p,*n);}
    else if(v==4 && *n>=3){p[*n-2]-=12;sort_notes(p,*n);}
    else if(v==5 && *n<MAX_VOICING){for(int i=*n;i>0;i--)p[i]=p[i-1];p[0]=root-12;(*n)++;}
}
static volatile uint16_t note_owners[128];
static volatile uint32_t sustained_notes[4];
static volatile uint8_t sustain_active;
static void bus_sustain(int on)
{
    uint32_t f=plat_lock();sustain_active=(uint8_t)!!on;
    if(!on)for(int i=0;i<4;i++)sustained_notes[i]=0;
    plat_unlock(f);
}
static int bus_sounding(int note)
{return note_owners[note] || (sustained_notes[note>>5]>>(note&31)&1u);}
/* Read-only visual telemetry. No drawing or extra MIDI work in the timer. */
static volatile uint32_t visual_note_serial;
static volatile uint8_t visual_last_note;
static void bus_on(int note,uint32_t velocity)
{
    if(note<0 || note>127)return;
    if(velocity<1u)velocity=1u;
    if(velocity>127u)velocity=127u;
    uint32_t f=plat_lock();
    if(!note_owners[note]++){
        sustained_notes[note>>5]&=~(1u<<(note&31));
        visual_last_note=(uint8_t)note;visual_note_serial++;
        ctl_send(0x9u|0x90u<<8|(uint32_t)note<<16|velocity<<24,TO_USB|TO_BLE);
    }
    plat_unlock(f);
}
static void bus_off(int note)
{
    if(note<0 || note>127)return;
    uint32_t f=plat_lock();
    if(!note_owners[note]){plat_unlock(f);return;}
    if(!--note_owners[note]){
        if(sustain_active)sustained_notes[note>>5]|=1u<<(note&31);
        ctl_send(0x8u|0x80u<<8|(uint32_t)note<<16,TO_USB|TO_BLE);
    }
    plat_unlock(f);
}
/* The notes one key plays: its (scale-snapped) pitch, or the chord voiced on it; ascending, valid MIDI only. */
static int note_voicing(int root,uint8_t *notes)
{
    int pitches[MAX_VOICING+1],count=1,n=0;
    int8_t intervals[MAX_VOICING];
    root=key_snap(root);
    pitches[0]=root;
    if(CHORD_ON){
        count=chord_intervals(root,intervals);
        for(int i=0;i<count;i++)pitches[i]=root+intervals[i];
        sort_notes(pitches,count);
        apply_voicing(pitches,&count,root+intervals[0]);
    }
    for(int i=0;i<count && n<MAX_VOICING;i++){
        int pitch=pitches[i],dup=0;
        for(int j=0;j<n;j++)dup|=notes[j]==pitch;
        if(pitch>=0 && pitch<128 && !dup)notes[n++]=(uint8_t)pitch;
    }
    return n;
}
