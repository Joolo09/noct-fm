"""Chord output and physical LED regressions, running the real C application."""
import re
import unittest
from test_midi_engine import scenario

def note_ons(output):
    return [(int(t),int(n,16)) for t,n in re.findall(r'(\d+) ms: 90 ([0-9A-F]{2}) 64 -> USB',output)]

class ChordLightTests(unittest.TestCase):
    def test_function_buttons_follow_active_sustain_modulation_and_record(self):
        scenario(['wait 800','tap SEL','expect led_buttons 2','press ENV',
            'expect led_buttons 6','release ENV','expect led_buttons 2',
            'press LFO','expect led_buttons 10','release LFO','wait 700',
            'expect led_buttons 2','tap REC','expect led_buttons 3074',
            'midi USB BF 75 00','expect led_buttons 2'])

    def test_generated_chord_messages_pass_through_fl_script_as_notes(self):
        from test_integrations import FLTests
        fl=FLTests('test_notes_and_other_channels_pass');fl.setUp()
        output=scenario(['wait 800','tap ARP','tap SEL','key 7 down',
            'wait 300','key 7 up'],midi_log=True)
        events=re.findall(r'ms: (90|80) ([0-9A-F]{2}) ([0-9A-F]{2}) -> USB',output)
        notes=[]
        for st,n,v in events:
            status,note,velocity=int(st,16),int(n,16),int(v,16)
            self.assertFalse(fl.event(note,velocity,status).handled)
            if status==0x90:notes.append(note)
        self.assertEqual(notes,[60,64,67])
        self.assertEqual(fl.calls,[])

    def test_sel_overrides_arp_even_without_external_clock(self):
        output=scenario(['wait 800','tap GLO','spin K1 2','tap ARP',
            'expect arp 1','tap SEL','expect arp 0','expect chords 1','out',
            'key 7 down','expect active 3','expect led_keys 18560',
            'key 7 up','expect active 0','expect led_keys 0','out'],midi_log=True)
        events=note_ons(output)
        self.assertEqual([note for _,note in events],[60,64,67])
        self.assertEqual(len({time for time,_ in events}),1)

    def test_quality_selection_enables_polyphony_from_a_browsed_page(self):
        scenario(['wait 800','spin PRESET 1','expect chords 0','spin K1 1',
            'expect chords 1','expect arp 0','key 7 down','expect active 3',
            'expect_out USB 90 3F 64','key 7 up','expect led_keys 0'])

    def test_switch_to_arp_disables_keyboard_chords_and_releases_old_voices(self):
        scenario(['wait 800','tap SEL','key 7 down','expect active 3',
            'tap ARP','expect chords 0','expect arp 1','expect pool 1',
            'expect active 1','key 7 up','expect active 0','expect led_keys 0'])

    def test_saved_conflicting_modes_migrate_to_polyphonic_chords(self):
        scenario(['wait 800','legacychord','reboot','wait 800','expect chords 1',
            'expect arp 0','key 7 down','expect active 3','key 7 up','expect active 0'])

    def test_each_chord_led_matches_the_sent_notes_at_every_octave(self):
        intervals=((0,4,7),(0,3,7),(0,2,7),(0,5,7),(0,4,7,11),(0,3,7,10))
        for kind,chord in enumerate(intervals):
            commands=['wait 800','tap SEL',f'spin K1 {kind}','tap OCT-','tap OCT-','tap OCT-']
            for octave in range(7):
                for key in range(27):
                    bright=sum(1<<(key+v) for v in chord if key+v<27)
                    commands += [f'key {key} down',f'expect active {len(chord)}',
                        f'expect led_keys {bright}',f'key {key} up','expect active 0','expect led_keys 0','out']
                if octave<6:commands+=['tap OCT+']
            with self.subTest(kind=kind):scenario(commands)

    def test_sustain_lights_release_with_the_pedal(self):
        scenario(['wait 800','tap SEL','press ENV','key 7 down','key 7 up',
            'expect active 0','expect sounding 3','expect led_keys 18560',
            'release ENV','expect sounding 0','expect led_keys 0'])

    def test_incoming_daw_notes_light_without_midi_echo(self):
        scenario(['wait 800','out','midi USB 90 3C 64','midi USB 90 40 64',
            'midi USB 90 43 64','expect led_keys 18560','expect notes_out 0',
            'midi USB 80 3C 00','midi USB 90 40 00','midi USB 80 43 00',
            'expect led_keys 0','expect sounding 0','expect notes_out 0'])

    def test_incoming_sustain_and_stop_clear_the_leds(self):
        scenario(['wait 800','midi USB B0 40 7F','midi USB 90 3C 64',
            'midi USB 80 3C 00','expect led_keys 128','midi USB B0 40 00',
            'expect led_keys 0','midi USB 90 40 64','midi USB FC 00 00',
            'expect sounding 0','expect led_keys 0'])

    def test_panic_clears_sustained_chords_and_incoming_notes(self):
        scenario(['wait 800','tap SEL','press ENV','key 7 down','key 7 up',
            'midi USB 90 45 64','expect sounding 4','press FX','tap REC','release FX',
            'expect active 0','expect sounding 0','expect led_keys 0','release ENV'])

    def test_out_of_range_notes_are_dim_octave_references(self):
        scenario(['wait 800','tap SEL','key 26 down','expect led_keys 67108864',
            'expect led_dim_keys 576','key 26 up','expect led_keys 0','expect led_dim_keys 0'])

    def test_generator_stop_clears_its_lights_but_keeps_a_held_keyboard_note(self):
        scenario(['wait 800','key 7 down','spin PRESET 2','spin K1 2','tap SEQ',
            'wait 300','midi USB FC 00 00','expect active 1','expect led_keys 128',
            'key 7 up','expect led_keys 0'])

if __name__=='__main__':unittest.main(verbosity=2)
