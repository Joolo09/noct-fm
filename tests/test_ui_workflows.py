"""Regression scenarios found by operating the interactive C emulator."""
import re
import unittest
from test_midi_engine import scenario

class WorkflowTests(unittest.TestCase):
    def test_visible_generator_knobs_edit_without_hold_or_cc(self):
        output=scenario(['wait 800','tap SEQ','expect page 2','out',
            'spin K1 1','expect gen_mode 1','spin K2 1','expect gen_scale 1',
            'spin K3 2','expect root_note 62','spin K4 1','wait 2200',
            'tap SEQ','expect active 0','out'])
        self.assertNotRegex(output,r'out BF 1[4-7] ')

    def test_home_restores_cc_while_generator_keeps_playing(self):
        output=scenario(['wait 800','tap SEQ','tap HOME','expect page 0','out',
            'spin K1 1','expect val0 65','spin K2 -1','expect val1 63',
            'expect gen_running 1','expect_out USB BF 14 41','expect_out USB BF 15 3F',
            'tap SEQ','expect active 0','out'])
        self.assertIn('out BF 14 41',output)

    def test_play_stops_local_preview_instead_of_starting_daw(self):
        output=scenario(['wait 800','tap SEQ','wait 400','out','tap PLAY',
            'expect playing 0','expect gen_running 0','expect active 0',
            'expect_out USB BF 6F 7F','out','wait 300','expect notes_out 0'])
        self.assertNotIn('out BF 6E 7F',output)

    def test_opening_sync_does_not_change_clock(self):
        scenario(['wait 800','tap GLO','expect page 5','expect clock 0',
                  'tap GLO','expect clock 0','spin K1 2','expect clock 2',
                  'spin K1 -2','expect clock 0'])

    def test_long_seq_hold_does_not_toggle_when_released(self):
        scenario(['wait 800','press SEQ','wait 700','release SEQ',
                  'expect generator 0','expect page 2','spin K1 1','expect gen_mode 1',
                  'tap SEQ','expect generator 1','press SEQ','wait 700','release SEQ',
                  'expect generator 1','tap SEQ','expect active 0'])

    def test_rhythm_page_and_navigation_keep_keyboard_routing(self):
        commands=['wait 800','spin PRESET 2','expect page 2','spin PRESET 1',
                  'expect page 3','spin K1 -3','expect density 5','spin K2 10','expect gate 95']
        for page in range(6):
            commands+=['tap HOME',f'spin PRESET {page}','out','key 7 down',
                       'expect_out USB 90 3C 64','key 7 up','expect active 0']
        scenario(commands)

    def test_octave_change_during_root_modifier_does_not_lose_held_key(self):
        scenario(['wait 800','key 7 down','expect active 1','press SEQ',
                  'tap OCT+','expect active 1','key 7 up','expect active 0',
                  'wait 400','release SEQ','expect generator 0'])

    def test_emulated_daw_clock_controls_the_real_engine(self):
        scenario(['wait 800','tap GLO','spin K1 2','tap SEQ',
                  'wait 300','expect active 0','simclock 1','midi USB FA 00 00',
                  'wait 700','expect gen_running 1','expect gen_cursor >20',
                  'simclock 0','wait 600','expect active 0',
                  'midi USB FC 00 00','expect gen_running 0'])

    def test_seq_resumes_paused_generator_with_one_press(self):
        scenario(['wait 800','tap SEQ','wait 300','midi USB FC 00 00',
                  'expect generator 1','expect gen_running 0','tap SEQ',
                  'expect generator 1','expect gen_running 1','expect playing 0',
                  'wait 300','tap SEQ','expect generator 0','expect active 0'])

if __name__=='__main__':unittest.main(verbosity=2)
