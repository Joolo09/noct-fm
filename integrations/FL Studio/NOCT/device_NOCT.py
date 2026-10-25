# name=NOCT FM
"""NOCT FM / FL Studio. Controls use channel 16; notes pass normally.
CC 24-35 stay unhandled for FL's Link to controller.
"""
import channels
import device
import mixer
import transport

_active = True
_sent = {}
_tempo_high = None
_tempo_sent = None


def _send(cc, value):
    value = max(0, min(127, int(value)))
    if device.isAssigned() and _sent.get(cc) != value:
        device.midiOutMsg(0xBF | (cc << 8) | (value << 16))
        _sent[cc] = value


def _feedback():
    global _tempo_sent
    if not _active:
        return
    track = mixer.trackNumber()
    _send(20, round(mixer.getTrackVolume(track) * 127))
    _send(21, round((mixer.getTrackPan(track) + 1) * 63.5))
    _send(22, round((mixer.getTrackStereoSep(track) + 1) * 63.5))
    channel = channels.selectedChannel()
    if channel >= 0:
        _send(23, round(channels.getChannelVolume(channel) * 127))
    _send(116, 127 if transport.isPlaying() else 0)
    _send(117, 127 if transport.isRecording() else 0)
    _send(113, 127 if mixer.isTrackSlotsEnabled(track) else 0)
    bpm10 = max(200, min(9990, round(mixer.getCurrentTempo() * 10)))
    if device.isAssigned() and _tempo_sent != bpm10:
        # Always send the pair together, even when the high byte is unchanged.
        device.midiOutMsg(0xBF | (114 << 8) | ((bpm10 >> 7) << 16))
        device.midiOutMsg(0xBF | (115 << 8) | ((bpm10 & 127) << 16))
        _tempo_sent = bpm10


def OnInit():
    global _tempo_sent
    _sent.clear()
    _tempo_sent = None
    _feedback()


def OnRefresh(flags):
    _feedback()


def OnIdle():
    _feedback()


def OnMidiMsg(event):
    global _active, _tempo_high, _tempo_sent
    if event.status != 0xBF:
        return
    cc, value = event.data1, event.data2
    if cc == 119:
        _active = value in (0, 2)  # 2: one MIDI preset for both DAWs
        _sent.clear()
        _tempo_sent = None
        event.handled = True
        _feedback()
        return
    if not _active:
        return
    track = mixer.trackNumber()
    if cc == 20:
        mixer.setTrackVolume(track, value / 127.0)
    elif cc == 21:
        mixer.setTrackPan(track, value / 63.5 - 1.0)
    elif cc == 22:
        mixer.setTrackStereoSep(track, value / 63.5 - 1.0)
    elif cc == 23:
        channel = channels.selectedChannel()
        if channel >= 0:
            channels.setChannelVolume(channel, value / 127.0)
    elif cc == 7:
        mixer.setTrackVolume(0, value / 127.0)
    elif cc == 113:
        if value:
            mixer.enableTrackSlots(track)
    elif cc == 114:
        _tempo_high = value
    elif cc == 115:
        if _tempo_high is not None:
            bpm10 = (_tempo_high << 7) | value
            _tempo_high = None
            if 200 <= bpm10 <= 9990:
                # Older FL versions can still receive MIDI; do not stop the script
                # merely because bidirectional tempo requires API 38.
                if hasattr(mixer, 'setCurrentTempo'):
                    mixer.setCurrentTempo(bpm10 / 10.0)
                _tempo_sent = None
    elif cc in (110, 111, 112):
        if value:
            {110: transport.start, 111: transport.stop, 112: transport.record}[cc]()
    else:
        return
    event.handled = True
    _feedback()
