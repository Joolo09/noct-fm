"""NOCT 0.8: composer 2.0, chord tools, tap tempo, arp additions and settings migration (real C app)."""
import re
import unittest
from test_midi_engine import scenario

MAJOR={0,2,4,5,7,9,11}
DORIAN={0,2,3,5,7,9,10}
HARM_MINOR={0,2,3,5,7,8,11}
PHRYGIAN={0,1,3,5,7,8,10}


def note_ons(output):
    return [(int(t),int(n,16),int(v,16)) for t,n,v in
            re.findall(r'(\d+) ms: 90 ([0-9A-F]{2}) ([0-9A-F]{2}) -> USB',output) if int(v,16)]


def clocks(n,wait=18):
    return ['midi USB F8 00 00',f'wait {wait}']*n


def external(mode=None,extra=()):
    """External clock, COMPOSE page settings applied through the visible knobs."""
    commands=['wait 800','tap GLO','spin K1 2','spin PRESET -4']
    if mode is not None:commands+=[f'spin K1 {mode}']
    return commands+list(extra)


class ComposerTests(unittest.TestCase):
    def test_new_scales_keep_melody_and_chords_in_key(self):
        for steps,scale in ((3,DORIAN),(4,HARM_MINOR),(7,PHRYGIAN)):
            for mode in (0,1):
                with self.subTest(scale=steps,mode=mode):
                    output=scenario(external(mode,[f'spin K2 {steps}','tap SEQ'])+clocks(384)+
                                    ['tap SEQ','expect active 0','expect notes_on 0'],midi_log=True)
                    notes=[n for _,n,_ in note_ons(output)]
                    self.assertGreater(len(notes),8)
                    self.assertTrue(all(n%12 in scale for n in notes),notes)

    def test_phrase_ends_on_the_tonic_for_many_seeds(self):
        for edits in range(8):
            with self.subTest(seed=edits):
                output=scenario(external(0,['tap EDIT']*edits+['tap SEQ'])+clocks(384)+
                                ['tap SEQ','expect active 0'],midi_log=True)
                notes=[n for _,n,_ in note_ons(output)]
                self.assertEqual(len(notes),32)
                self.assertEqual(notes[-1]%12,0)

    def test_full_mode_adds_a_low_bass_under_chords_and_melody(self):
        output=scenario(external(4,['tap SEQ'])+clocks(1)+
                        ['expect active 5']+clocks(383)+['tap SEQ','expect active 0','expect notes_on 0'],midi_log=True)
        first=[n for t,n,_ in note_ons(output)][:5]
        self.assertEqual(len(first),5)
        self.assertLess(min(first),48)        # the bass sits an octave below the chords
        self.assertGreater(max(first),70)     # the melody sits above them

    def test_bass_mode_follows_the_rhythm_in_a_low_register(self):
        output=scenario(external(3,['tap SEQ'])+clocks(384)+['tap SEQ','expect active 0'],midi_log=True)
        notes=[n for _,n,_ in note_ons(output)]
        self.assertEqual(len(notes),32)
        self.assertTrue(all(n<60 for n in notes),notes)

    def test_chord_styles_and_sevenths(self):
        # STYLE page: K1 style, K2 colour. Notes per bar for one voice set: HOLD 1, PULSE 4, OFFBEAT 4, BROKEN 8.
        for style,onsets in ((0,1),(1,4),(2,4),(3,8)):
            for color,voices in ((0,3),(1,4)):
                with self.subTest(style=style,color=color):
                    output=scenario(external(1,['spin PRESET 2',f'spin K1 {style}',f'spin K2 {color}',
                                                'tap SEQ'])+clocks(96)+['tap SEQ','expect active 0',
                                                'expect notes_on 0'],midi_log=True)
                    notes=note_ons(output)
                    per_onset=1 if style==3 else voices
                    self.assertEqual(len(notes),onsets*per_onset)
                    self.assertTrue(all(n%12 in MAJOR for _,n,_ in notes))

    def test_swing_delays_off_beat_sixteenths(self):
        straight=scenario(external(0,['spin PRESET 1','spin K1 8','tap SEQ'])+clocks(96)+['tap SEQ'],midi_log=True)
        swung=scenario(external(0,['spin PRESET 1','spin K1 8','spin K3 2','tap SEQ'])+clocks(96)+['tap SEQ'],midi_log=True)
        start=lambda out:note_ons(out)[0][0]
        on_grid=lambda out:sorted({(t-start(out))%(6*20) for t,_,_ in note_ons(out)})
        self.assertEqual(on_grid(straight),[0])            # every sixteenth: 6 clocks of 20 ms
        self.assertEqual(on_grid(swung),[0,2*20])           # odd sixteenths two clocks late

    def test_human_feel_shapes_velocity(self):
        output=scenario(external(0,['spin PRESET 1','spin K4 20','expect human 40','tap SEQ'])+clocks(384)+
                        ['tap SEQ','expect active 0'],midi_log=True)
        velocities=[v for _,_,v in note_ons(output)]
        self.assertEqual(len(velocities),32)
        self.assertGreater(len(set(velocities)),3)
        self.assertTrue(all(1<=v<=127 for v in velocities))

    def test_edit_undo_and_new_harmony(self):
        scenario(['wait 800','tap EDIT','expect history 1','tap EDIT','expect history 2',
                  'press SEQ','tap EDIT','release SEQ','expect history 1','expect generator 0',
                  'press EDIT','wait 700','release EDIT','expect progression >0'])
        first=scenario(['wait 800','tap SEQ','wait 1900','tap SEQ'],midi_log=True)
        undone=scenario(['wait 800','tap EDIT','press SEQ','tap EDIT','release SEQ',
                         'tap SEQ','wait 1900','tap SEQ'],midi_log=True)
        self.assertEqual([n for _,n,_ in note_ons(first)],[n for _,n,_ in note_ons(undone)])


