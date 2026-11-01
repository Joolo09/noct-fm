# NOCT FM — improvement prompt

A reusable brief for an AI agent (or a person) improving this firmware. Run it top to bottom; every step ends
with evidence, not an opinion.

## Role and goal

You are improving NOCT, a MIDI-controller firmware for the M-VAVE FM-1 used with FL Studio and Ableton Live.
Make it more reliable, more musical and easier to play — **without ever risking the device**.

## Hard rules (the device must never be bricked)

1. Do not modify `upstream/base` (bootloader, USB, OTA, flash driver, BSP). The app lives in `firmware/app`.
2. Flash is written only by SAVE (`project_save`). No background or periodic writes.
3. Settings stay compatible: append fields to `project_t`, validate every field in `project_valid`, migrate
   older formats in `project_load`. An unknown or invalid save must fall back to defaults, never crash.
4. The 1 kHz timer (`app_tick`) does constant, small work: no allocation, no big stack buffers, no loops that
   depend on user data beyond fixed bounds, no drawing. Heavy work (composition) runs in the UI thread and is
   handed over under `plat_lock`, held as briefly as possible (it disables all interrupts, including USB).
5. Every loop is bounded; every array index is range-checked or provably in range; no division by a value that
   can be zero; no recursion.
6. Every Note On has exactly one Note Off on the same channel — through stop, panic, octave change, mode or
   channel switch, clock loss and reboot.
7. A change is done only when the host tests pass **and** the real device image builds in CI with all image
   checks (RAM, `.ram_text` calls, MMIO, soft-double) green.

## Audit checklist

- **Note safety:** fuzz random button / key / encoder / MIDI-in sequences on the host build; after release,
  stop and panic, assert zero sounding notes and balanced On/Off per channel.
- **Timing:** swing, gates, external clock loss, Continue / Song Position, local preview vs DAW transport.
- **Musical quality:** listen to (print) many generator variations — key, cadence, repetition vs variety,
  register, voice leading; nothing monotonous, nothing out of key unless intended.
- **UX:** every visible knob has one clear job, values are readable, toasts explain hidden combos, defaults
  are useful at power-on (nothing surprising starts by itself).
- **Performance:** frame time and lock time stay small; no per-loop O(128×N) work that is not needed.
- **Docs and site:** README (RU), site (EN/RU), emulator help and on-screen hints describe the same controls.

## Working order

1. Read the code paths you will touch; reproduce a problem with a host scenario before fixing it.
2. Fix with the smallest change that removes the cause; add a regression test that fails before the fix.
3. Run `sh build.sh --host --no-ble`, every `tests/test_*.py`, `tests/noct.ctl` and `node tests/installer.test.mjs`.
4. Build the device image in CI on the branch; merge only when it is green.
5. Report what changed, what was verified, and what still needs a real FM-1 to confirm.
