/* SPDX-License-Identifier: MPL-2.0 */
static const char *const CLOCK_NAME[3]={"AUTO","INTERNAL","EXTERNAL"};
static const char *const NOTE_NAME[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static char *note_text(char *out,int pitch)
{
    pitch=clamp(pitch,0,127);out=put_s(out,NOTE_NAME[pitch%12]);
    if(pitch<12)return put_s(out,"-1");
    return put_u(out,(uint32_t)(pitch/12-1));
}
/* A chord symbol from the composer's plan, e.g. "Am7", "F", "Cadd9", "Dsus2" (FONT_XS/S/B only: lowercase). */
static char *chord_symbol(char *out,const composition_t *p,int half,unsigned scale)
{
    const uint8_t *st=SCALE_STEPS[scale%NGSCALES];half&=7;int degree=p->degree[half];
    int q=degree_quality(scale,degree),pc=((int)GEN_ROOT+st[degree%7])%12;unsigned color=p->color[half];
    out=put_s(out,NOTE_NAME[pc]);
    if(color==CO_SUS2)return put_s(out,"sus2");
    if(color==CO_ADD9)return put_s(out,q==1?"madd9":q==2?"dim9":"add9");
    if(color==CO_SEVENTH){
        int seventh=scale_pitch(degree+6,st)-scale_pitch(degree,st);
        if(q==0)return put_s(out,seventh==11?"maj7":"7");
        if(q==1)return put_s(out,seventh==11?"mM7":"m7");
        if(q==2)return put_s(out,seventh==9?"dim7":"m7b5");
        return put_s(out,"+M7");
    }
    return put_s(out,q==1?"m":q==2?"dim":q==3?"+":"");
}
/* A bar's chords: "Am" or "F/G" when it changes at beat 3. */
static char *bar_symbol(char *out,const composition_t *p,int bar,unsigned scale)
{
    out=chord_symbol(out,p,bar*2,scale);
    if(p->split>>(bar&3)&1u){out=put_s(out,"/");out=chord_symbol(out,p,bar*2+1,scale);}
    return out;
}
static const char *run_label(void)
{
    if(generator.running){
        if(generator.wait_bar)return "ARMED";
        if(clock_external() && plat_ms()-arp_engine.last_clock>500u)return "WAIT";
        return generator.preview?"LOCAL":"PLAY";
    }
    return ui.playing?"PLAY":generator.enabled?"PAUSE":"STOP";
}
/* Ring position 0..127 for a value inside lo..hi. */
static int knob_level(int value,int lo,int hi){return hi>lo?clamp((value-lo)*127/(hi-lo),0,127):0;}
static void knob_text(int page,int knob,char *label,char *value,int *level)
{
    *level=-1;
    if(page==PAGE_DAW){if(!KNOB_MAP && knob<2)put_s(label,knob?"PAN":"VOLUME");else put_u(put_s(label,"CC "),(KNOB_MAP?24u:20u)+(unsigned)knob);put_u(value,proj.val[KNOB_MAP?1:0][knob]);*level=proj.val[KNOB_MAP?1:0][knob];}
    else if(page==PAGE_CHORD){
        static const char *const names[4]={"QUALITY","VOICING","STRUM","KEYS"};put_s(label,names[knob]);
        if(knob==0){put_s(value,CHORD_NAME[CHORD_TYPE]);*level=knob_level(CHORD_TYPE,0,NCHORDS-1);}
        if(knob==1){put_s(value,VOICING_NAME[proj.chord_voicing]);*level=knob_level(proj.chord_voicing,0,NVOICINGS-1);}
        if(knob==2){
            if(STRUM_MS[proj.chord_strum])put_s(put_u(value,STRUM_MS[proj.chord_strum])," MS");else put_s(value,"OFF");
            *level=knob_level(proj.chord_strum,0,NSTRUMS-1);
        }
        if(knob==3){put_s(value,proj.key_scale?"IN SCALE":"CHROMATIC");*level=proj.key_scale?127:0;}
    }else if(page==PAGE_GEN){
        static const char *const names[4]={"MODE","SCALE","ROOT","HARMONY"};put_s(label,names[knob]);
        if(knob==0){put_s(value,GEN_MODE_NAME[proj.gen_mode]);*level=knob_level(proj.gen_mode,0,NGMODES-1);}
        if(knob==1){put_s(value,SCALE_NAME[GEN_SCALE]);*level=knob_level(GEN_SCALE,0,NGSCALES-1);}
        if(knob==2){note_text(value,music_midi_pitch(GEN_ROOT+12*proj.octave));*level=knob_level(GEN_ROOT,24,96);}
        if(knob==3){if(proj.gen_progression>=NPROGS)put_s(value,"AUTO");else harmony_text(value,GEN_SCALE,proj.gen_progression);*level=knob_level(proj.gen_progression,0,NPROGS);}
    }else if(page==PAGE_FEEL){
        static const char *const names[4]={"NOTES/BAR","LENGTH","SWING","HUMAN"};put_s(label,names[knob]);
        if(knob==0){put_u(value,GEN_DENSITY);*level=knob_level(GEN_DENSITY,0,16);}
        if(knob==1){put_s(put_u(value,GEN_GATE),"%");*level=knob_level(GEN_GATE,10,100);}
        if(knob==2){put_s(value,SWING_NAME[proj.gen_swing]);*level=knob_level(proj.gen_swing,0,NSWING-1);}
        if(knob==3){if(proj.human)put_s(put_u(value,proj.human*100u/40u),"%");else put_s(value,"FLAT");*level=knob_level(proj.human,0,40);}
    }else if(page==PAGE_STYLE){
        static const char *const names[4]={"CHORD STYLE","COLOR","RANGE","MELODY"};put_s(label,names[knob]);
        if(knob==0){put_s(value,STYLE_NAME[proj.gen_style]);*level=knob_level(proj.gen_style,0,NSTYLES-1);}
        if(knob==1){put_s(value,COLOR_NAME[proj.gen_color%NCOLORS]);*level=knob_level(proj.gen_color,0,NCOLORS-1);}
        if(knob==2){put_s(put_u(value,GEN_OCTAVES)," OCT");*level=knob_level(GEN_OCTAVES,1,3);}
        if(knob==3){put_s(value,MELODY_NAME[proj.gen_melody%NMELODY]);*level=knob_level(proj.gen_melody,0,NMELODY-1);}
    }else if(page==PAGE_ARP){
        static const char *const names[4]={"RATE","ORDER","RANGE","GATE"};put_s(label,names[knob]);
        if(knob==0){put_s(value,RATE_NAME[proj.arp_rate]);*level=knob_level(proj.arp_rate,0,NRATE-1);}
        if(knob==1){put_s(value,ARP_NAME[proj.arp_mode]);*level=knob_level(proj.arp_mode,0,NARP-1);}
        if(knob==2){put_s(put_u(value,proj.arp_oct)," OCT");*level=knob_level(proj.arp_oct,1,3);}
        if(knob==3){put_s(put_u(value,proj.arp_gate),"%");*level=knob_level(proj.arp_gate,10,100);}
    }else{
        static const char *const names[4]={"CLOCK","BPM","VELOCITY","TOUCH"};put_s(label,names[knob]);
        if(knob==0){put_s(value,CLOCK_NAME[proj.clock]);*level=knob_level(proj.clock,0,NCLK-1);}
        if(knob==1){put_u(value,proj.bpm10/10u);*level=knob_level(proj.bpm10,600,2000);}
        if(knob==2){put_u(value,proj.vel);*level=proj.vel;}
        if(knob==3){put_s(value,proj.touch?"SENSED":"FIXED");*level=proj.touch?127:0;}
    }
}
static void sounding_text(char *out)
{
    int count=0;
    for(int i=0;i<128;i++)if(ui_sounding(i)){
        if(count)out=put_s(out," ");
        out=note_text(out,i);
        if(++count==5)break;
    }
    if(!count)put_s(out,"PLAY A KEY");
}
#include "afterglow.c"
