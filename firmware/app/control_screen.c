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
static void phrase_strip(int y)
{
    const composition_t *p=&generator.plan[generator.active];
    unsigned bar=generator.step/16u;
    for(int i=0;i<16;i++){
        int hit=(p->rhythm[bar]>>i)&1u;
        int h=p->mode==1?5:3+p->melody[bar*16u+(unsigned)i]%8;
        uint16_t color=generator.running && i==generator.step%16u?K_HOT:hit?K_MINT:K_LINE;
        cv_rect(9+i*14,y+10-h,10,hit||p->mode==1?h:2,color);
    }
}
static void noct_draw(void)
{
    uint32_t now=plat_ms();char text[64],label[24],value[32],*p;int page=ui.page;
    if(now-ui.sig[0]<50u && ui.frame)return;
    ui.sig[0]=now;
    cv_begin(240,120,K_BG);
    cv_text(8,4,&FONT_B,"NOCT",K_TEXT);
    cv_rect(53,9,3,3,lit(PORT_USB,0)||lit(PORT_USB,1)?K_MINT:K_LINE);
    cv_text(61,6,&FONT_XS,"USB",K_DIM);
    p=put_u(text,proj.bpm10/10u);
    if(proj.bpm10%10u)put_u(put_s(p,"."),proj.bpm10%10u);
    cv_text(96,5,&FONT_XS,text,K_TEXT);
    const char *status=run_label();
    cv_rect(181,3,51,18,generator.running||ui.playing?RGB(27,59,53):K_PANEL);
    noct_center(206,5,&FONT_XS,status,generator.running||ui.playing?K_MINT:K_DIM);
    if(noct_recording)cv_rect(173,9,4,4,K_HOT);
    static const char *const tabs[5]={"DAW","CHRD","GEN","ARP","SYNC"};
    int selected=page==PAGE_DAW?0:page==PAGE_CHORD?1:page==PAGE_ARP?3:page==PAGE_SYNC?4:2;
    for(int i=0;i<5;i++){
        int x=8+i*45;
        cv_rect(x,27,43,17,i==selected?K_PANEL:K_BG);
        noct_center(x+21,28,&FONT_XS,tabs[i],i==selected?K_MINT:K_DIM);
        if(i==selected)cv_rect(x,45,43,2,K_MINT);
    }
    if(page==PAGE_DAW){
        cv_band(49,69);noct_sky(now/100u);
        sounding_text(text);
        noct_center(120,7,&FONT_B,text,ui.down?K_MINT:K_TEXT);
        cv_text(9,31,&FONT_XS,"FOUR FIXED CONTROLLERS",K_DIM);
        noct_logo(12,47,1,0);cv_text(34,49,&FONT_XS,"FL STUDIO",K_HOT);
        noct_logo(140,47,1,1);cv_text(167,49,&FONT_XS,"LIVE",K_MINT);
    }else if(page==PAGE_CHORD){
        cv_text(9,52,&FONT_XS,CHORD_ON?"KEYBOARD / CHORDS ON":"KEYBOARD / SINGLE NOTES",K_DIM);
        cv_text(8,67,&FONT_M,CHORD_ON?CHORD_NAME[CHORD_TYPE]:"CHROMATIC",K_TEXT);
        sounding_text(text);cv_text(9,99,&FONT_XS,text,K_MINT);
    }else if(page==PAGE_GEN || page==PAGE_FEEL){
        cv_text(9,52,&FONT_XS,page==PAGE_GEN?"COMPOSE / 1 OF 2":"RHYTHM / 2 OF 2",K_DIM);
        cv_text(8,66,&FONT_B,proj.gen_mode==2?"MELODY + CHORDS":GEN_MODE_NAME[proj.gen_mode],K_TEXT);
        p=note_text(text,GEN_ROOT+12*proj.octave);p=put_s(p," ");p=put_s(p,SCALE_NAME[GEN_SCALE]);
        p=put_s(p," / BAR ");put_u(p,generator.step/16u+1u);
        cv_text(9,86,&FONT_XS,text,K_MINT);phrase_strip(104);
    }else if(page==PAGE_ARP){
        cv_text(9,52,&FONT_XS,proj.arp_on?"ARPEGGIATOR / ON":"ARPEGGIATOR / OFF",K_DIM);
        cv_text(8,67,&FONT_M,ARP_NAME[proj.arp_mode],K_TEXT);
        p=put_s(text,RATE_NAME[proj.arp_rate]);p=put_s(p," / ");put_s(p,proj.arp_latch?"HOLD ON":"HOLD OFF");
        cv_text(9,99,&FONT_XS,text,K_MINT);
    }else{
        cv_text(9,52,&FONT_XS,"TEMPO / CLOCK SOURCE",K_DIM);
        p=put_u(text,proj.bpm10/10u);put_u(put_s(p,"."),proj.bpm10%10u);
        cv_text(8,65,&FONT_L,text,K_TEXT);
        const char *sync=clock_external()?(now-arp_engine.last_clock>500u?"WAITING FOR MIDI CLOCK":"MIDI CLOCK CONNECTED"):arp_engine.daw_tempo_valid?"DAW BPM / INTERNAL GRID":"INTERNAL GRID";
        cv_text(9,103,&FONT_XS,sync,clock_external()&&now-arp_engine.last_clock>500u?K_HOT:K_MINT);
    }
    cv_blit(0,0);plat_yield();
    cv_begin(240,120,K_BG);
    for(int i=0;i<4;i++){
        int x=8+(i%2)*114,y=(i/2)*48,level,hot=ui.touched==i && now<ui.touch_until;
        knob_text(page,i,label,value,&level);
        cv_rect(x,y+1,110,44,K_PANEL);cv_rect(x,y+1,2,44,hot?K_HOT:K_LINE);
        put_u(text,(unsigned)i+1u);cv_text(x+7,y+5,&FONT_XS,text,hot?K_HOT:K_DIM);
        cv_text(x+20,y+5,&FONT_XS,label,K_DIM);
        cv_text(x+8,y+20,&FONT_B,value,hot?K_HOT:K_TEXT);
        if(level>=0){cv_rect(x+60,y+36,42,2,K_LINE);cv_rect(x+60,y+36,42*level/127,2,K_MINT);}
    }
    cv_rect(8,98,224,1,K_LINE);
    if(now<ui.msg_until)put_s(text,ui.message);
    else if(generator.pending)put_s(text,"NEXT BAR / HOME: DAW");
    else if(page==PAGE_GEN)put_s(text,"PRESET: RHYTHM  EDIT: NEW");
    else if(page==PAGE_FEEL)put_s(text,"SEQ: RUN/STOP  EDIT: NEW");
    else if(page==PAGE_DAW)put_s(text,"PRESET: PAGES  SELECT: BPM");
    else put_s(text,"PRESET: PAGES  HOME: DAW");
    cv_text(8,103,&FONT_XS,text,now<ui.msg_until?K_HOT:K_DIM);
    cv_blit(0,120);
}
