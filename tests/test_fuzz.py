"""Random panel / keyboard / MIDI-in sessions on the real C app: nothing may be left sounding.

Each seed plays a reproducible session of key presses, buttons (held and tapped), encoder turns, MIDI clock,
transport, Song Position, sustain and incoming notes. Afterwards everything is released and the DAW stops:
no note may still sound, and on every channel each Note On must have had its Note Off.
"""
import random
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_midi_engine import HOST

BUTTONS=['FX','SEL','ENV','LFO','EDIT','GLO','HOME','ARP','SEQ','PLAY','REC','OCT-','OCT+']
ENCODERS=['SELECT','ALGO','PRESET','K1','K2','K3','K4']
MIDI=['midi USB F8 00 00']*6+['midi USB FA 00 00','midi USB FB 00 00','midi USB FC 00 00','midi USB BF 74 7F',
      'midi USB BF 74 00','midi USB F2 10 00','midi USB B0 40 7F','midi USB B0 40 00','midi USB 90 3C 64',
      'midi USB 80 3C 00','midi USB BF 72 09','midi USB BF 73 30']


def session(seed,steps=300,panic=False):
    rnd=random.Random(seed);commands=['wait 800'];held=set();keys=set()
    for _ in range(steps):
        x=rnd.random()
        if x<0.25:
            k=rnd.randrange(27)
            commands.append(f'key {k} {"up" if k in keys else "down"}');keys^={k}
        elif x<0.40:
            b=rnd.choice(BUTTONS)
            if b in held:commands.append(f'release {b}');held.discard(b)
            elif rnd.random()<0.5:commands.append(f'press {b}');held.add(b)
            else:commands.append(f'tap {b}')
        elif x<0.65:commands.append(f'spin {rnd.choice(ENCODERS)} {rnd.choice([-3,-1,1,1,2,5])}')
        elif x<0.80:commands.append(rnd.choice(MIDI))
        else:commands.append(f'wait {rnd.choice([5,20,60,200])}')
    commands+=[f'key {k} up' for k in sorted(keys)]+[f'release {b}' for b in sorted(held)]
    commands+=['wait 100','midi USB FC 00 00','wait 600','midi USB FC 00 00','wait 50']
    if panic:commands+=['press FX','tap REC','release FX']
    commands+=['expect active 0']
    with tempfile.TemporaryDirectory(prefix='noct-fuzz-') as directory:
        script=Path(directory)/'fuzz.ctl';script.write_text('\n'.join(commands)+'\n',encoding='utf8')
        result=subprocess.run([str(HOST),str(script),directory],capture_output=True,text=True,timeout=60)
        log=(Path(directory)/'midi_out.txt').read_text()
    balance={}
    for status,note,velocity in re.findall(r'ms: ([89][0-9A-F]) ([0-9A-F]{2}) ([0-9A-F]{2}) -> USB',log):
        s=int(status,16);key=(s&15,int(note,16))
        balance[key]=balance.get(key,0)+(1 if s>>4==9 and int(velocity,16) else -1)
    return result,{k:v for k,v in balance.items() if v>0}


class FuzzTests(unittest.TestCase):
    def test_random_sessions_leave_nothing_sounding(self):
        for seed in range(200):
            with self.subTest(seed=seed):
                result,hanging=session(seed,panic=seed%2==0)
                self.assertEqual(result.returncode,0,result.stdout[-600:])
                self.assertEqual(hanging,{})


if __name__=='__main__':
    unittest.main(verbosity=2)
