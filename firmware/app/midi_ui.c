/* SPDX-License-Identifier: MPL-2.0
 * NOCT 0.2: one MIDI preset, invariant keyboard routing, no background flash writes.
 * Uses CTL-1/fm1-base's key sensing and pixel renderer under MPL-2.0.
 */
#define K_BG RGB(10,14,25)
enum { PAGE_DAW, PAGE_CHORD, PAGE_GEN, PAGE_FEEL, PAGE_ARP, PAGE_SYNC, PAGE_COUNT };
static const char *const PAGE_NAME[PAGE_COUNT]={"DAW","CHORDS","COMPOSE","RHYTHM","ARP","SYNC"};
#define K_PANEL RGB(19,25,43)
#define K_LINE RGB(39,48,70)
#define K_DIM RGB(135,150,177)
#define K_TEXT RGB(229,237,246)
#define K_HOT RGB(255,166,77)
#define K_MINT RGB(119,235,207)
static const char *const ARP_NAME[NARP]={"UP","DOWN","UP-DOWN","RANDOM","PLAYED"};
static const char *const RATE_NAME[NRATE]={"1/4","1/8","1/8T","1/16","1/16T","1/32"};
static const uint8_t WHITE_K[16]=PLAT_WHITE_KEYS;
static struct {
    uint32_t btn,keys,down,heard[4],sig[4],frame,touch_until,msg_until,save_t;
    uint8_t key_note[NKEYS][MAX_VOICING],key_count[NKEYS],key_vel[NKEYS],key_arp[NKEYS];
    uint8_t playing,sustain,dirty,arp_used,fx_used,save_done,seq_used,glo_used,fx_on,page;
    uint8_t tempo_hi,tempo_pending;
    int8_t touched;
    int32_t master_raw;
    int16_t master_sent;
    char last[40],message[40];
    uint32_t saves,seq_t,arp_t;
    uint8_t revoicing;
} ui;
static uint8_t noct_recording;
static uint32_t noct_transition;
#define touch_live touch_is_on()
static char *put_s(char *b,const char *s){while(*s)*b++=*s++;*b=0;return b;}
static char *put_u(char *b,uint32_t n){char t[12];int k=0;do{t[k++]=(char)('0'+n%10u);n/=10u;}while(n);while(k)*b++=t[--k];*b=0;return b;}
static uint32_t pkt3(uint32_t st,uint32_t a,uint32_t b){return(st>>4)|st<<8|a<<16|b<<24;}
static void play(uint32_t st,uint32_t a,uint32_t b){ctl_send(pkt3(st|proj.chan,a,b),TO_USB|TO_BLE);}
static void noct_control(uint32_t a,uint32_t b){ctl_send(pkt3(0xBFu,a,b),TO_USB|TO_BLE);}
static void say(const char *a,const char *b)
{
    uint32_t n=0;
    while(a && *a && n<sizeof ui.message-1u)ui.message[n++]=*a++;
    if(b && n<sizeof ui.message-1u)ui.message[n++]=' ';
    while(b && *b && n<sizeof ui.message-1u)ui.message[n++]=*b++;
    ui.message[n]=0;
    ui.msg_until=plat_ms()+800u;
    if(ann_on())ann_say(ui.message,0,ANN_NOW);
}
void ui_say(const char *a,const char *b){say(a,b);}
int ui_dirty(void){return ui.dirty;}
static void key_up(int k)
{
    int i;
    if(!(ui.down>>k&1u))return;
    for(i=0;i<ui.key_count[k];i++){
        if(ui.key_arp[k])arp_release(ui.key_note[k][i]);
        else bus_off(ui.key_note[k][i]);
    }
    ui.key_count[k]=0;
    ui.down&=~(1u<<k);
}
static void key_down(int k,uint32_t vel)
{
    int n=53+k+12*proj.octave,i;
    if(k<0||k>=NKEYS||n<0||n>127)return;
    if(!ui.revoicing && (plat_buttons()>>B_SEQ&1u)){GEN_ROOT=(uint8_t)clamp(53+k,0,115);ui.seq_used=1;ui.dirty=1;generator_regenerate();return;}
    key_up(k);
    ui.down|=1u<<k;ui.key_vel[k]=(uint8_t)vel;
    ui.key_count[k]=(uint8_t)note_voicing(n,ui.key_note[k]);
    ui.key_arp[k]=proj.arp_on;
    for(i=0;i<ui.key_count[k];i++){
        if(proj.arp_on)arp_press(ui.key_note[k][i]);
        else bus_on(ui.key_note[k][i],vel);
    }
    put_u(put_s(ui.last,"NOTE "),(uint32_t)n);
}
static void all_keys_up(void){int i;for(i=0;i<NKEYS;i++)key_up(i);}
static void panic(void)
{
    all_keys_up();engine_panic();play(0xB0u,64,0);play(0xB0u,123,0);
    ui.sustain=0;put_s(ui.last,"ALL NOTES OFF");
}
static void revoice(int octave_delta,int arp_toggle)
{
    uint32_t held=ui.down;uint8_t velocity[NKEYS];int i;
    memcpy(velocity,ui.key_vel,sizeof velocity);
    all_keys_up();arp_clear();
    proj.octave=(int8_t)clamp(proj.octave+octave_delta,-3,3);
    if(arp_toggle)proj.arp_on=(uint8_t)!proj.arp_on;
    ui.revoicing=1;
    for(i=0;i<NKEYS;i++)if(held>>i&1u)key_down(i,velocity[i]);
    ui.revoicing=0;
    if(octave_delta)generator_regenerate();
    ui.dirty=1;
}
static void touch_apply(void)
{
    all_keys_up();touch_on(0);
    touch_curve(proj.vel_curve);touch_sens(proj.at_sens);
    if(proj.touch)touch_on(1);
}
static void input_keys(void)
{
    uint32_t keys=plat_keys(),changed=keys^ui.keys;int k;
    ui.keys=keys;
    for(k=0;k<NKEYS;k++)if(changed>>k&1u){
        if(touch_live && touch_sensed((uint32_t)k))continue;
        if(keys>>k&1u){uint32_t v=proj.touch?touch_vel_contact((uint32_t)k):0;key_down(k,v?v:proj.vel);}
        else key_up(k);
    }
    if(touch_live){
        touch_ev_t e;
        while(touch_take(&e)){
            if(e.type==TOUCH_PRESS)key_down(e.key,proj.touch?e.vel:proj.vel);
            else key_up(e.key);
        }
        touch_update();
    }
}
static void midi_in(void)
{
    uint32_t pkt;int port,budget=64;
    /* A sustained feedback flood must not starve key releases or encoder scans. */
    while(budget-- && plat_midi_in(&pkt,&port)){
        uint32_t st=(pkt>>8)&255u,n=(pkt>>16)&127u,v=(pkt>>24)&127u;
        if(port==PORT_TRS)ctl_send(pkt&~0xF0u,TO_USB|TO_BLE);
        /* No USB/BLE feedback echo and no synth queue in this MIDI-only app. */
        if(st>=0xF8u){
            engine_midi_clock(st);
            if(st==0xFAu || st==0xFBu)ui.playing=1;
            if(st==0xFCu){ui.playing=0;if(ui.sustain){ui.sustain=0;play(0xB0u,64,0);}}
            continue;
        }
        if(st==0xBFu){
            if(n>=20u&&n<=23u)proj.val[0][n-20u]=(uint8_t)v;
            if(n==113u)ui.fx_on=(uint8_t)!!v;
            if(n==114u){ui.tempo_hi=(uint8_t)v;ui.tempo_pending=1;}
            if(n==115u && ui.tempo_pending){
                uint32_t value=((uint32_t)ui.tempo_hi<<7)|v,f=plat_lock();
                if(value>=200u && value<=9990u){engine_tempo(value);arp_engine.daw_tempo_valid=1;}
                plat_unlock(f);ui.tempo_pending=0;
            }
            if(n==116u){
                if(!arp_engine.transport_known || ui.playing!=(uint8_t)!!v || (!v && generator.preview))engine_transport(v!=0,v!=0);
                ui.playing=(uint8_t)!!v;
                if(!v && ui.sustain){ui.sustain=0;play(0xB0u,64,0);}
            }
            if(n==117u)noct_recording=(uint8_t)!!v;
            continue;
        }
        if((st&15u)!=proj.chan)continue;
        if((st&0xF0u)==0x90u&&v)ui.heard[n>>5]|=1u<<(n&31u);
        if((st&0xF0u)==0x80u||((st&0xF0u)==0x90u&&!v))ui.heard[n>>5]&=~(1u<<(n&31u));
        if((st&0xF0u)==0xB0u&&(n==120u||n==123u))memset(ui.heard,0,sizeof ui.heard);
    }
}
static void save_now(void)
{
    ui.save_done=1;
    if(ui.down||ui.playing||arp_engine.count||generator.running||ui.sustain||plat_ms()<ui.touch_until){say("RELEASE KEYS", "STOP BEFORE SAVE");return;}
    if(project_save()==0){ui.dirty=0;ui.saves++;say("SETTINGS SAVED",0);}
    else say("FLASH SAVE FAILED",0);
}
static void save_if_idle(void)
{if(!ui.save_done && (ui.btn>>B_SAVE&1u) && plat_ms()-ui.save_t>=1000u)save_now();}
static void local_tempo(int delta)
{
    uint32_t f=plat_lock();engine_tempo((uint32_t)clamp((int32_t)proj.bpm10+delta,200,9990));plat_unlock(f);
    noct_control(114,proj.bpm10>>7);noct_control(115,proj.bpm10&127u);ui.dirty=1;
}
static void cycle_clock(void)
{proj.clock=(uint8_t)((proj.clock+1)%NCLK);revoice(0,0);say("SYNC",proj.clock==CLK_AUTO?"AUTO":proj.clock==CLK_EXT?"EXTERNAL":"INTERNAL");ui.page=PAGE_SYNC;}
static void button_down(int b)
{
    switch(b){
    case B_ARP:ui.arp_used=0;ui.arp_t=plat_ms();ui.page=PAGE_ARP;break;
    case B_SEQ:ui.seq_used=0;ui.seq_t=plat_ms();ui.page=PAGE_GEN;break;
    case B_GLO:ui.glo_used=0;break;
    case B_FX:ui.fx_used=0;break;
    case B_SAVE:ui.save_t=plat_ms();ui.save_done=0;break;
    case B_PLAY:
        if(ui.btn>>B_GLO&1u){
            ui.glo_used=1;cycle_clock();
        }else{ui.playing=(uint8_t)(!(ui.playing || (generator.preview && generator.running)));engine_play(ui.playing);noct_control(ui.playing?110:111,127);if(!ui.playing && ui.sustain){ui.sustain=0;play(0xB0u,64,0);}}
        break;
    case B_REC:
        if(ui.btn>>B_FX&1u){ui.fx_used=1;panic();}
        else{noct_recording=!noct_recording;noct_control(112,127);}
        break;
    case B_ENV:
        if(ui.btn>>B_ARP&1u){ui.arp_used=1;proj.arp_latch=!proj.arp_latch;arp_latch_changed();ui.dirty=1;say("ARP HOLD",proj.arp_latch?"ON":"OFF");}
        else if(ui.btn>>B_GLO&1u){ui.glo_used=1;proj.touch=!proj.touch;touch_apply();ui.dirty=1;say("VELOCITY",proj.touch?"SENSED":"FIXED 100");}
        else{ui.sustain=1;play(0xB0u,64,127);}
        break;
    case B_LFO:engine_mod(1);break;
    case B_OCTDN:revoice(-1,0);break;
    case B_OCTUP:revoice(1,0);break;
    default:break;
    }
}
static void button_up(int b)
{
    switch(b){
    case B_ARP:if(!ui.arp_used && plat_ms()-ui.arp_t<350u){revoice(0,1);say("ARPEGGIATOR",proj.arp_on?"ON":"OFF");}break;
    case B_FX:if(!ui.fx_used){noct_control(113,127);ui.fx_on=!ui.fx_on;say("DAW FX",ui.fx_on?"ON":"BYPASS");}break;
    case B_ENV:if(ui.sustain){ui.sustain=0;play(0xB0u,64,0);}break;
    case B_LFO:engine_mod(0);break;
    case B_SAVE:if(!ui.save_done)save_now();break;
    case B_SEL:CHORD_ON=!CHORD_ON;revoice(0,0);ui.page=1;say("CHORDS",CHORD_ON?CHORD_NAME[CHORD_TYPE]:"OFF");break;
    case B_HOME:ui.page=0;say("NOCT MIDI","ONE FIXED PRESET");break;
    case B_SEQ:
        if(!ui.seq_used && plat_ms()-ui.seq_t<350u){
            generator_toggle();ui.page=2;
            say("GENERATOR",generator.enabled?"ON":"OFF");
        }break;
    case B_EDIT:music_random(&proj.gen_seed);proj.gen_progression=(uint8_t)((proj.gen_progression+1)%4);generator_regenerate();ui.dirty=1;ui.page=2;say("NEW PHRASE",generator.running?"NEXT BAR":"READY");break;
    case B_GLO:if(!ui.glo_used){ui.page=PAGE_SYNC;say("SYNC SETTINGS",0);}break;
    default:break;
    }
}
/* Every visible page has four direct parameters. HOME always restores DAW CCs. */
static void page_knob(int page,int i,int delta)
{
    int compose=0,voice=0;uint32_t f=plat_lock();
    if(page==PAGE_CHORD){
        if(i==0){CHORD_TYPE=(uint8_t)clamp(CHORD_TYPE+delta,0,5);voice=1;}
        if(i==1)proj.vel=(uint8_t)clamp(proj.vel+delta,1,127);
        if(i==2){proj.touch=(uint8_t)clamp(proj.touch+delta,0,1);}
        if(i==3){plat_unlock(f);revoice(delta,0);return;}
    }else if(page==PAGE_GEN){
        if(i==0)proj.gen_mode=(uint8_t)clamp(proj.gen_mode+delta,0,2);
        if(i==1)GEN_SCALE=(uint8_t)clamp(GEN_SCALE+delta,0,2);
        if(i==2)GEN_ROOT=(uint8_t)clamp(GEN_ROOT+delta,24,96);
        if(i==3)proj.gen_progression=(uint8_t)clamp(proj.gen_progression+delta,0,3);
        compose=1;
    }else if(page==PAGE_FEEL){
        if(i==0)GEN_DENSITY=(uint8_t)clamp(GEN_DENSITY+delta,0,16);
        if(i==1)GEN_GATE=(uint8_t)clamp(GEN_GATE+delta,10,100);
        if(i==2)GEN_OCTAVES=(uint8_t)clamp(GEN_OCTAVES+delta,1,3);
        if(i==3)proj.vel=(uint8_t)clamp(proj.vel+delta,1,127);
        compose=1;
    }else if(page==PAGE_ARP){
        if(i==0)proj.arp_rate=(uint8_t)clamp(proj.arp_rate+delta,0,NRATE-1);
        if(i==1)proj.arp_mode=(uint8_t)clamp(proj.arp_mode+delta,0,NARP-1);
        if(i==2)proj.arp_oct=(uint8_t)clamp(proj.arp_oct+delta,1,3);
        if(i==3){plat_unlock(f);local_tempo(delta*10);return;}
    }else if(page==PAGE_SYNC){
        if(i==0)proj.clock=(uint8_t)clamp(proj.clock+delta,0,NCLK-1);
        if(i==1){plat_unlock(f);local_tempo(delta*10);return;}
        if(i==2)proj.vel=(uint8_t)clamp(proj.vel+delta,1,127);
        if(i==3)proj.touch=(uint8_t)clamp(proj.touch+delta,0,1);
    }
    plat_unlock(f);
    if(voice && CHORD_ON)revoice(0,0);
    if((page==PAGE_CHORD && i==2)||(page==PAGE_SYNC && i==3))touch_apply();
    if(compose)generator_regenerate();
    ui.dirty=1;ui.msg_until=0;
}
static void input(void)
{
    uint32_t btn=plat_buttons(),changed=btn^ui.btn;int i;int32_t e;
    ui.btn=btn;
    for(i=0;i<NB;i++)if(changed>>i&1u){if(btn>>i&1u)button_down(i);else button_up(i);}
    input_keys();midi_in();
    for(i=0;i<NKNOBS;i++)if((e=plat_enc(EN_K1+i))!=0){
        if(btn>>B_ARP&1u)ui.arp_used=1;
        if(btn>>B_SEQ&1u)ui.seq_used=1;
        if(ui.page!=PAGE_DAW)page_knob(ui.page,i,e);
        else{
            int value=clamp(proj.val[0][i]+e,0,127);
            if(value!=proj.val[0][i]){proj.val[0][i]=(uint8_t)value;noct_control(20u+(uint32_t)i,(uint32_t)value);}
        }
        ui.touched=(int8_t)i;ui.touch_until=plat_ms()+600u;
    }
    if((e=plat_enc(EN_SELECT))!=0)local_tempo(e*10);
    if((e=plat_enc(EN_ALGO))!=0){
        if(ui.page==PAGE_DAW)ui.page=CHORD_ON?PAGE_CHORD:PAGE_ARP;
        page_knob(ui.page,ui.page==PAGE_GEN?3:ui.page==PAGE_ARP?1:0,e);
    }
    if((e=plat_enc(EN_PRESET))!=0){ui.page=(uint8_t)clamp(ui.page+e,0,PAGE_COUNT-1);ui.msg_until=0;}
    ui.master_raw+=((int32_t)plat_master()-ui.master_raw)/4;
    i=clamp(ui.master_raw*127/4000,0,127);
    if(i!=ui.master_sent && (i-ui.master_sent>1||ui.master_sent-i>1||i==0||i==127)){
        ui.master_sent=(int16_t)i;noct_control(7,(uint32_t)i);
    }
    save_if_idle();
}
static int lit(int port,int out){return plat_midi_ago(port,out)<120u;}
#include "midi_screen.c"
void ui_init(void)
{
    memset(&ui,0,sizeof ui);ui.touched=-1;ui.master_raw=(int32_t)plat_master();
    ui.master_sent=(int16_t)clamp(ui.master_raw*127/4000,0,127);
    noct_recording=0;noct_transition=plat_ms()+500u;
    touch_on(0);touch_apply();lcd_fill(0,0,240,240,K_BG);
    noct_control(119,2); /* Unified preset handshake; never selects a DAW. */
}
void ui_input_only(void){input();}
void ui_frame(void)
{
    input();noct_draw();
    plat_lights((ui.page==PAGE_DAW?1u<<B_HOME:0u)|(proj.arp_on?1u<<B_ARP:0u)|(ui.playing?LED_PLAY_GREEN:0u)|
        (noct_recording?1u<<B_REC:0u)|(ui.fx_on?1u<<B_FX:0u)|(CHORD_ON?1u<<B_SEL:0u)|
        (generator.running?1u<<B_SEQ:0u),ui.down,ui.btn,0,1);
    ann_tick(plat_ms());ui.frame++;
}
