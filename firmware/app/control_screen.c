/* SPDX-License-Identifier: MPL-2.0 */
static const char *const SCALE_NAME[3]={"MAJOR","MINOR","MIN PENTA"};
static const char *const HARMONY_NAME[2][4]={{"I-vi-IV-V","I-IV-vi-V","I-V-vi-IV","ii-V-I-I"},{"i-VI-III-VII","i-iv-VI-v","i-VII-VI-VII","i-III-VII-iv"}};
static const char *const CLOCK_NAME[3]={"AUTO","INTERNAL","EXTERNAL"};
static char *note_text(char *out,int pitch)
{
    static const char *const names[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    pitch=clamp(pitch,0,127);out=put_s(out,names[pitch%12]);
    if(pitch<12)return put_s(out,"-1");
    return put_u(out,(uint32_t)(pitch/12-1));
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
static void knob_text(int page,int knob,char *label,char *value,int *level)
{
    *level=-1;
    if(page==PAGE_DAW){if(knob<2)put_s(label,knob?"PAN":"VOLUME");else put_u(put_s(label,"CC "),20u+(unsigned)knob);put_u(value,proj.val[0][knob]);*level=proj.val[0][knob];}
    else if(page==PAGE_CHORD){
        static const char *const names[4]={"QUALITY","VELOCITY","TOUCH","OCTAVE"};put_s(label,names[knob]);
        if(knob==0)put_s(value,CHORD_NAME[CHORD_TYPE]);
        if(knob==1)put_u(value,proj.vel);
        if(knob==2)put_s(value,proj.touch?"SENSED":"FIXED");
        if(knob==3){char *p=put_s(value,proj.octave<0?"-":"+");put_u(p,(unsigned)(proj.octave<0?-proj.octave:proj.octave));}
    }else if(page==PAGE_GEN){
        static const char *const names[4]={"MODE","SCALE","ROOT","HARMONY"};put_s(label,names[knob]);
        if(knob==0)put_s(value,GEN_MODE_NAME[proj.gen_mode]);
        if(knob==1)put_s(value,SCALE_NAME[GEN_SCALE]);
        if(knob==2)note_text(value,GEN_ROOT+12*proj.octave);
        if(knob==3)put_s(value,HARMONY_NAME[GEN_SCALE!=0][proj.gen_progression]);
    }else if(page==PAGE_FEEL){
        static const char *const names[4]={"NOTES/BAR","LENGTH","RANGE","VELOCITY"};put_s(label,names[knob]);
        if(knob==0)put_u(value,GEN_DENSITY);
        if(knob==1)put_s(put_u(value,GEN_GATE),"%");
        if(knob==2)put_s(put_u(value,GEN_OCTAVES)," OCT");
        if(knob==3)put_u(value,proj.vel);
    }else if(page==PAGE_ARP){
        static const char *const names[4]={"RATE","ORDER","RANGE","BPM"};put_s(label,names[knob]);
        if(knob==0)put_s(value,RATE_NAME[proj.arp_rate]);
        if(knob==1)put_s(value,ARP_NAME[proj.arp_mode]);
        if(knob==2)put_s(put_u(value,proj.arp_oct)," OCT");
        if(knob==3)put_u(value,proj.bpm10/10u);
    }else{
        static const char *const names[4]={"CLOCK","BPM","VELOCITY","TOUCH"};put_s(label,names[knob]);
        if(knob==0)put_s(value,CLOCK_NAME[proj.clock]);
        if(knob==1)put_u(value,proj.bpm10/10u);
        if(knob==2)put_u(value,proj.vel);
        if(knob==3)put_s(value,proj.touch?"SENSED":"FIXED");
    }
}
static void sounding_text(char *out)
{
    int count=0;
    for(int i=0;i<128;i++)if(note_owners[i]){
        if(count)out=put_s(out," ");
        out=note_text(out,i);
        if(++count==5)break;
    }
    if(!count)put_s(out,"PLAY A KEY");
}
#include "afterglow.c"
