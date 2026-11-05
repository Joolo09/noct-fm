# Changelog

## 0.9.3
- Bluetooth LE MIDI alongside USB (fm1-base's radio, unchanged): notes, controls and clock on both; advertises as
  "FM-1"; on / off on the new LINK page, kept by SAVE. A USB-only image is still built as a fallback.
- APPS host mode for apps such as Koala Sampler: PLAY sends MIDI Start / Stop.
- CLOCK OUT: the FM-1 sends MIDI Clock from its tempo; as master it ignores incoming clock and Start but always
  obeys Stop.
- Knob channel 16 or 1. Header shows Bluetooth (blinking while waiting, steady when connected).
- Tests run on both builds; `docs/HARDWARE_FACTS.md` lists the platform facts with sources.

## 0.9.2
- `docs/IMPROVE_PROMPT.md`: a reusable improvement brief (no-brick rules, audit checklist, definition of done).
- Fuzzing: 2000 random sessions left no note sounding; 200 now run in CI (`tests/test_fuzz.py`).
- Melodies never repeat one note four times in a row (PEDAL and the cadence excepted); ECHO alternates its side.

## 0.9.1
- Six new melody characters: runs, ostinato, hook, turns, octaves, echo (13 + AUTO).
- Each variation picks a phrase form (AABA, ABAB, AAAB, ABAC, ABBA), a motif answer (as is, inverted, backwards)
  and a phrase shape (arch, rise, fall, wave). The form is shown with the phrase number.

## 0.9.0
- SYNC K4 **SPLIT**: generator melody on channel 1, chords on 2, bass on 3; panic clears channels 2–3.
- SYNC K3: velocity 1–127, below 1 = key-sensed.
- In-scale mode lights the scale on the keys.

## 0.8.2
- The arpeggiator is always off after power-on; its settings are still restored.

## 0.8.1
- ALGORITHM browses 1000 phrase variations on any page instead of repeating a page knob.
- 8 melody characters, 24 progressions per scale family + AUTO (some with two chords per bar),
  chord colours: triads, 7ths, add9, sus2, mixed.

## 0.8.0
- Composer 2.0: motif, answer, contrast and cadence; BASS and FULL modes; 8 scales; chord styles; swing; human
  velocity; EDIT / hold EDIT / undo.
- Keyboard chords: DOM7, POWER, in-key, learned MEMORY chord, voicings, strum, scale lock.
- Tap tempo, CONVERGE / PEDAL arp orders and gate, CC120 on panic, arp buffers off the interrupt stack.
- New STYLE page, rings for every knob, unsaved-changes marker; settings format 6 with migration from 0.7.
- Site in English with a Russian switch.

## 0.7.x
- SEL chords play simultaneously without MIDI Clock and turn the arp off; quality edits under SEL keep chords on.
- Key lights show every sounding note, sustain and incoming MIDI.
- MIDI recovery under USB congestion, Song Position Pointer, Continue independent of feedback order.
- HOME hold switches MIXER (CC20–23) / MIDI MAP (CC24–27).
- AFTERGLOW pixel scenes; the interactive emulator runs the real C code.
- The installer refreshes its manifest and verifies size and SHA-256 before writing.
