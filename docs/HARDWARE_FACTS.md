# FM-1 hardware and platform facts

Facts NOCT relies on, with their source in this repository. **UNKNOWN** means not verified here; do not build on
it without checking.

| Area | Fact | Source |
| --- | --- | --- |
| Device | M-VAVE FM-1, JieLi AC79 (wl82) family, pi32v2 CPU | `upstream/BUILDING.md`, `upstream/base/tools/build.py` |
| Exact board / PCB revision | **UNKNOWN** (not inspected) | — |
| Toolchain / SDK | JieLi Linux toolchain (clang 4.0.1, pi32v2), AC79 SDK `AC79NN_SDK_V1.2.1_2023-12-13` | `upstream/BUILDING.md`, `.github/workflows/firmware.yml` |
| Update path | `.fwsc` package = app + felucca-bsp update loader, written over USB MIDI SysEx by the installer; the bootloader itself is not written | `upstream/BUILDING.md`, `site/lib/fm1ota.js` |
| Recovery | Hold OCT− and OCT+ for 5 s for update mode; stock firmware via official M-UPGRADE; otherwise FM-1-transporter. **Not verified by this project on hardware** | `upstream/BUILDING.md` |
| Crash handling | Two crashes while starting → safe mode (no audio, log readout); a crashed Bluetooth run leaves Bluetooth off at the next boot | `upstream/base/device/main_fm1.c`, `device/radio.c` |
| Settings store | Object OBJ_PROJ at 0xFC000, panel calibration 0xDC000, Bluetooth syscfg 0xDE000; max 3840 B per object, CRC-checked with two copies | `firmware/app/app.h`, `upstream/base/device/storage.c`, `include/plat.h` |
| RAM budget | 96 KiB `.data`+`.bss` (USB build uses ~20 KiB); pool 344 KiB | `upstream/base/tools/build.py` image check, CI log |
| Stacks | System (interrupt) stack ~7.9 KiB, user stack ~24 KiB, guarded | `upstream/base/device/app.ld` |
| Timer | `app_tick` at 1 kHz inside a 10 kHz timer interrupt; `plat_lock` disables all interrupts (USB included) | `upstream/base/device/main_fm1.c`, `plat_fm1.c` |
| Display | 240×240 RGB565, drawn in bands | `upstream/base/device/gfx.c`, `firmware/app/afterglow.c` |
| Controls | 14 buttons, 7 encoders (SELECT, ALGORITHM, PRESET, K1–K4), MASTER pot, 27 keys F3–G5, two LED brightness levels | `upstream/base/include/plat.h` |
| MIDI ports | USB MIDI in/out; TRS MIDI **in only** (no MIDI out jack); Bluetooth LE MIDI in/out when built with `ble = true` | `upstream/base/include/plat.h` |
| Bluetooth | JieLi BT libraries from the AC79 SDK via fm1-base's shim; peripheral advertising the BLE-MIDI service as **"FM-1"**; pairing kept in OBJ_BLE; started at boot unless the app or a crash says off | `upstream/base/ble/midi/le_midi.c`, `include/plat.h` |
| USB class | Class-compliant USB MIDI (works without drivers on macOS/Windows per upstream); behaviour with every Android phone **UNKNOWN** (needs an OTG-capable phone; not tested here) | `upstream/BUILDING.md` |
| Bluetooth + Android | Android supports BLE-MIDI through its MIDI API, but an app (or a connector app) must open the device; **not tested here** | — |
| Bluetooth radio on this hardware with NOCT | Built and image-checked in CI only; **not run on a real FM-1 by this project** | CI |
