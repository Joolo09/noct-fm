/* SPDX-License-Identifier: MPL-2.0
 * NOCT MIDI engine. Adapted from Charles Vestal's CTL-1 arpeggiator.
 * No sequencer, sound engine, USB clock output or flash operations.
 */
static const uint8_t RATE_TICKS[NRATE] = {24,12,8,6,4,3};
static struct {
    uint8_t refs[128], order[27], count, note, gate, channel;
    uint32_t phase, ticks, last_clock, gate_ms, random, position;
    uint8_t mod, mod_target, mod_wait;
} arp_engine;

static void eng_send(uint32_t st, uint32_t a, uint32_t b)
{ ctl_send((st >> 4) | st << 8 | a << 16 | b << 24, TO_USB | TO_BLE); }
static void arp_off(void)
{
    if (arp_engine.note != 255u) {
        eng_send(0x80u | arp_engine.channel, arp_engine.note,0);
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
    uint8_t sorted[27], sequence[81];
    int i,j,o,n=0,index=0;
    uint32_t f=plat_lock();
    arp_off();
    if(!proj.arp_on || !arp_engine.count){plat_unlock(f);return;}
    for(i=0;i<arp_engine.count;i++)sorted[i]=arp_engine.order[i];
    if(proj.arp_mode!=ARP_PLAYED)
        for(i=1;i<arp_engine.count;i++)for(j=i;j>0 && sorted[j-1]>sorted[j];j--){
            uint8_t t=sorted[j];sorted[j]=sorted[j-1];sorted[j-1]=t;
        }
    for(o=0;o<proj.arp_oct;o++)for(i=0;i<arp_engine.count;i++)
        if(sorted[i]+12*o<=127)sequence[n++]=(uint8_t)(sorted[i]+12*o);
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
        eng_send(0x90u|proj.chan,sequence[index],proj.vel);
    }
    plat_unlock(f);
}
static void arp_clock(void)
{
    if(arp_engine.gate && !--arp_engine.gate)arp_off();
    if(proj.arp_on && arp_engine.count && arp_engine.ticks%RATE_TICKS[proj.arp_rate]==0u)arp_step();
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
    if(arp_engine.refs[note]<27u)arp_engine.refs[note]++;
    for(i=0;i<arp_engine.count && arp_engine.order[i]!=note;i++);
    if(i==arp_engine.count && arp_engine.count<27u)arp_engine.order[arp_engine.count++]=(uint8_t)note;
    if(first){
        arp_engine.position=0;arp_engine.ticks=0;arp_engine.phase=0;
        /* Internal arp responds immediately; external waits for its first F8. */
        if(proj.clock!=CLK_EXT){arp_step();arp_engine.ticks=1;}
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
void engine_panic(void)
{
    uint32_t f=plat_lock();
    arp_off();
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
    if(proj.clock==CLK_EXT){
        /* Missing external clocks must never leave the last gate hanging. */
        if(arp_engine.note!=255u && plat_ms()-arp_engine.last_clock>250u)arp_off();
        return;
    }
    arp_engine.phase+=proj.tempo*24u;
    if(arp_engine.phase>=60000u){arp_engine.phase-=60000u;arp_clock();}
}
void engine_midi_clock(uint32_t st)
{
    uint32_t f=plat_lock();
    if(proj.clock==CLK_EXT){
        if(st==0xF8u){arp_engine.last_clock=plat_ms();arp_clock();}
        if(st==0xFAu){arp_off();arp_engine.ticks=arp_engine.position=0;}
        if(st==0xFCu)arp_off();
    }
    plat_unlock(f);
}
void engine_play(int on){(void)on;} /* Transport commands belong to the DAW; no local drums. */
void engine_reset_tempo(void){arp_engine.phase=0;}
static int eng_arp_note(void){return arp_engine.note==255u?-1:arp_engine.note;}
static int eng_pool(uint8_t *out){int i;for(i=0;i<arp_engine.count;i++)out[i]=arp_engine.order[i];return arp_engine.count;}
static void midi_engine_init(void)
{memset(&arp_engine,0,sizeof arp_engine);arp_engine.note=255u;arp_engine.random=2463534242u;}
