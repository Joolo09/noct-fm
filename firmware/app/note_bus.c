/* SPDX-License-Identifier: MPL-2.0
 * Shared note ownership: keyboard, chords, arp and generator.
 */
#define MAX_VOICING 4
#define CHORD_ON (proj.rsv[1])
#define CHORD_TYPE (proj.rsv[2])
#define GEN_SCALE (proj.rsv[4])
#define GEN_DENSITY (proj.rsv[5])
#define GEN_ROOT (proj.rsv[6])
#define GEN_OCTAVES (proj.rsv[7])
#define GEN_GATE (proj.rsv[8])
#define CLK_AUTO CLK_INT_OUT
static const char *const CHORD_NAME[6]={"MAJOR","MINOR","SUS2","SUS4","MAJ7","MIN7"};
static volatile uint16_t note_owners[128];
/* Read-only visual telemetry. No drawing or extra MIDI work in the timer. */
static volatile uint32_t visual_note_serial;
static volatile uint8_t visual_last_note;
static void bus_on(int note,uint32_t velocity)
{
    if(note<0 || note>127)return;
    uint32_t f=plat_lock();
    if(!note_owners[note]++){
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
    if(!--note_owners[note])ctl_send(0x8u|0x80u<<8|(uint32_t)note<<16,TO_USB|TO_BLE);
    plat_unlock(f);
}
static int note_voicing(int root,uint8_t *notes)
{
    static const uint8_t intervals[6][4]={{0,4,7,0},{0,3,7,0},{0,2,7,0},{0,5,7,0},{0,4,7,11},{0,3,7,10}};
    int i,n=0,count=CHORD_ON?(CHORD_TYPE>=4?4:3):1;
    for(i=0;i<count;i++){
        int pitch=root+(CHORD_ON?intervals[CHORD_TYPE][i]:0);
        if(pitch>=0 && pitch<128)notes[n++]=(uint8_t)pitch;
    }
    return n;
}
