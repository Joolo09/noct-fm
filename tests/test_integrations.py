"""Contract tests using DAW API doubles, not a substitute for real DAW tests."""
import importlib.util
import sys
import types
import unittest
from contextlib import nullcontext
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FLTests(unittest.TestCase):
    def setUp(self):
        self.calls = []
        self.sent = []
        modules = {
            'channels': dict(selectedChannel=lambda: 2, getChannelVolume=lambda n: .5,
                             setChannelVolume=lambda *a: self.calls.append(('channel', a))),
            'device': dict(isAssigned=lambda: True, midiOutMsg=self.sent.append),
            'mixer': dict(trackNumber=lambda: 3, getTrackVolume=lambda n: .5,
                          getTrackPan=lambda n: 0, getTrackStereoSep=lambda n: 0,
                          setTrackVolume=lambda *a: self.calls.append(('volume', a)),
                          setTrackPan=lambda *a: self.calls.append(('pan', a)),
                          setTrackStereoSep=lambda *a: self.calls.append(('width', a))),
            'transport': dict(isPlaying=lambda: False, isRecording=lambda: False,
                              start=lambda: self.calls.append('play'),
                              stop=lambda: self.calls.append('stop'),
                              record=lambda: self.calls.append('record'))}
        for name, attrs in modules.items():
            sys.modules[name] = types.SimpleNamespace(**attrs)
        self.script = load('integrations/FL Studio/NOCT/device_NOCT.py', 'fl_noct_test')

    def event(self, cc, val, status=0xBF):
        event = types.SimpleNamespace(status=status, data1=cc, data2=val, handled=False)
        self.script.OnMidiMsg(event)
        return event

    def test_notes_and_other_channels_pass(self):
        self.assertFalse(self.event(53, 100, 0x90).handled)
        self.assertFalse(self.event(20, 100, 0xB0).handled)
        self.assertEqual(self.calls, [])

    def test_controls_and_transport(self):
        self.assertTrue(self.event(20, 127).handled)
        self.assertIn(('volume', (3, 1.0)), self.calls)
        self.event(21, 0)
        self.assertIn(('pan', (3, -1.0)), self.calls)
        for cc in (110, 111, 112):
            self.event(cc, 127)
            self.event(cc, 0)
        self.assertEqual(self.calls[-3:], ['play', 'stop', 'record'])

    def test_manual_links_remain_available(self):
        for cc in range(24, 36):
            self.assertFalse(self.event(cc, 50).handled)

    def test_profile_gating_and_feedback_cache(self):
        self.script.OnInit()
        self.assertTrue(self.sent)
        before = len(self.sent)
        self.script.OnIdle()
        self.assertEqual(len(self.sent), before)
        self.event(119, 1)
        self.assertFalse(self.event(20, 127).handled)
        self.assertEqual(self.calls, [])
        self.event(119, 0)
        self.assertTrue(self.event(20, 100).handled)


class Parameter:
    def __init__(self, minimum=0, maximum=1, value=.5, quantized=False):
        self.min, self.max, self.value = minimum, maximum, value
        self.is_quantized, self.is_enabled = quantized, True


class LiveTests(unittest.TestCase):
    def setUp(self):
        class Surface:
            def __init__(surface, instance):
                surface.sent = []
            def component_guard(surface): return nullcontext()
            def song(surface): return self.song
            def _send_midi(surface, message): surface.sent.append(message)
            def _register_timer_callback(surface, cb): pass
            def _unregister_timer_callback(surface, cb): pass
            def disconnect(surface): pass

        class Control:
            def __init__(self, *args): self.callbacks = []
            def add_value_listener(self, cb): self.callbacks.append(cb)
            def remove_value_listener(self, cb): self.callbacks.remove(cb)

        for module, attr, value in [('ControlSurface', 'ControlSurface', Surface),
                                     ('ButtonElement', 'ButtonElement', Control),
                                     ('EncoderElement', 'EncoderElement', Control),
                                     ('InputControlElement', 'MIDI_CC_TYPE', 1)]:
            sys.modules['_Framework.' + module] = types.SimpleNamespace(**{attr: value})
        sys.modules['Live'] = types.SimpleNamespace(MidiMap=types.SimpleNamespace(
            MapMode=types.SimpleNamespace(absolute=0)))
        track = types.SimpleNamespace(mixer_device=types.SimpleNamespace(
            volume=Parameter(), panning=Parameter(-1, 1, 0), sends=[Parameter(), Parameter()]),
            view=types.SimpleNamespace(selected_device=types.SimpleNamespace(
                parameters=[Parameter() for _ in range(9)])))
        self.song = types.SimpleNamespace(view=types.SimpleNamespace(selected_track=track),
            master_track=track, is_playing=False, record_mode=False)
        self.song.start_playing = lambda: setattr(self.song, 'is_playing', True)
        self.song.stop_playing = lambda: setattr(self.song, 'is_playing', False)
        self.script = load('integrations/Ableton Live/NOCT/noct.py', 'live_noct_test').Noct(None)

    def test_mixer_macro_and_transport(self):
        for cc in range(40, 52):
            self.script._on_cc(cc, 127)
            self.assertEqual(self.script._parameter(cc).value, self.script._parameter(cc).max)
        self.script._on_cc(110, 127)
        self.assertTrue(self.song.is_playing)
        self.script._on_cc(111, 127)
        self.assertFalse(self.song.is_playing)
        self.script._on_cc(112, 127)
        self.assertTrue(self.song.record_mode)

    def test_missing_sends_and_device(self):
        track = self.song.view.selected_track
        track.mixer_device.sends = []
        track.view.selected_device = None
        for cc in range(42, 52):
            self.assertIsNone(self.script._parameter(cc))
            self.script._on_cc(cc, 100)
        self.script._feedback()

    def test_feedback_cache_and_profile(self):
        self.script._feedback()
        size = len(self.script.sent)
        self.script._feedback()
        self.assertEqual(len(self.script.sent), size)
        self.script._on_cc(119, 0)
        self.script._on_cc(110, 127)
        self.assertFalse(self.song.is_playing)
        self.script._on_cc(119, 1)
        self.script._on_cc(110, 127)
        self.assertTrue(self.song.is_playing)
        self.script.disconnect()

    def test_quantized_parameter(self):
        p = self.script._parameter(44)
        p.min, p.max, p.is_quantized = 0, 7, True
        self.script._on_cc(44, 64)
        self.assertEqual(p.value, 4)


if __name__ == '__main__':
    unittest.main(verbosity=2)
