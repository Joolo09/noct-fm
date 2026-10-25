/* SPDX-License-Identifier: MPL-2.0
 * NOCT 0.2: one MIDI preset, invariant keyboard routing, no background flash writes.
 * Uses CTL-1/fm1-base's key sensing and pixel renderer under MPL-2.0.
 */
#define K_BG RGB(9,12,25)
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
    uint8_t key_note[NKEYS],key_vel[NKEYS],key_arp[NKEYS],note_refs[128];
    uint8_t playing,sustain,dirty,arp_used,fx_used,save_done;
    int8_t touched;
    int32_t master_raw;
    int16_t master_sent;
    char last[40],message[40];
    uint32_t saves;
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
    char *p=put_s(ui.message,a);if(b){p=put_s(p," ");put_s(p,b);}
    ui.msg_until=plat_ms()+1500u;
    if(ann_on())ann_say(ui.message,0,ANN_NOW);
}
void ui_say(const char *a,const char *b){say(a,b);}
int ui_dirty(void){return ui.dirty;}
static void key_up(int k)
{
    uint8_t n;
    if(!(ui.down>>k&1u))return;
    n=ui.key_note[k];
    if(ui.key_arp[k])arp_release(n);
    else if(ui.note_refs[n] && !--ui.note_refs[n])play(0x80u,n,0);
    ui.down&=~(1u<<k);
}
static void key_down(int k,uint32_t vel)
{
    int n=53+k+12*proj.octave;
    if(k<0||k>=NKEYS||n<0||n>127)return;
    key_up(k);
    ui.down|=1u<<k;ui.key_note[k]=(uint8_t)n;ui.key_vel[k]=(uint8_t)vel;
    ui.key_arp[k]=proj.arp_on;
    if(proj.arp_on)arp_press(n);
    else if(!ui.note_refs[n]++)play(0x90u,(uint32_t)n,vel);
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
    all_keys_up();engine_panic();
    proj.octave=(int8_t)clamp(proj.octave+octave_delta,-3,3);
    if(arp_toggle)proj.arp_on=(uint8_t)!proj.arp_on;
    for(i=0;i<NKEYS;i++)if(held>>i&1u)key_down(i,velocity[i]);
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
        if(st>=0xF8u){engine_midi_clock(st);continue;}
        if(st==0xBFu){
            if(n>=20u&&n<=23u)proj.val[0][n-20u]=(uint8_t)v;
            if(n==116u)ui.playing=(uint8_t)!!v;
            if(n==117u)noct_recording=(uint8_t)!!v;
            continue;
        }
        if((st&15u)!=proj.chan)continue;
        if((st&0xF0u)==0x90u&&v)ui.heard[n>>5]|=1u<<(n&31u);
        if((st&0xF0u)==0x80u||((st&0xF0u)==0x90u&&!v))ui.heard[n>>5]&=~(1u<<(n&31u));
        if((st&0xF0u)==0xB0u&&(n==120u||n==123u))memset(ui.heard,0,sizeof ui.heard);
    }
}
static void save_if_idle(void)
{
    if(ui.save_done||!(ui.btn>>B_SAVE&1u)||plat_ms()-ui.save_t<1000u)return;
    ui.save_done=1;
    if(ui.down||ui.playing||arp_engine.count||ui.sustain){say("RELEASE KEYS", "STOP BEFORE SAVE");return;}
    if(project_save()==0){ui.dirty=0;ui.saves++;say("SETTINGS SAVED",0);}
    else say("FLASH SAVE FAILED",0);
}
static void button_down(int b)
{
    switch(b){
    case B_ARP:ui.arp_used=0;break;
    case B_FX:ui.fx_used=0;break;
    case B_SAVE:ui.save_t=plat_ms();ui.save_done=0;break;
    case B_PLAY:
        if(ui.btn>>B_GLO&1u){
            uint32_t f=plat_lock();engine_panic();proj.clock=proj.clock==CLK_EXT?CLK_INT:CLK_EXT;plat_unlock(f);
            /* Re-register held arp keys on the new clock without losing releases. */
            revoice(0,0);say("ARP CLOCK",proj.clock==CLK_EXT?"EXTERNAL":"INTERNAL");ui.dirty=1;
        }else{ui.playing=!ui.playing;noct_control(ui.playing?110:111,127);}
        break;
    case B_REC:
        if(ui.btn>>B_FX&1u){ui.fx_used=1;panic();}
        else{noct_recording=!noct_recording;noct_control(112,127);}
        break;
    case B_ENV:
        if(ui.btn>>B_GLO&1u){proj.touch=!proj.touch;touch_apply();ui.dirty=1;say("VELOCITY",proj.touch?"SENSED":"FIXED 100");}
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
    case B_ARP:if(!ui.arp_used){revoice(0,1);say("ARPEGGIATOR",proj.arp_on?"ON":"OFF");}break;
    case B_FX:if(!ui.fx_used){proj.arp_latch=!proj.arp_latch;arp_latch_changed();ui.dirty=1;say("ARP LATCH",proj.arp_latch?"ON":"OFF");}break;
    case B_ENV:if(ui.sustain){ui.sustain=0;play(0xB0u,64,0);}break;
    case B_LFO:engine_mod(0);break;
    case B_SAVE:if(!ui.save_done)say("HOLD SAVE 1 SEC","WHEN STOPPED");break;
    case B_SEL:case B_HOME:case B_SEQ:case B_EDIT:case B_GLO:say("MIDI / CHROMATIC","ONE FIXED PRESET");break;
    default:break;
    }
}
static void input(void)
{
    uint32_t btn=plat_buttons(),changed=btn^ui.btn;int i;int32_t e;
    input_keys();ui.btn=btn;
    for(i=0;i<NB;i++)if(changed>>i&1u){if(btn>>i&1u)button_down(i);else button_up(i);}
    midi_in();
    for(i=0;i<NKNOBS;i++)if((e=plat_enc(EN_K1+i))!=0){
        if(btn>>B_ARP&1u){
            uint32_t f=plat_lock();ui.arp_used=1;
            if(i==0)proj.arp_rate=(uint8_t)clamp(proj.arp_rate+e,0,NRATE-1);
            if(i==1)proj.arp_mode=(uint8_t)clamp(proj.arp_mode+e,0,NARP-1);
            if(i==2)proj.arp_oct=(uint8_t)clamp(proj.arp_oct+e,1,3);
            if(i==3)proj.tempo=(uint8_t)clamp(proj.tempo+e,40,240);
            plat_unlock(f);ui.dirty=1;say("ARP SETTINGS",0);
        }else{
            int value=clamp(proj.val[0][i]+e,0,127);
            if(value!=proj.val[0][i]){proj.val[0][i]=(uint8_t)value;noct_control(20u+(uint32_t)i,(uint32_t)value);}
        }
        ui.touched=(int8_t)i;ui.touch_until=plat_ms()+1000u;
    }
    if((e=plat_enc(EN_SELECT))!=0){uint32_t f=plat_lock();proj.tempo=(uint8_t)clamp(proj.tempo+e,40,240);plat_unlock(f);ui.dirty=1;}
    if((e=plat_enc(EN_ALGO))!=0){uint32_t f=plat_lock();proj.arp_mode=(uint8_t)clamp(proj.arp_mode+e,0,NARP-1);plat_unlock(f);ui.dirty=1;}
    (void)plat_enc(EN_PRESET); /* One preset: no channel, program or routing changes. */
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
    plat_lights((1u<<B_HOME)|(proj.arp_on?1u<<B_ARP:0u)|(ui.playing?LED_PLAY_GREEN:0u)|
        (noct_recording?1u<<B_REC:0u)|(proj.arp_latch?1u<<B_FX:0u),ui.down,0,0,1);
    ann_tick(plat_ms());ui.frame++;
}
