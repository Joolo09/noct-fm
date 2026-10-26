"""Exercise the actual USB TX queue and endpoint batching with a fake SIE."""
import os
import re
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def function(source,name):
    start=source.index('static void '+name+'(')
    if name=='ep1_tx':return source[start:source.index('\n#if FELUCCA_CDC\nstatic void ep3_rx',start)]
    opening=source.index('{',start);depth=1;end=opening+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

class USBChordTests(unittest.TestCase):
    def test_revoicing_burst_survives_tx_queue_endpoint_limits_and_wrap(self):
        source=(ROOT/'upstream/base/bsp/chip/usb.c').read_text(encoding='utf8')
        size=int(re.search(r'#define MOQ (\d+)u',source)[1])
        fixture=r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define FELUCCA_OTA 0
#define S_INDEX 1
#define S_TXCSR1 2
#define RING_PUBLISH() ((void)0)
static struct {uint32_t config,tx_pkts;} usb={1,0};
static uint32_t midi_out_q[MOQ],mo_w,mo_r,csr;
/* Also compile the OTA/SysEx arbitration branch used by the real device. */
#define SXQ 16u
static uint32_t sx_out_q[SXQ],so_w,so_r;
static uint8_t sx_busy;
static uint8_t ep1tx[64],received[4096];static unsigned count;
static uint32_t sie_rd(int reg){(void)reg;return csr;}
static void sie_wr(int reg,uint32_t value){if(reg==S_TXCSR1)csr=value;}
static void fm1_usb_ep_send(int ep,const uint8_t *bytes,unsigned n){
    assert(ep==1 && n<=64 && n%4==0);memcpy(received+count,bytes,n);count+=n;
}
''' +source[source.index('/* Bounded recovery storage:'):source.index('static void midi_out_event(')]+function(source,'midi_out_event')+'\n'+function(source,'ep1_tx')+r'''
int main(void){
    for(unsigned round=0;round<100;round++){
        uint32_t expected[80];unsigned before=mo_w;count=0;
        /* 38 old chord voices off, 38 new voices on, and four controllers. */
        for(unsigned i=0;i<80;i++){
            unsigned status=i<38?0x80:i<76?0x90:0xBF;
            unsigned note=(round+i)%127;
            expected[i]=(status>>4)|(status<<8)|(note<<16)|((status==0x80?0u:100u)<<24);
            midi_out_event(expected[i]);
        }
        assert(mo_w-before==80);ep1_tx();unsigned pending=mo_r;
        ep1_tx();assert(mo_r==pending); /* no overwrite while USB is busy */
        while(mo_r!=mo_w){csr=0;ep1_tx();}
        csr=0;assert(count==320);
        for(unsigned i=0;i<80;i++){
            uint32_t actual=received[4*i]|(uint32_t)received[4*i+1]<<8|
                (uint32_t)received[4*i+2]<<16|(uint32_t)received[4*i+3]<<24;
            assert(actual==expected[i]);
        }
    }
    /* A blocked host must not lose Note Off, velocity-zero Note On, or
     * sustain release, even across all channels/pitches and repeated Offs. */
    count=0;midi_out_event(0x643C9009);ep1_tx();csr=0;
    for(unsigned i=0;i<MOQ;i++)midi_out_event(0x6414B00B);
    unsigned queued=mo_w;
    for(unsigned repeat=0;repeat<3;repeat++)
        for(unsigned ch=0;ch<16;ch++)for(unsigned note=0;note<128;note++)
            midi_out_event(8u|(0x80u|ch)<<8|note<<16);
    midi_out_event(0x003D9009); /* Note On velocity zero also releases. */
    midi_out_event(0x0040B00B); /* sustain off */
    midi_out_event(0x643E9009); /* suppressed until the deferred Offs go out */
    assert(mo_w==queued && midi_release_pending());
    csr=1;unsigned pending_read=mo_r;ep1_tx();assert(mo_r==pending_read);
    while(mo_r!=mo_w || midi_release_pending()){csr=0;ep1_tx();}
    unsigned off[16][128]={{0}},ons=0,pedal=0;
    for(unsigned i=0;i<count;i+=4){
        unsigned st=received[i+1],note=received[i+2];
        if((st&240u)==0x80u)off[st&15u][note]++;
        if((st&240u)==0x90u && received[i+3])ons++;
        if(st==0xB0 && note==64 && !received[i+3])pedal++;
    }
    assert(ons==1 && pedal==1);
    for(unsigned ch=0;ch<16;ch++)for(unsigned note=0;note<128;note++)assert(off[ch][note]==1);
    csr=0;count=0;midi_out_event(0x643C9009);midi_out_event(0x003C8008);ep1_tx();
    assert(count==8 && received[1]==0x90 && received[5]==0x80);
    /* Consumer acknowledgements must not erase a newly posted release in
     * the same word, nor turn duplicate releases into a cancelled bit. */
    midi_release_reset();midi_release_defer(0x003C8008);
    assert(midi_release_next()==0x003C8008);
    midi_release_defer(0x003D8008);midi_release_defer(0x003C8008);
    midi_release_defer(0x003C8008);
    assert(midi_release_next()==0x003C8008);
    assert(midi_release_next()==0x003D8008);
    assert(!midi_release_pending());
#if FELUCCA_OTA
    csr=0;count=0;sx_busy=1;midi_release_defer(0x003C8008);
    ep1_tx();assert(count==0 && midi_release_pending());
    sx_out_q[so_w++%SXQ]=0x0000F705;sx_busy=0;ep1_tx();
    assert(count==8 && received[1]==0xF7 && received[5]==0x80);
    assert(!midi_release_pending());
#endif
    unsigned before=mo_w;usb.config=0;midi_out_event(0x643C9009);assert(mo_w==before);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='noct-usb-') as directory:
            path=Path(directory);code=path/'usb.c';exe=path/('usb.exe' if os.name=='nt' else 'usb')
            cc=os.environ.get('CC','cc')
            compiler=[cc] if os.name=='nt' else shlex.split(cc)
            for ota in (0,1):
                code.write_text(f'#define MOQ {size}u\n'+fixture.replace('received[4096]','received[16384]').replace('#define FELUCCA_OTA 0',f'#define FELUCCA_OTA {ota}'),encoding='utf8')
                subprocess.run(compiler+['-std=c11','-O2',str(code),'-o',str(exe)],check=True,capture_output=True)
                subprocess.run([str(exe)],check=True,timeout=10)

    def test_usb_parser_accepts_valid_song_position_and_rejects_bad_data(self):
        source=(ROOT/'upstream/base/bsp/chip/usb.c').read_text(encoding='utf8')
        fixture=r'''
#include <stdint.h>
#include <assert.h>
#define FELUCCA_OTA 0
static struct {unsigned sx_on;} usb;
static uint32_t packet,events;
static int midi_enqueue(uint32_t p,unsigned source){assert(source==1);packet=p;events++;return 1;}
static void sysex_byte(uint8_t b){(void)b;}
'''+function(source,'midi_in_event')+r'''
int main(void){
    midi_in_event(0x0010F203);assert(events==1 && packet==0x0010F203);
    midi_in_event(0x7F7FF203);assert(events==2 && packet==0x7F7FF203);
    midi_in_event(0x0080F203);midi_in_event(0x8010F203);midi_in_event(0x0010F20F);
    assert(events==2);return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='noct-spp-') as directory:
            path=Path(directory);code=path/'rx.c';exe=path/('rx.exe' if os.name=='nt' else 'rx')
            code.write_text(fixture,encoding='utf8')
            cc=os.environ.get('CC','cc');compiler=[cc] if os.name=='nt' else shlex.split(cc)
            subprocess.run(compiler+['-std=c11','-O2',str(code),'-o',str(exe)],check=True,capture_output=True)
            subprocess.run([str(exe)],check=True,timeout=10)

    def test_trs_song_position_allows_clock_interleaving_but_no_running_status(self):
        source=(ROOT/'upstream/base/device/midi_uart.c').read_text(encoding='utf8')
        fixture=r'''
