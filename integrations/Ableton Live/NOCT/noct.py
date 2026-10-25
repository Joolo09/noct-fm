# SPDX-License-Identifier: MPL-2.0
"""Live 11/12 control surface. Uses Live's bundled Python framework."""
from _Framework.ControlSurface import ControlSurface
from _Framework.ButtonElement import ButtonElement
from _Framework.EncoderElement import EncoderElement
from _Framework.InputControlElement import MIDI_CC_TYPE
import Live


class Noct(ControlSurface):
    def __init__(self, c_instance):
        super(Noct, self).__init__(c_instance)
        self._active = True
        self._sent = {}
        self._noct_controls = []
        self._noct_callbacks = []
        with self.component_guard():
            for cc in [7] + list(range(40, 52)) + [110, 111, 112, 119]:
                control = (ButtonElement(True, MIDI_CC_TYPE, 15, cc) if cc >= 110 else
                           EncoderElement(MIDI_CC_TYPE, 15, cc, Live.MidiMap.MapMode.absolute))
                callback = lambda value, number=cc: self._on_cc(number, value)
                control.add_value_listener(callback)
                self._noct_controls.append(control)
                self._noct_callbacks.append(callback)
        self._register_timer_callback(self._feedback)

    def _parameter(self, cc):
        track = self.song().view.selected_track
        if cc == 7:
            return self.song().master_track.mixer_device.volume
        if cc == 40:
            return track.mixer_device.volume
        if cc == 41:
            return track.mixer_device.panning
        if cc in (42, 43):
            sends = track.mixer_device.sends
            return sends[cc - 42] if len(sends) > cc - 42 else None
        if 44 <= cc <= 51:
            selected = track.view.selected_device
            if selected is not None:
                parameters = selected.parameters
                index = cc - 43
                return parameters[index] if len(parameters) > index else None
        return None

    def _on_cc(self, cc, value):
        if cc == 119:
            self._active = value == 1
            self._sent.clear()
            self._feedback()
            return
        if not self._active:
            return
        if cc == 110 and value:
            self.song().start_playing()
        elif cc == 111 and value:
            self.song().stop_playing()
        elif cc == 112 and value:
            self.song().record_mode = not self.song().record_mode
        else:
            parameter = self._parameter(cc)
            if parameter is not None and parameter.is_enabled:
                scaled = parameter.min + value / 127.0 * (parameter.max - parameter.min)
                parameter.value = round(scaled) if parameter.is_quantized else scaled
        self._feedback()

    def _send_value(self, cc, value):
        value = max(0, min(127, int(value)))
        if self._sent.get(cc) != value:
            self._send_midi((0xBF, cc, value))
            self._sent[cc] = value

    def _feedback(self):
        if not self._active:
            return
        for cc in range(40, 52):
            parameter = self._parameter(cc)
            value = 0
            if parameter is not None and parameter.max > parameter.min:
                value = round(127 * (parameter.value - parameter.min) / (parameter.max - parameter.min))
            self._send_value(cc, value)
        self._send_value(116, 127 if self.song().is_playing else 0)
        self._send_value(117, 127 if self.song().record_mode else 0)

    def disconnect(self):
        self._unregister_timer_callback(self._feedback)
        for control, callback in zip(self._noct_controls, self._noct_callbacks):
            control.remove_value_listener(callback)
        super(Noct, self).disconnect()
