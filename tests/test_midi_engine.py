"""Exercise the real C app: musical arp order and simultaneous CC/key traffic."""
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
HOST=Path(os.environ.get('NOCT_HOST',ROOT/'firmware/app/build/host'/('noct-usb_host.exe' if os.name=='nt' else 'noct-usb_host')))

def scenario(commands):
    with tempfile.TemporaryDirectory(prefix='noct-midi-') as directory:
        script=Path(directory)/'test.ctl'
        script.write_text('\n'.join(commands)+'\n',encoding='utf8')
        result=subprocess.run([str(HOST),str(script),directory],capture_output=True,text=True,timeout=30)
        if result.returncode:raise AssertionError(result.stdout+'\n'+result.stderr)
        return result.stdout

class MidiEngineTests(unittest.TestCase):
    def test_musical_arp_orders(self):
        # First note sounds immediately; then the complete chord follows the selected pattern.
        for mode,following in [(0,[57,60,53,57]),(1,[57,53,60,57]),
                               (2,[57,60,57,53]),(4,[53,57,60,53])]:
            with self.subTest(mode=mode):
                output=scenario(['wait 800',f'spin ALGO {mode}','tap ARP','out',
                    'key 7 down','key 0 down','key 4 down','wait 520',
                    'key 7 up','key 0 up','key 4 up','expect active 0','expect notes_on 0','out'])
                notes=[int(n,16) for n in re.findall(r'out 90 ([0-9A-F]{2}) 64 -> USB',output)]
                self.assertEqual(notes,[60]+following)

    def test_controllers_do_not_stop_notes_or_trigger_flash(self):
        commands=['wait 800','out']
        for i in range(350):
            key=i%27
            commands += [f'key {key} down',f'spin K{i%4+1} {1 if i%2 else -1}',
                         f'midi USB BF {20+i%4:02X} {i%128:02X}',
                         'spin PRESET 1','spin ALGO 1','spin SELECT 1',f'key {key} up']
            if i%20==0:commands+=['expect active 0','out']
        commands+=['wait 10000','expect saves 0','expect active 0','expect chan 0',
                   'expect bank 0','expect program 0','expect cc0 20','expect cc3 23']
        scenario(commands)

    def test_every_octave_has_valid_balanced_notes(self):
        commands=['wait 800','tap OCT-','tap OCT-','tap OCT-','out']
        for octave in range(7):
            for key in range(27):commands += [f'key {key} down',f'key {key} up']
            commands+=['expect active 0','expect notes_on 0','out']
            if octave<6:commands+=['tap OCT+']
        output=scenario(commands)
        notes=[int(n,16) for n in re.findall(r'out 90 ([0-9A-F]{2}) 64 -> USB',output)]
        self.assertEqual(len(notes),189)
        self.assertEqual(min(notes),17)
        self.assertEqual(max(notes),115)

if __name__=='__main__':unittest.main(verbosity=2)
