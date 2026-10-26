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
static uint8_t ep1tx[64],received[4096];static unsigned count;
static uint32_t sie_rd(int reg){(void)reg;return csr;}
static void sie_wr(int reg,uint32_t value){if(reg==S_TXCSR1)csr=value;}
static void fm1_usb_ep_send(int ep,const uint8_t *bytes,unsigned n){
    assert(ep==1 && n<=64 && n%4==0);memcpy(received+count,bytes,n);count+=n;
}
''' +function(source,'midi_out_event')+'\n'+function(source,'ep1_tx')+r'''
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
    unsigned before=mo_w;usb.config=0;midi_out_event(0x643C9009);assert(mo_w==before);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='noct-usb-') as directory:
            path=Path(directory);code=path/'usb.c';exe=path/('usb.exe' if os.name=='nt' else 'usb')
            code.write_text(f'#define MOQ {size}u\n'+fixture,encoding='utf8')
            cc=os.environ.get('CC','cc')
            compiler=[cc] if os.name=='nt' else shlex.split(cc)
            subprocess.run(compiler+['-std=c11','-O2',str(code),'-o',str(exe)],check=True,capture_output=True)
            subprocess.run([str(exe)],check=True,timeout=10)

if __name__=='__main__':unittest.main(verbosity=2)
