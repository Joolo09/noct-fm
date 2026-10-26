/* SPDX-License-Identifier: MPL-2.0
 * NOCT MIDI engine. Adapted from Charles Vestal's CTL-1 arpeggiator.
 * No sequencer, sound engine, USB clock output or flash operations.
 */
static const uint8_t RATE_TICKS[NRATE] = {24,12,8,6,4,3};
static struct {
    uint8_t refs[128], order[128], count, note, gate, channel;
    uint32_t phase, ticks, last_clock, gate_ms, random, position;
    uint8_t mod, mod_target, mod_wait;
    uint8_t have_clock, arp_paused, remote_running, transport_known, daw_tempo_valid;
    uint32_t beat_start, clock_pulses;
} arp_engine;
#include "composer.c"
static void engine_tempo(uint32_t value)
{
    proj.bpm10=(uint16_t)clamp((int32_t)value,200,9990);
    proj.tempo=(uint8_t)clamp((proj.bpm10+5)/10,40,240);
}
static int clock_external(void)
{return proj.clock==CLK_EXT || (proj.clock==CLK_AUTO && arp_engine.have_clock && (arp_engine.remote_running || (!generator.preview && plat_ms()-arp_engine.last_clock<500u)));}

static void eng_send(uint32_t st, uint32_t a, uint32_t b)
{ ctl_send((st >> 4) | st << 8 | a << 16 | b << 24, TO_USB | TO_BLE); }
static void arp_off(void)
{
    if (arp_engine.note != 255u) {
        bus_off(arp_engine.note);
        arp_engine.note=255u;
    }
    arp_engine.gate=0;
}
static int arp_held(void)
{
    int i;
    for(i=0;i<128;i++)if(arp_engine.refs[i])return 1;
    return 0;
}
static void arp_step(void)
{
    uint8_t sorted[128], sequence[128];uint32_t seen[4]={0,0,0,0};
    int i,j,o,n=0,index=0;
    uint32_t f=plat_lock();
    arp_off();
    if(!proj.arp_on || !arp_engine.count || arp_engine.arp_paused){plat_unlock(f);return;}
    for(i=0;i<arp_engine.count;i++)sorted[i]=arp_engine.order[i];
    if(proj.arp_mode!=ARP_PLAYED)
        for(i=1;i<arp_engine.count;i++)for(j=i;j>0 && sorted[j-1]>sorted[j];j--){
            uint8_t t=sorted[j];sorted[j]=sorted[j-1];sorted[j-1]=t;
        }
    for(o=0;o<proj.arp_oct;o++)for(i=0;i<arp_engine.count;i++){
        int pitch=sorted[i]+12*o;
        if(pitch<=127 && !(seen[pitch>>5]>>(pitch&31)&1u)){
            sequence[n++]=(uint8_t)pitch;seen[pitch>>5]|=1u<<(pitch&31);
        }
    }
    if(n){
        uint32_t p=arp_engine.position;
        if(proj.arp_mode==ARP_DOWN)index=n-1-(int)(p%(uint32_t)n);
        else if(proj.arp_mode==ARP_UPDOWN){
            int period=n>1?2*n-2:1, at=(int)(p%(uint32_t)period);
            index=at<n?at:period-at;
        }else if(proj.arp_mode==ARP_RANDOM){
            arp_engine.random^=arp_engine.random<<13;
            arp_engine.random^=arp_engine.random>>17;
            arp_engine.random^=arp_engine.random<<5;
            index=(int)(arp_engine.random%(uint32_t)n);
        }else index=(int)(p%(uint32_t)n);
        arp_engine.position=(p+1u)%1000000u;
        arp_engine.note=sequence[index];
        arp_engine.channel=proj.chan;
        arp_engine.gate=(uint8_t)(RATE_TICKS[proj.arp_rate]/2u);
        arp_engine.gate_ms=plat_ms();
        bus_on(sequence[index],proj.vel);
    }
    plat_unlock(f);
}
static void arp_clock(void)
{
    if(arp_engine.gate && !--arp_engine.gate)arp_off();
    if(proj.arp_on && arp_engine.count && arp_engine.ticks%RATE_TICKS[proj.arp_rate]==0u)arp_step();
    generator_clock();
    arp_engine.ticks++;
}
void arp_press(int note)
{
    uint32_t f=plat_lock();int i,first;
    if(note<0||note>127){plat_unlock(f);return;}
    first=!arp_engine.count;
    if(proj.arp_latch && !arp_held()){
        arp_engine.count=0;first=1;
    }
    arp_engine.arp_paused=0;
    if(arp_engine.refs[note]<128u)arp_engine.refs[note]++;
    for(i=0;i<arp_engine.count && arp_engine.order[i]!=note;i++);
    if(i==arp_engine.count && arp_engine.count<128u)arp_engine.order[arp_engine.count++]=(uint8_t)note;
    if(first){
        arp_engine.position=0;
        /* Internal arp responds immediately; external waits for its first F8. */
        if(!clock_external()){arp_step();}
    }
    plat_unlock(f);
}
static void arp_prune(void)
{
    int i,n=0;
    for(i=0;i<arp_engine.count;i++)if(arp_engine.refs[arp_engine.order[i]])
        arp_engine.order[n++]=arp_engine.order[i];
    arp_engine.count=(uint8_t)n;
    if(!n){arp_off();arp_engine.position=0;}
}
void arp_release(int note)
{
    uint32_t f=plat_lock();
    if(note>=0 && note<128 && arp_engine.refs[note])arp_engine.refs[note]--;
    if(!proj.arp_latch && note==arp_engine.note && !arp_engine.refs[note])arp_off();
    if(!proj.arp_latch)arp_prune();
    plat_unlock(f);
}
void arp_latch_changed(void)
{uint32_t f=plat_lock();if(!proj.arp_latch)arp_prune();plat_unlock(f);}
static void arp_clear(void)
{arp_off();memset(arp_engine.refs,0,sizeof arp_engine.refs);arp_engine.count=0;arp_engine.position=0;}
void engine_panic(void)
{
    uint32_t f=plat_lock();
    arp_off();
    generator_off();generator.running=generator.preview=0;
    memset(arp_engine.refs,0,sizeof arp_engine.refs);
    arp_engine.count=0;arp_engine.position=0;
    if(arp_engine.mod)eng_send(0xB0u|proj.chan,1,0);
    arp_engine.mod=arp_engine.mod_target=0;
    plat_unlock(f);
}
void arp_set(int on)
{uint32_t f=plat_lock();engine_panic();proj.arp_on=(uint8_t)!!on;plat_unlock(f);}
void engine_mod(int up){arp_engine.mod_target=(uint8_t)(up?127:0);}
void engine_tick(void)
{
    if(arp_engine.mod_wait)arp_engine.mod_wait--;
    if(!arp_engine.mod_wait && arp_engine.mod!=arp_engine.mod_target){
        if(arp_engine.mod<arp_engine.mod_target)arp_engine.mod++;
        else arp_engine.mod--;
        eng_send(0xB0u|proj.chan,1,arp_engine.mod);arp_engine.mod_wait=5;
    }
    if(clock_external()){
        /* Missing external clocks must never leave the last gate hanging. */
        if(plat_ms()-arp_engine.last_clock>500u){arp_off();generator_off();generator.resume_chord=1;}
        return;
    }
    arp_engine.phase+=proj.bpm10*24u;
    if(arp_engine.phase>=600000u){arp_engine.phase-=600000u;arp_clock();}
}
static void engine_transport(int on,int restart)
{
    uint32_t f=plat_lock();
    arp_engine.transport_known=1;arp_engine.remote_running=(uint8_t)!!on;
    generator.preview=0;generator.wait_bar=0;generator.resume_chord=(uint8_t)(on && !restart);
    generator.running=(uint8_t)(on && generator.enabled);
    if(!on){arp_off();generator_off();arp_engine.arp_paused=1;}
    else {
        arp_engine.arp_paused=0;
        if(restart){arp_off();generator_off();arp_engine.ticks=arp_engine.position=arp_engine.phase=0;generator.step=0;generator.cursor=0;}
    }
    plat_unlock(f);
}
/* A DAW snapshot is not a Start/Stop command. In particular, stopped
 * snapshots must not cancel local preview, and playing snapshots must not
 * turn Continue into Start depending on callback/clock ordering. */
