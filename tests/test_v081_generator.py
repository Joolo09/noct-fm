"""NOCT 0.8.1: ALGORITHM phrase browser, melody characters, AUTO harmony and chord colours (real C app)."""
import re
import unittest
from test_midi_engine import scenario
from test_v08_features import note_ons, clocks, external, MAJOR, DORIAN

MAJOR_TONES={0,2,4,5,7,9,11}


def phrase(extra,mode=0,bars=4):
    output=scenario(external(mode,extra+['tap SEQ'])+clocks(96*bars)+
                    ['tap SEQ','expect active 0','expect notes_on 0','out'],midi_log=True)
    return output,note_ons(output)


class AlgorithmTests(unittest.TestCase):
    def test_algorithm_browses_phrases_and_leaves_page_knobs_alone(self):
        output=scenario(['wait 800','tap SEL','spin K1 2','tap HOME','spin PRESET 5','spin K2 1','tap HOME',
                         'out','spin ALGO 5','expect variation 5','spin ALGO -7','expect variation 998',
                         'expect page 0','expect chord_type 2','expect arp_mode 1','expect chords 1',
                         'expect saves 0','out'])
        self.assertNotRegex(output,r'out BF 1[4-7] ')          # no DAW CC from the phrase dial

    def test_each_variation_is_distinct_and_turning_back_restores_it(self):
        seen=[]
        for v in (0,1,2,3):
            _,notes=phrase([f'spin ALGO {v}'] if v else [])
            seen.append(tuple(n for _,n,_ in notes))
        self.assertEqual(len(set(seen)),4)
        _,back=phrase(['spin ALGO 3','spin ALGO -1','spin ALGO 1','spin ALGO -3'])
        self.assertEqual(tuple(n for _,n,_ in back),seen[0])

    def test_variation_and_character_survive_save_and_reboot(self):
        commands=['spin ALGO 42','spin PRESET 2','spin K4 3','expect melody 3','spin PRESET -2']
        first=phrase(commands)[1]
        rebooted=phrase(commands+['wait 1000','tap SAVE','expect saves 1','reboot','wait 800',
                                  'expect variation 42','expect melody 3','tap GLO','spin K1 2','spin PRESET -4'])[1]
        self.assertEqual([n for _,n,_ in first],[n for _,n,_ in rebooted])


class MelodyCharacterTests(unittest.TestCase):
    def test_every_character_keeps_count_key_and_cadence(self):
        for character in range(1,8):
            for variation in (0,7):
                with self.subTest(character=character,variation=variation):
                    _,notes=phrase(['spin PRESET 2',f'spin K4 {character}','spin PRESET -2']+
                                   ([f'spin ALGO {variation}'] if variation else []))
                    pitches=[n for _,n,_ in notes]
                    self.assertEqual(len(pitches),32)
                    self.assertTrue(all(n%12 in MAJOR_TONES for n in pitches),pitches)
                    self.assertEqual(pitches[-1]%12,0)
                    self.assertGreater(len(set(pitches)),2)

    def test_characters_sound_different(self):
        lines=set()
        for character in range(1,8):
            _,notes=phrase(['spin PRESET 2',f'spin K4 {character}','spin PRESET -2'])
            lines.add(tuple(n for _,n,_ in notes))
        self.assertGreaterEqual(len(lines),6)

    def test_riff_repeats_its_half_bar_cell(self):
        _,notes=phrase(['spin PRESET 2','spin K4 3','spin PRESET -2','spin PRESET 1','spin K1 8','spin PRESET -1'],bars=1)
        start=notes[0][0]
        steps={round((t-start)/120):n for t,n,_ in notes}
        first=[s for s in steps if s<8]
        self.assertTrue(all(s+8 in steps for s in first))


class HarmonyTests(unittest.TestCase):
    def test_auto_harmony_changes_progression_per_variation_and_stays_in_key(self):
        progressions=set()
        for v in range(6):
            _,notes=phrase(['spin K4 30']+([f'spin ALGO {v}'] if v else []),mode=1)
            self.assertTrue(all(n%12 in MAJOR_TONES for _,n,_ in notes))
            progressions.add(tuple(n for _,n,_ in notes))
        self.assertGreaterEqual(len(progressions),4)

    def test_two_chords_in_a_bar(self):
        # Progression 10 (0-based 9): I V / vi IV twice: two HOLD chords per bar.
        _,notes=phrase(['spin K4 9'],mode=1,bars=1)
        onsets=sorted({t for t,_,_ in notes})
        self.assertEqual(len(onsets),2)
        self.assertEqual(len(notes),6)

    def test_colours_add9_sus2_and_mixed(self):
        for colour,sizes in ((2,{4}),(3,{3}),(4,{3,4})):
            with self.subTest(colour=colour):
                _,notes=phrase(['spin PRESET 2',f'spin K2 {colour}','spin PRESET -2'],mode=1)
                self.assertTrue(all(n%12 in MAJOR_TONES for _,n,_ in notes))
                groups={}
                for t,n,_ in notes:groups.setdefault(t,[]).append(n)
                self.assertTrue({len(g) for g in groups.values()}<=sizes,groups)

    def test_new_harmonies_in_other_scales(self):
        for prog in (12,20,23):
            with self.subTest(prog=prog):
                _,notes=phrase(['spin K2 3',f'spin K4 {prog}'],mode=4)
                self.assertTrue(all(n%12 in DORIAN or n<48 for _,n,_ in notes))   # bass approach notes may be chromatic



class StartupTests(unittest.TestCase):
    def test_arp_is_off_after_power_on_but_keeps_its_settings(self):
        scenario(['wait 800','tap ARP','expect arp 1','spin K1 -1','spin K2 2','spin K4 4','press ARP','tap ENV',
                  'release ARP','expect latch 1','tap ARP','tap ARP','expect arp 1','wait 1000','tap SAVE',
                  'expect saves 1','reboot','wait 800','expect arp 0','expect arp_rate 2','expect arp_mode 2',
                  'expect arp_gate 70','expect latch 1','out','key 7 down','expect active 1',
                  'expect_out USB 90 3C 64','key 7 up','expect active 0','tap ARP','expect arp 1'])


if __name__=='__main__':
    unittest.main(verbosity=2)
