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
#include "control_screen.c"
