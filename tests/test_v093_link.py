"""NOCT 0.9.3: Bluetooth + USB, and generic MIDI for apps such as Koala Sampler (real C app).

Run against the Bluetooth build with NOCT_HOST=firmware/app/build/host/noct_host; the Bluetooth tests are
skipped on the USB-only build.
"""
import os
import re
import unittest
from test_midi_engine import scenario, HOST

BLE_BUILD=os.path.basename(str(HOST)).startswith('noct_host')
LINK=['tap GLO','spin PRESET 1','expect page 7']


class BluetoothTests(unittest.TestCase):
    @unittest.skipUnless(BLE_BUILD,'USB-only build')
    def test_bluetooth_starts_at_boot_and_carries_notes_both_ways(self):
        output=scenario(['wait 1500','expect ble_state 2','ble connect','wait 50','out','key 7 down',
                         'key 7 up','midi BLE 90 40 64','expect led_keys 2048','midi BLE 80 40 00',
                         'expect led_keys 0','out'])
        self.assertIn('out 90 3C 64 -> USB BLE',output)
        self.assertIn('out 80 3C 00 -> USB BLE',output)

    @unittest.skipUnless(BLE_BUILD,'USB-only build')
    def test_bluetooth_switch_is_saved_and_respected_at_boot(self):
        scenario(['wait 1500']+LINK+['spin K1 -1','expect ble_off 1','expect ble_state 0','wait 1000',
                  'tap SAVE','expect saves 1','reboot','wait 1500','expect ble_off 1','expect ble_state 0']+LINK+
                 ['spin K1 1','expect ble_off 0','wait 1100','expect ble_state 2'])

    def test_usb_keeps_working_with_bluetooth_off(self):
        output=scenario(['wait 800']+LINK+['spin K1 -1','tap HOME','out','key 7 down','key 7 up','out'])
        self.assertRegex(output,r'out 90 3C 64 -> USB')


class AppModeTests(unittest.TestCase):
    def test_apps_mode_play_sends_midi_start_and_stop(self):
        output=scenario(['wait 800']+LINK+['spin K2 1','expect host_mode 1','out','tap PLAY',
                         'expect playing 1','tap PLAY','expect playing 0','out'])
        self.assertIn('out FA',output)
        self.assertIn('out FC',output)
        self.assertNotIn('out BF 6E',output)                     # no script CC in APPS mode

    def test_daw_mode_still_uses_the_script_controls(self):
        output=scenario(['wait 800','out','tap PLAY','out'])
        self.assertIn('out BF 6E 7F',output)
        self.assertNotIn('out FA',output)

    def test_clock_out_sends_24_ppqn_and_ignores_incoming_clock(self):
        # 120 BPM: 48 clocks per second (the counter covers the whole run, the first 800 ms without clock out)
        scenario(['wait 800']+LINK+['expect clocks_out 0','spin K3 1','expect clock_out 1','wait 1000',
                  'expect clocks_out >45','expect clocks_out <52'])
        scenario(['wait 800']+LINK+['spin K3 1']+['midi USB F8 00 00','wait 10']*50+['expect bpm10 1200'])

    def test_master_starts_the_generator_with_play_and_obeys_an_incoming_stop(self):
        scenario(['wait 800']+LINK+['spin K2 1','spin K3 1','tap SEQ','tap PLAY','wait 300',
                  'expect gen_running 1','midi USB FA 00 00','expect gen_running 1',
                  'midi USB FC 00 00','expect gen_running 0','expect active 0'])

    def test_knobs_can_send_on_channel_1(self):
        output=scenario(['wait 800']+LINK+['spin K4 1','expect knob_ch 1','tap HOME','out','spin K1 1','out'])
        self.assertIn('out B0 14 41',output)
        self.assertNotIn('out BF 14',output)

    def test_link_settings_survive_save_and_reboot(self):
        scenario(['wait 800']+LINK+['spin K2 1','spin K3 1','spin K4 1','wait 1000','tap SAVE','expect saves 1',
                  'reboot','wait 800','expect host_mode 1','expect clock_out 1','expect knob_ch 1'])


if __name__=='__main__':
    unittest.main(verbosity=2)