static void engine_feedback(int on)
{
    uint32_t f=plat_lock();
    if(arp_engine.remote_running!=(uint8_t)!!on)engine_transport(on,0);
    arp_engine.transport_known=1;
    plat_unlock(f);
}
static void engine_song_position(uint32_t position)
{
    uint32_t f=plat_lock(),ticks=(position&16383u)*6u;
    arp_off();generator_off();
    arp_engine.ticks=ticks;arp_engine.phase=0;
    arp_engine.position=ticks/RATE_TICKS[proj.arp_rate];
    generator.cursor=(uint16_t)(ticks%384u);
    generator.step=(uint8_t)(generator.cursor/6u);
    generator.wait_bar=0;generator.resume_chord=generator.running;
    plat_unlock(f);
}
void engine_midi_clock(uint32_t st)
{
    uint32_t f=plat_lock();
    if(st==0xF8u && proj.clock!=CLK_INT){
        uint32_t now=plat_ms();
        if(!arp_engine.have_clock || now-arp_engine.last_clock>500u){arp_engine.clock_pulses=0;arp_engine.beat_start=now;}
        arp_engine.have_clock=1;arp_engine.last_clock=now;
        if(++arp_engine.clock_pulses>=25u){
            uint32_t delta=now-arp_engine.beat_start;
            if(!arp_engine.daw_tempo_valid && delta>=60u && delta<=3000u)engine_tempo(600000u/delta);
            arp_engine.beat_start=now;arp_engine.clock_pulses=1;
        }
        if(!generator.preview || proj.clock==CLK_EXT){arp_engine.phase=0;arp_clock();}
    }
    if(st==0xFAu)engine_transport(1,1);
    if(st==0xFBu)engine_transport(1,0);
    if(st==0xFCu)engine_transport(0,0);
    plat_unlock(f);
}
void engine_play(int on){engine_transport(on,0);}
void engine_reset_tempo(void){arp_engine.phase=0;}
static int eng_arp_note(void){return arp_engine.note==255u?-1:arp_engine.note;}
static int eng_pool(uint8_t *out){int i;for(i=0;i<arp_engine.count;i++)out[i]=arp_engine.order[i];return arp_engine.count;}
static void midi_engine_init(void)
{
    bus_sustain(0);
    visual_note_serial=0;visual_last_note=0;
    memset(&arp_engine,0,sizeof arp_engine);arp_engine.note=255u;arp_engine.random=2463534242u;
    memset(&generator,0,sizeof generator);generator.melody_note=255;generator_regenerate();
    for(int i=0;i<128;i++)note_owners[i]=0;
}
