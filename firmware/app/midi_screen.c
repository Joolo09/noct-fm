/* SPDX-License-Identifier: MPL-2.0
 * NOCT MIDI's integer pixel UI. Logos identify the two supported DAWs,
 * not different firmware presets. Rendering never changes the MIDI mapping.
 */
static const char NOCT_FRUIT[18][17]={
 "0000000000033300","0000000003333000","0000000033300000","0000333330000000",
 "0000033300000000","0000111122000000","0001111112200000","0011111111220000",
 "0011111111120000","0011111111110000","0001111111110000","0001111111100000",
 "0000111111100000","0000111111000000","0000011111000000","0000001110000000",
 "0000001100000000","0000001000000000"};
static void noct_logo(int x,int y,int scale,int live)
{
    int i,j;
    if(live)for(i=0;i<4;i++){
        cv_rect(x+i*3*scale,y+2*scale,scale,12*scale,K_MINT);
        cv_rect(x+13*scale,y+(2+i*3)*scale,8*scale,scale,K_MINT);
    }else for(j=0;j<18;j++)for(i=0;i<16;i++){
        char c=NOCT_FRUIT[j][i];
        if(c!='0')cv_rect(x+i*scale,y+j*scale,scale,scale,c=='3'?RGB(151,214,107):c=='2'?RGB(255,211,141):K_HOT);
    }
}
static void noct_sky(uint32_t tick)
{
    int i,x,y;
    for(i=0;i<34;i++)cv_rect((i*83+17)%240,(i*41+11)%72,1+(i%9==0),1,
        ((tick/5+(uint32_t)i)%7==0)?K_TEXT:K_LINE);
    for(x=0;x<240;x+=3){int a=(x+27)%110;y=80-(a<55?a:110-a)/3;cv_rect(x,y,3,95-y,RGB(26,26,51));}
    cv_rect(0,94,240,1,RGB(104,85,146));
    for(i=-4;i<=4;i++)cv_line(120+i*15,95,120+i*56,119,K_LINE);
    for(i=0;i<4;i++){int t=(i*6+(int)(tick%6))%24;cv_rect(0,95+t*t/24,240,1,RGB(33,34,61));}
}
static void noct_center(int x,int y,const felucca_font_t *f,const char *s,uint16_t c)
{cv_text(x-text_w(f,s)/2,y,f,s,c);}
static void noct_draw(void)
{
    uint32_t now=plat_ms();int k,arp_edit=ui.btn>>B_ARP&1u,gen_edit=ui.btn>>B_SEQ&1u;char t[40],*p;
    if(now-ui.sig[0]<64u && ui.frame)return;
    ui.sig[0]=now;
    cv_begin(240,144,K_BG);
    cv_band(0,24);
    cv_text(8,4,&FONT_B,"NOCT",K_TEXT);cv_text(48,6,&FONT_XS,"MIDI",K_HOT);
    cv_text(90,6,&FONT_XS,"CH 1",K_DIM);
    cv_rect(139,9,4,4,lit(PORT_USB,1)||lit(PORT_USB,0)?K_MINT:K_LINE);
    cv_text(148,6,&FONT_XS,"USB",K_DIM);
    cv_text(182,6,&FONT_XS,ui.playing?"PLAY":"IDLE",ui.playing?K_MINT:K_DIM);
    if(noct_recording)cv_rect(227,8,5,5,RGB(255,91,115));
    cv_rect(8,23,224,1,K_LINE);
    cv_band(24,120);noct_sky(now/80u);
    noct_logo(10,9,1,0);cv_text(32,12,&FONT_XS,"FL STUDIO",K_HOT);
    noct_logo(133,9,1,1);cv_text(160,12,&FONT_XS,"LIVE",K_MINT);
    const char *heading=ui.page==2?"GENERATOR":ui.page==3?"SYNC":CHORD_ON?"CHORDS":"CHROMATIC";
    noct_center(120,36,&FONT_M,now<noct_transition?"NOCT MIDI":heading,K_TEXT);
    if(now<ui.msg_until)noct_center(120,65,&FONT_XS,ui.message,K_DIM);
    else {
        if(ui.page==2){
            p=put_s(t,GEN_SCALE==0?"MAJOR":GEN_SCALE==1?"MINOR":"PENTA");
            p=put_s(p," / ");p=put_s(p,GEN_MODE_NAME[proj.gen_mode]);p=put_s(p," / ");put_u(p,GEN_ROOT);
        }else if(ui.page==3)put_s(t,clock_external()?"MIDI CLOCK LOCKED":"DAW BPM / INTERNAL GRID");
        else if(CHORD_ON)put_s(t,CHORD_NAME[CHORD_TYPE]);
        else {
        p=put_s(t,proj.arp_on?"ARP ":"ARP OFF / ");
        if(proj.arp_on){p=put_s(p,ARP_NAME[proj.arp_mode]);p=put_s(p," ");p=put_s(p,RATE_NAME[proj.arp_rate]);}
        else p=put_s(p,"F3-G5");
        if(proj.arp_latch)put_s(p," HOLD");
        }
        noct_center(120,65,&FONT_XS,t,proj.arp_on?K_MINT:K_DIM);
    }
    p=put_u(put_s(t,proj.clock==CLK_AUTO?"AUTO / ":proj.clock==CLK_EXT?"EXT / ":"INT / "),proj.bpm10/10u);
    if(proj.bpm10%10u)put_u(put_s(p,"."),proj.bpm10%10u);
    cv_text(8,102,&FONT_XS,t,K_TEXT);
    p=put_s(t,"OCT ");if(proj.octave<0)p=put_s(p,"-");else p=put_s(p,"+");
    put_u(p,(uint32_t)(proj.octave<0?-proj.octave:proj.octave));
    cv_text(186,102,&FONT_XS,t,K_DIM);cv_blit(0,0);plat_yield();
    cv_begin(240,96,K_BG);
    for(k=0;k<4;k++){
        int x=8+k*58,v=proj.val[0][k],hot=ui.touched==k&&now<ui.touch_until;
        const char *name=0;
        cv_rect(x,2,50,64,K_PANEL);cv_rect(x,2,50,1,hot?K_MINT:K_LINE);
        if(arp_edit){
            static const char *const labels[4]={"RATE","MODE","OCTS","BPM"};name=labels[k];
            if(k==0)put_s(t,RATE_NAME[proj.arp_rate]);
            if(k==1)put_u(t,proj.arp_mode+1u);
            if(k==2)put_u(t,proj.arp_oct);
            if(k==3)put_u(t,proj.bpm10/10u);
        }else if(gen_edit){
            static const char *const labels[4]={"MODE","DENS","RANGE","GATE"};name=(k==1 && proj.gen_mode==1)?"PROG":labels[k];
            if(k==0)put_s(t,proj.gen_mode==0?"MEL":proj.gen_mode==1?"CHORD":"BOTH");
            if(k==1)put_u(t,proj.gen_mode==1?proj.gen_progression+1u:GEN_DENSITY);
            if(k==2)put_u(t,GEN_OCTAVES);
            if(k==3)put_u(t,GEN_GATE);
        }else{put_u(put_s(t,"CC "),20u+(uint32_t)k);name=t;}
        noct_center(x+25,8,&FONT_XS,name,hot?K_MINT:K_DIM);
        if(!arp_edit && !gen_edit)put_u(t,(uint32_t)v);
        noct_center(x+25,25,((arp_edit||gen_edit) && (k==0||(gen_edit&&k==2)))?&FONT_S:&FONT_M,t,K_TEXT);
        if(!arp_edit && !gen_edit){cv_rect(x+6,57,38,3,K_LINE);cv_rect(x+6,57,38*v/127,3,K_HOT);}
    }
    cv_band(70,26);cv_rect(8,2,224,1,K_LINE);
    if(generator.enabled){if(generator.pending)put_s(t,"NEXT BAR");else if(generator.wait_bar)put_s(t,"WAIT BAR");else if(generator.running && clock_external() && now-arp_engine.last_clock>500u)put_s(t,"WAIT CLOCK");else {p=put_u(put_s(t,"BAR "),generator.step/16u+1u);put_u(put_s(p," : "),generator.step%16u+1u);}cv_text(8,9,&FONT_XS,t,generator.running?K_MINT:K_DIM);}
    else cv_text(8,9,&FONT_XS,arp_edit?"HOLD ARP: EDIT":gen_edit?"HOLD SEQ: EDIT":ui.fx_on?"FX ON / ARP":"FX BYPASS / ARP",K_DIM);
    cv_text(132,9,&FONT_XS,"SELECT: BPM",K_DIM);
    cv_blit(0,144);
}
