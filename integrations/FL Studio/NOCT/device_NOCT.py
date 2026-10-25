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


def _send(cc, value):
    value = max(0, min(127, int(value)))
    if device.isAssigned() and _sent.get(cc) != value:
        device.midiOutMsg(0xBF | (cc << 8) | (value << 16))
        _sent[cc] = value


def _feedback():
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


def OnInit():
    _sent.clear()
    _feedback()


def OnRefresh(flags):
    _feedback()


def OnIdle():
    _feedback()


def OnMidiMsg(event):
    global _active
    if event.status != 0xBF:
        return
    cc, value = event.data1, event.data2
    if cc == 119:
        _active = value == 0
        _sent.clear()
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
    elif cc in (110, 111, 112):
        if value:
            {110: transport.start, 111: transport.stop, 112: transport.record}[cc]()
    else:
        return
    event.handled = True
    _feedback()