class KeyboardTests(unittest.TestCase):
    def chord(self,commands,key=7):
        output=scenario(['wait 800']+commands+['out',f'key {key} down','wait 120',f'key {key} up',
                         'expect active 0','expect notes_on 0'],midi_log=True)
        return [n for _,n,_ in note_ons(output)]

    def test_in_key_chords_follow_the_compose_key(self):
        self.assertEqual(self.chord(['tap SEL','spin K1 8']),[60,64,67])        # C
        self.assertEqual(self.chord(['tap SEL','spin K1 8'],key=9),[62,65,69])  # Dm
        self.assertEqual(self.chord(['tap SEL','spin K1 9'],key=11),[64,67,71,74])  # Em7
        self.assertEqual(self.chord(['tap SEL','spin K1 8'],key=8),[60,64,67])  # C#: the chord of C below it

    def test_voicings(self):
        self.assertEqual(self.chord(['tap SEL','spin K2 1']),[64,67,72])
        self.assertEqual(self.chord(['tap SEL','spin K2 2']),[67,72,76])
        self.assertEqual(self.chord(['tap SEL','spin K2 3']),[60,67,76])
        self.assertEqual(self.chord(['tap SEL','spin K2 4']),[52,60,67])
        self.assertEqual(self.chord(['tap SEL','spin K2 5']),[48,60,64,67])

    def test_scale_lock_snaps_keys_into_the_key(self):
        output=scenario(['wait 800','spin PRESET 1','spin K4 1','expect key_scale 1','out']+
                        [c for k in range(27) for c in (f'key {k} down',f'key {k} up')]+
                        ['expect active 0','expect notes_on 0'],midi_log=True)
        notes=[n for _,n,_ in note_ons(output)]
        self.assertTrue(all(n%12 in MAJOR for n in notes))
        self.assertEqual(len(notes),27)

    def test_strum_spreads_voices_and_early_release_leaves_nothing_hanging(self):
        output=scenario(['wait 800','tap SEL','spin K3 6','expect strum 6','out','key 7 down',
                         'wait 200','key 7 up','expect active 0'],midi_log=True)
        times=[t for t,_,_ in note_ons(output)]
        self.assertEqual(len(times),3)
        self.assertGreaterEqual(times[1]-times[0],45)
        self.assertGreaterEqual(times[2]-times[1],45)
        output=scenario(['wait 800','tap SEL','spin K3 7','out','key 7 down','wait 30','key 7 up',
                         'expect active 0','wait 300','expect active 0','expect notes_on 0'],midi_log=True)
        self.assertEqual(len(note_ons(output)),1)

    def test_sel_and_keys_learn_a_memory_chord_that_survives_reboot(self):
        output=scenario(['wait 800','press SEL','key 7 down','key 11 down','key 16 down',
                         'key 7 up','key 11 up','key 16 up','release SEL','expect chords 1',
                         'expect chord_type 10','expect custom_n 3','expect custom2 9',
                         'wait 1000','tap SAVE','expect saves 1','reboot','wait 800',
                         'expect chord_type 10','out','key 0 down','expect active 3','key 0 up',
                         'expect active 0'],midi_log=True)
        self.assertEqual([n for _,n,_ in note_ons(output)][-3:],[53,57,62])

    def test_single_key_under_sel_does_not_learn(self):
        scenario(['wait 800','press SEL','key 7 down','key 7 up','release SEL',
                  'expect chords 1','expect chord_type 0','key 7 down','expect active 3','key 7 up'])


