"""Reproduce the MIDI/DAW failures from the 0.7.0 review on the real C app."""
import re
import unittest
from test_midi_engine import scenario


def notes(output):
    return [int(n,16) for n in re.findall(r'out 90 ([0-9A-F]{2}) [0-9A-F]{2} -> USB',output)]


class ReviewRegressionTests(unittest.TestCase):
    def test_feedback_and_continue_order_preserves_position_and_chords(self):
        for resume in (['midi USB FB 00 00','midi USB BF 74 7F'],
                       ['midi USB BF 74 7F','midi USB FB 00 00']):
            with self.subTest(resume=resume):
                scenario(['wait 800','tap GLO','spin K1 2','press SEQ','spin K1 1',
                          'release SEQ','tap SEQ','midi USB FA 00 00']+
                         ['midi USB F8 00 00']*24+
                         ['expect gen_cursor 24','midi USB FC 00 00','expect active 0']+
                         resume+['midi USB F8 00 00','expect gen_cursor 25',
                                 'expect active 3','midi USB FC 00 00','expect active 0'])

    def test_stopped_snapshots_keep_local_preview_and_sustain(self):
        scenario(['wait 800','midi USB BF 74 00','tap SEQ','wait 100','press ENV',
                  'key 0 down','midi USB BF 74 00','midi USB BF 74 00',
                  'expect gen_running 1','expect sustain 1','expect playing 0',
                  'midi USB FC 00 00','expect gen_running 0','expect sustain 0',
                  'expect active 1','key 0 up','release ENV','expect active 0'])

    def test_repeated_stopped_snapshot_does_not_release_keyboard_pedal(self):
        scenario(['wait 800','midi USB BF 74 00','press ENV','key 7 down','key 7 up',
                  'midi USB BF 74 00','expect sustain 1','expect sounding 1',
                  'release ENV','expect sounding 0'])

    def test_song_position_restores_the_right_bar_and_explicit_start_resets(self):
        setup=['wait 800','tap GLO','spin K1 2','press SEQ','spin K1 1',
               'release SEQ','tap SEQ','midi USB FA 00 00']
        normal=scenario(setup+['midi USB F8 00 00']*96+['out','midi USB F8 00 00','out'])
        sought=scenario(setup+['midi USB FC 00 00','midi USB F2 10 00',
                              'expect gen_cursor 96','out','midi USB FB 00 00',
                              'midi USB F8 00 00','expect gen_cursor 97','expect active 3','out'])
        self.assertEqual(notes(normal)[-3:],notes(sought))
        scenario(setup+['midi USB FC 00 00','midi USB F2 7F 7F',
                        'expect gen_cursor 378','midi USB FB 00 00','midi USB F8 00 00',
                        'expect gen_cursor 379','midi USB FA 00 00','expect gen_cursor 0'])

    def test_all_generator_modes_transpose_each_octave(self):
        for mode in range(3):
            patterns=[]
            for octave in range(4):
                patterns.append(notes(scenario(['wait 800']+['tap OCT+']*octave+
                    ['spin PRESET 2',f'spin K1 {mode}','tap SEQ','wait 2000','tap SEQ','out'])))
            for octave in range(1,4):
                with self.subTest(mode=mode,octave=octave):
                    self.assertEqual(patterns[octave],[n+12*octave for n in patterns[0]])

    def test_extreme_generator_roots_keep_three_distinct_valid_chord_notes(self):
        for root_delta in (-60,55):
            for octave in (-3,3):
                commands=['wait 800','spin PRESET 2','spin K1 1',f'spin K3 {root_delta}']
                commands+=['tap OCT+' if octave>0 else 'tap OCT-']*abs(octave)
                output=scenario(commands+['tap GLO','spin K1 2','tap SEQ','out',
                    'midi USB F8 00 00','expect active 3','tap SEQ','expect active 0','out'])
                chord=notes(output)
                self.assertEqual(len(set(chord)),3)
                self.assertTrue(all(0<=n<=127 for n in chord))

    def test_home_switches_mappable_knobs_and_keeps_mixer_feedback_separate(self):
        scenario(['wait 800','press HOME','wait 650','release HOME','expect knob_map 1',
                  'out','spin K1 1','spin K2 1','spin K3 1','spin K4 1',
                  'expect_out USB BF 18 41','expect_out USB BF 19 41',
                  'expect_out USB BF 1A 41','expect_out USB BF 1B 41',
                  'midi USB BF 14 12','expect val0 18','expect mapval0 65',
                  'tap SEQ','tap HOME','expect knob_map 1','expect gen_running 1',
                  'tap SEQ','wait 1000','tap SAVE','expect saves 1','reboot','wait 800',
                  'expect knob_map 1','expect mapval0 65','press HOME','wait 650',
                  'release HOME','expect knob_map 0','out','spin K1 1',
                  'expect_out USB BF 14 13'])


if __name__=='__main__':
    unittest.main(verbosity=2)
