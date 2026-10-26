/* SPDX-License-Identifier: MPL-2.0
 * Physical panel feedback from the same note state used by the MIDI engine.
 * FM-1 key LEDs have two brightness levels, not individually addressable RGB.
 */
static void lighting_update(void)
{
    uint32_t now=plat_ms(),bright=0,dim=ui.down,buttons=ui.btn;
    int base=53+12*proj.octave;
    for(int note=0;note<128;note++)if(ui_sounding(note)){
        int key=note-base;
        if(key>=0 && key<NKEYS)bright|=1u<<key;
        else{key%=12;if(key<0)key+=12;dim|=1u<<key;}
    }
    if(ui.btn>>B_SEQ&1u){
        int key=((int)GEN_ROOT-53)%12;if(key<0)key+=12;dim|=1u<<key;
    }
    if(CHORD_ON)buttons|=1u<<B_SEL;
    if(proj.arp_on)buttons|=1u<<B_ARP;
    if(generator.running)buttons|=1u<<B_SEQ;
    if(ui.playing || generator.running)buttons|=LED_PLAY_GREEN;
    if(noct_recording)buttons|=(1u<<B_REC)|(1u<<B_PLAY);
    if(ui.fx_on)buttons|=1u<<B_FX;
    if(ui.sustain)buttons|=1u<<B_ENV;
    if(arp_engine.mod || arp_engine.mod_target)buttons|=1u<<B_LFO;
    if(proj.octave>0)buttons|=1u<<B_OCTUP;
    if(proj.octave<0)buttons|=1u<<B_OCTDN;
    if(ui.saved_until && now<ui.saved_until)buttons|=1u<<B_SAVE;
    static const uint8_t page_button[PAGE_COUNT]={B_HOME,B_SEL,B_SEQ,B_SEQ,B_ARP,B_GLO};
    unsigned selected=page_button[ui.page];
    if(ui.page==PAGE_DAW || now%1400u<180u)buttons|=1u<<selected;
    /* All available controls remain dim; steady bright means active or held. */
    uint32_t dim_buttons=((1u<<NB)-1u)&~buttons;
    dim_buttons&=~(1u<<B_PLAY);
    if(!(buttons&(LED_PLAY_GREEN|(1u<<B_PLAY))))dim_buttons|=LED_PLAY_GREEN;
    dim&=~bright;
    if(buttons!=ui.led_buttons || bright!=ui.led_keys || dim_buttons!=ui.led_dim_buttons || dim!=ui.led_dim_keys){
        ui.led_buttons=buttons;ui.led_keys=bright;
        ui.led_dim_buttons=dim_buttons;ui.led_dim_keys=dim;
        plat_lights(buttons,bright,dim_buttons,dim,1);
    }
}
