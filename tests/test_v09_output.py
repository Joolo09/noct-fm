"""NOCT 0.9: split generator channels, the velocity/touch knob and scale lights (real C app)."""
import re
import unittest
from test_midi_engine import scenario
from test_v08_features import clocks, external


def events(output):
    return [(int(t),int(s,16),int(n,16),int(v,16)) for t,s,n,v in
            re.findall(r'(\d+) ms: ([89][0-9A-F]) ([0-9A-F]{2}) ([0-9A-F]{2}) -> USB',output)]


def sync_page():
    return ['tap HOME','tap GLO']


class SplitTests(unittest.TestCase):
    def full(self,split):
        commands=external(4,[])+sync_page()+([f'spin K4 1','expect gen_split 1'] if split else [])
        commands+=['spin PRESET -4','tap SEQ']+clocks(384)+['tap SEQ','expect active 0','expect notes_on 0','out']
        return events(scenario(commands,midi_log=True))

    def test_split_sends_melody_chords_and_bass_on_channels_1_2_3(self):
        together,split=self.full(False),self.full(True)
        self.assertEqual({s for _,s,_,_ in together},{0x90,0x80})
        ons={s&15:set() for _,s,_,v in split if s>>4==9 and v}
        for _,s,n,v in split:
            if s>>4==9 and v:ons[s&15].add(n)
        self.assertEqual(set(ons),{0,1,2})
        self.assertLess(max(ons[2]),min(ons[0]))            # bass below melody
        # Same pitches either way: only the channels differ (on one channel equal pitches merge).
        self.assertEqual({n for _,s,n,v in together if s==0x90 and v},
                         {n for _,s,n,v in split if s>>4==9 and v})
        # Every channel balanced.
        for ch in (0,1,2):
            on=sum(1 for _,s,_,v in split if s==0x90|ch and v)
            off=sum(1 for _,s,_,v in split if s==0x80|ch or (s==0x90|ch and not v))
            self.assertEqual(on,off)

    def test_switching_split_while_playing_leaves_no_hanging_notes(self):
        output=scenario(external(4,[])+['spin PRESET -4','tap SEQ']+clocks(30)+sync_page()+
                        ['spin K4 1']+clocks(60)+['spin K4 -1']+clocks(60)+
                        ['tap SEQ','expect active 0','expect notes_on 0','out'],midi_log=True)
        for ch in (0,1,2):
            on=sum(1 for _,s,_,v in events(output) if s==0x90|ch and v)
            off=sum(1 for _,s,_,v in events(output) if s==0x80|ch or (s==0x90|ch and not v))
            self.assertEqual(on,off,ch)

    def test_keyboard_stays_on_channel_1_and_panic_clears_split_channels(self):
        output=scenario(['wait 800']+sync_page()+['spin K4 1','tap HOME','out','key 7 down','expect_out USB 90 3C 64',
                         'key 7 up','press FX','tap REC','release FX','expect_out USB B1 7B 00','expect_out USB B2 7B 00'])
        self.assertIn('out 90 3C 64',output)


class KnobTests(unittest.TestCase):
    def test_velocity_knob_reaches_sensed_and_back(self):
        scenario(['wait 800']+sync_page()+['spin K3 -200','expect touch_on 1','expect velocity 1',
                  'spin K3 1','expect touch_on 0','expect velocity 1','spin K3 99','expect velocity 100'])

    def test_scale_lock_lights_the_scale(self):
        # C major from F3: F G A B C D E F G A B C D E F G = keys 0,2,4,6,7,9,11,...
        scenario(['wait 800','tap SEL','spin K4 1','tap SEL','expect led_dim_keys 95247061'])


if __name__=='__main__':
    unittest.main(verbosity=2)