class TempoArpAndSettingsTests(unittest.TestCase):
    def test_tap_tempo_on_the_sync_page_reaches_the_daw(self):
        output=scenario(['wait 800','tap GLO','expect page 6','out','tap GLO','wait 420','tap GLO',
                         'wait 420','tap GLO','wait 420','tap GLO','expect bpm10 1200','expect clock 0','out'])
        self.assertIn('out BF 72 09',output)
        self.assertIn('out BF 73 30',output)

    def test_converge_and_pedal_orders_and_gate(self):
        for mode,expected in ((5,[60,67,64,60,67]),(6,[67,60,64,60,67])):
            with self.subTest(mode=mode):
                output=scenario(['wait 800','tap ARP',f'spin K2 {mode}','out','key 7 down','key 11 down',
                                 'key 14 down','wait 1100','key 7 up','key 11 up','key 14 up',
                                 'expect active 0','expect notes_on 0','out'])
                notes=[int(n,16) for n in re.findall(r'out 90 ([0-9A-F]{2}) 64 -> USB',output)]
                self.assertEqual(notes[3:8],expected)
        scenario(['wait 800','tap ARP','spin K4 10','expect arp_gate 100','spin K4 -20','expect arp_gate 10',
                  'key 7 down','wait 400','key 7 up','expect active 0'])

    def test_arp_swing_on_sixteenths(self):
        output=scenario(['wait 800','tap ARP','spin PRESET -2','spin K3 3','expect swing 3','tap HOME',
                         'out','key 7 down','wait 1000','key 7 up','expect active 0'],midi_log=True)
        times=[t for t,_,_ in note_ons(output)]
        gaps=sorted({b-a for a,b in zip(times[2:],times[3:])})
        self.assertGreater(len(gaps),1)                      # long-short pairs instead of an even pulse

    def test_new_settings_persist(self):
        scenario(['wait 800','tap SEL','spin K2 3','spin K3 2','spin K4 1','spin PRESET 1','spin K1 4',
                  'spin K2 5','spin PRESET 1','spin K3 1','spin K4 5','spin PRESET 1','spin K1 2','spin K2 1',
                  'wait 1000','tap SAVE','expect saves 1','reboot','wait 800',
                  'expect voicing 3','expect strum 2','expect key_scale 1','expect gen_mode 4',
                  'expect gen_scale 5','expect swing 1','expect human 10','expect style 2','expect color 1'])

    def test_previous_version_settings_migrate_without_changing_the_sound(self):
        scenario(['wait 800','tap SEL','spin K1 5','spin PRESET 1','spin K1 2','format5','reboot','wait 800',
                  'expect chords 1','expect chord_type 5','expect gen_mode 2','expect arp_gate 50',
                  'expect custom_n 5','expect voicing 0','expect strum 0','expect swing 0','expect human 0',
                  'key 7 down','expect active 4','key 7 up','expect active 0'])


if __name__=='__main__':
    unittest.main(verbosity=2)
