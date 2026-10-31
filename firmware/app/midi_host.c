/* SPDX-License-Identifier: MPL-2.0 */
static uint8_t simulated_clock;
static uint32_t simulated_phase;
void app_host_sim_tick(void)
{
    if(!simulated_clock)return;
    simulated_phase+=proj.bpm10*24u;
    if(simulated_phase>=600000u){
        simulated_phase-=600000u;
        min_q[mi_w%MQ]=0xFu|0xF8u<<8;min_port[mi_w++%MQ]=PORT_USB;
    }
}
static int out_count(int which)
{
    uint32_t i;int n=0;
    for(i=0;i<n_out;i++){
        uint32_t st=out_pkt[i]>>8&255u,on=(st&240u)==144u&&(out_pkt[i]>>24);
        if(which==0)n+=st==248u;
        if(which==1)n+=(int)on;
        if(which==2)n+=on?1:((st&240u)==128u||((st&240u)==144u&&!on))?-1:0;
    }
    return n;
}
int app_expect(const char *what,int *got)
{
    int n=0;
    if(!strcmp(what,"led_keys"))n=(int)ui.led_keys;
    else if(!strcmp(what,"led_dim_keys"))n=(int)ui.led_dim_keys;
    else if(!strcmp(what,"led_buttons"))n=(int)((ui.led_buttons&((1u<<NB)-1u))|((ui.led_buttons&LED_PLAY_GREEN)?1u<<B_PLAY:0u));
    else if(!strncmp(what,"led_",4)){int k=atoi(what+4);n=k>=0&&k<NKEYS?(int)(ui.led_keys>>k&1u):0;}
    else if(!strcmp(what,"sounding")){for(int i=0;i<128;i++)n+=ui_sounding(i)!=0;}
    else if(!strcmp(what,"page"))n=ui.page;
    else if(!strcmp(what,"root_note"))n=GEN_ROOT;
    else if(!strcmp(what,"gen_scale"))n=GEN_SCALE;
    else if(!strcmp(what,"gate"))n=GEN_GATE;
    else if(!strcmp(what,"velocity"))n=proj.vel;
    else if(!strcmp(what,"chan"))n=proj.chan;
    else if(!strcmp(what,"profile"))n=proj.rsv[0];
    else if(!strcmp(what,"knob_map"))n=KNOB_MAP;
    else if(!strncmp(what,"mapval",6))n=proj.val[1][atoi(what+6)&3];
    else if(!strcmp(what,"bank"))n=proj.bank;
    else if(!strcmp(what,"scale"))n=proj.scale;
    else if(!strcmp(what,"program"))n=proj.program;
    else if(!strcmp(what,"sound"))n=proj.sound;
    else if(!strcmp(what,"octave"))n=proj.octave;
    else if(!strcmp(what,"tempo"))n=proj.tempo;
    else if(!strcmp(what,"bpm10"))n=proj.bpm10;
    else if(!strcmp(what,"chords"))n=CHORD_ON;
    else if(!strcmp(what,"chord_type"))n=CHORD_TYPE;
    else if(!strcmp(what,"generator"))n=generator.enabled;
    else if(!strcmp(what,"gen_running"))n=generator.running;
    else if(!strcmp(what,"gen_mode"))n=proj.gen_mode;
    else if(!strcmp(what,"gen_pending"))n=!!generator.pending;
    else if(!strcmp(what,"gen_cursor"))n=generator.cursor;
    else if(!strcmp(what,"gen_step"))n=generator.step;
    else if(!strcmp(what,"fx"))n=ui.fx_on;
    else if(!strcmp(what,"density"))n=GEN_DENSITY;
    else if(!strcmp(what,"clock"))n=proj.clock;
    else if(!strcmp(what,"arp"))n=proj.arp_on;
    else if(!strcmp(what,"latch"))n=proj.arp_latch;
    else if(!strcmp(what,"arp_note"))n=eng_arp_note();
    else if(!strcmp(what,"pool"))n=arp_engine.count;
    else if(!strcmp(what,"active")){
        int i;for(i=0;i<128;i++)n+=note_owners[i]!=0;
    }
    else if(!strcmp(what,"arp_mode"))n=proj.arp_mode;
    else if(!strcmp(what,"arp_gate"))n=proj.arp_gate;
    else if(!strcmp(what,"swing"))n=proj.gen_swing;
    else if(!strcmp(what,"human"))n=proj.human;
    else if(!strcmp(what,"style"))n=proj.gen_style;
    else if(!strcmp(what,"color"))n=proj.gen_color;
    else if(!strcmp(what,"voicing"))n=proj.chord_voicing;
    else if(!strcmp(what,"strum"))n=proj.chord_strum;
    else if(!strcmp(what,"key_scale"))n=proj.key_scale;
    else if(!strcmp(what,"custom_n"))n=proj.custom_n;
    else if(!strncmp(what,"custom",6))n=proj.custom[atoi(what+6)%5];
    else if(!strcmp(what,"progression"))n=proj.gen_progression;
    else if(!strcmp(what,"seed"))n=(int)(proj.gen_seed&0x7FFFFFFFu);
    else if(!strcmp(what,"history"))n=generator.history_n;
    else if(!strcmp(what,"dirty"))n=ui.dirty;
    else if(!strcmp(what,"arp_rate"))n=proj.arp_rate;
    else if(!strcmp(what,"arp_oct"))n=proj.arp_oct;
    else if(!strcmp(what,"saves"))n=(int)ui.saves;
    else if(!strcmp(what,"playing"))n=ui.playing;
    else if(!strcmp(what,"recording"))n=noct_recording;
    else if(!strcmp(what,"sustain"))n=ui.sustain;
    else if(!strcmp(what,"touch"))n=touch_live;
    else if(!strcmp(what,"notes_out"))n=out_count(1);
    else if(!strcmp(what,"notes_on"))n=out_count(2);
    else if(!strcmp(what,"clocks_out"))n=out_count(0);
    else if(!strncmp(what,"cc",2))n=proj.cc[0][atoi(what+2)&3];
    else if(!strncmp(what,"val",3))n=proj.val[0][atoi(what+3)&3];
    else return 0;
    *got=n;return 1;
}
int app_host_cmd(const char *cmd,const char *a,const char *b,const char *c,const char *d)
{
    (void)a;(void)b;(void)c;(void)d;
    if(!strcmp(cmd,"splash")){app_splash();return 1;}
    if(!strcmp(cmd,"simclock")){
        simulated_clock=(uint8_t)!!atoi(a);simulated_phase=0;return 1;
    }
    if(!strcmp(cmd,"snapshot")){
        printf("@STATE {\"ms\":%u,\"page\":%u,\"playing\":%u,\"generator\":%u,\"running\":%u,\"mode\":%u,\"arp\":%u,\"chords\":%u,\"knob_map\":%u,\"bpm\":%u,\"clock\":%u,\"pending\":%u,\"notes\":[",
            now_ms,ui.page,ui.playing,generator.enabled,generator.running,proj.gen_mode,
            proj.arp_on,CHORD_ON,KNOB_MAP,proj.bpm10,proj.clock,!!generator.pending);
        int comma=0;
        for(int i=0;i<128;i++)if(note_owners[i]){printf("%s%d",comma?",":"",i);comma=1;}
        printf("],\"led_keys\":%u,\"led_dim_keys\":%u,\"led_buttons\":%u,\"led_dim_buttons\":%u,\"octave\":%d,\"sounding\":[",ui.led_keys,ui.led_dim_keys,
            (ui.led_buttons&((1u<<NB)-1u))|((ui.led_buttons&LED_PLAY_GREEN)?1u<<B_PLAY:0u),
            (ui.led_dim_buttons&((1u<<NB)-1u))|((ui.led_dim_buttons&LED_PLAY_GREEN)?1u<<B_PLAY:0u),proj.octave);
        comma=0;
        for(int i=0;i<128;i++)if(ui_sounding(i)){printf("%s%d",comma?",":"",i);comma=1;}
        printf("]}\n");return 1;
    }
    if(!strcmp(cmd,"format5")){
        /* A 0.7 save: format 5 with the 0.8 fields absent (zero). */
        proj.format=5;proj.gen_swing=proj.gen_style=proj.gen_color=proj.chord_voicing=proj.chord_strum=proj.key_scale=0;
        proj.arp_gate=0;proj.custom_n=0;memset(proj.custom,0,sizeof proj.custom);proj.human=0;
        project_save();return 1;
    }
    if(!strcmp(cmd,"legacychord")){CHORD_ON=1;proj.arp_on=1;project_save();return 1;}
    if(!strcmp(cmd,"fixed")){proj.touch=0;touch_apply();return 1;}
    if(!strcmp(cmd,"legacy")){
        /* Seed the exact previous settings format, including dangerous routes. */
        proj.format=2;proj.chan=14;proj.bank=3;proj.rsv[0]=1;proj.scale=3;
        proj.clock=CLK_EXT;proj.sound=SOUND_IN;proj.cc[0][0]=99;
        project_save();return 1;
    }
    return 0;
}
void app_host_reboot(void){simulated_clock=0;simulated_phase=0;midi_engine_init();}
void app_speak_dump(void){ann_say("NOCT MIDI. Channel one. Chromatic. Four fixed controllers.",0,ANN_NOW);}