#include <stdint.h>
#include <assert.h>
static struct {uint32_t msgs,drops;uint8_t st,need,got,d0,sysex;} um;
static uint32_t received[16],events;
static int um_put(uint32_t p){received[events++]=p;return 1;}
'''+source[source.index('static uint32_t um_len('):source.index('static void uart_midi_take(')]+r'''
int main(void){
    um_byte(0x90);um_byte(60);um_byte(100);
    um_byte(0xF2);um_byte(16);um_byte(0xF8);um_byte(0);
    assert(events==3 && received[1]==0x0000F80F && received[2]==0x0010F203);
    um_byte(61);um_byte(100);assert(events==3);
    um_byte(0x90);um_byte(61);um_byte(100);um_byte(62);um_byte(100);
    assert(events==5 && (received[4]>>16&127u)==62);return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='noct-trs-') as directory:
            path=Path(directory);code=path/'trs.c';exe=path/('trs.exe' if os.name=='nt' else 'trs')
            code.write_text(fixture,encoding='utf8')
            cc=os.environ.get('CC','cc');compiler=[cc] if os.name=='nt' else shlex.split(cc)
            subprocess.run(compiler+['-std=c11','-O2',str(code),'-o',str(exe)],check=True,capture_output=True)
            subprocess.run([str(exe)],check=True,timeout=10)

if __name__=='__main__':unittest.main(verbosity=2)
