# NOCT MIDI 0.2 regressions: invariant routes, chromatic keyboard and arp.
wait 800
expect profile 2
expect chan 0
expect bank 0
expect scale 0
expect sound 2
expect touch 0
expect clock 1
out
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
key 1 down
expect_out USB 90 36 64
key 1 up
expect_out USB 80 36 00
key 2 down
expect_out USB 90 37 64
key 2 up
expect_out USB 80 37 00
key 3 down
expect_out USB 90 38 64
key 3 up
expect_out USB 80 38 00
key 4 down
expect_out USB 90 39 64
key 4 up
expect_out USB 80 39 00
key 5 down
expect_out USB 90 3A 64
key 5 up
expect_out USB 80 3A 00
key 6 down
expect_out USB 90 3B 64
key 6 up
expect_out USB 80 3B 00
key 7 down
expect_out USB 90 3C 64
key 7 up
expect_out USB 80 3C 00
key 8 down
expect_out USB 90 3D 64
key 8 up
expect_out USB 80 3D 00
key 9 down
expect_out USB 90 3E 64
key 9 up
expect_out USB 80 3E 00
key 10 down
expect_out USB 90 3F 64
key 10 up
expect_out USB 80 3F 00
key 11 down
expect_out USB 90 40 64
key 11 up
expect_out USB 80 40 00
key 12 down
expect_out USB 90 41 64
key 12 up
expect_out USB 80 41 00
key 13 down
expect_out USB 90 42 64
key 13 up
expect_out USB 80 42 00
key 14 down
expect_out USB 90 43 64
key 14 up
expect_out USB 80 43 00
key 15 down
expect_out USB 90 44 64
key 15 up
expect_out USB 80 44 00
key 16 down
expect_out USB 90 45 64
key 16 up
expect_out USB 80 45 00
key 17 down
expect_out USB 90 46 64
key 17 up
expect_out USB 80 46 00
key 18 down
expect_out USB 90 47 64
key 18 up
expect_out USB 80 47 00
key 19 down
expect_out USB 90 48 64
key 19 up
expect_out USB 80 48 00
key 20 down
expect_out USB 90 49 64
key 20 up
expect_out USB 80 49 00
key 21 down
expect_out USB 90 4A 64
key 21 up
expect_out USB 80 4A 00
key 22 down
expect_out USB 90 4B 64
key 22 up
expect_out USB 80 4B 00
key 23 down
expect_out USB 90 4C 64
key 23 up
expect_out USB 80 4C 00
key 24 down
expect_out USB 90 4D 64
key 24 up
expect_out USB 80 4D 00
key 25 down
expect_out USB 90 4E 64
key 25 up
expect_out USB 80 4E 00
key 26 down
expect_out USB 90 4F 64
key 26 up
expect_out USB 80 4F 00
expect active 0
expect notes_on 0
shot midi-home.png
out
tap HOME
spin K1 1
expect cc0 20
spin K2 1
expect cc1 21
spin K3 1
expect cc2 22
spin K4 1
expect cc3 23
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
tap SEL
spin K1 1
expect cc0 20
spin K2 1
expect cc1 21
spin K3 1
expect cc2 22
spin K4 1
expect cc3 23
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
tap SEQ
spin K1 1
expect cc0 20
spin K2 1
expect cc1 21
spin K3 1
expect cc2 22
spin K4 1
expect cc3 23
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
tap EDIT
spin K1 1
expect cc0 20
spin K2 1
expect cc1 21
spin K3 1
expect cc2 22
spin K4 1
expect cc3 23
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
tap GLO
spin K1 1
expect cc0 20
spin K2 1
expect cc1 21
spin K3 1
expect cc2 22
spin K4 1
expect cc3 23
key 0 down
expect_out USB 90 35 64
key 0 up
expect_out USB 80 35 00
spin SELECT 10
spin ALGO 2
spin PRESET 17
expect chan 0
expect bank 0
expect profile 2
expect program 0
expect scale 0
expect tempo 130
expect arp_mode 2
wait 6000
expect saves 0
expect active 0
out
spin SELECT -10
spin ALGO -2
midi USB BF 14 64
expect val0 100
spin K1 1
expect_out USB BF 14 65
midi USB BF 74 7F
expect playing 1
midi USB BF 74 00
expect playing 0
wait 5000
expect saves 0
out
tap PLAY
expect_out USB BF 6E 7F
wait 600
expect notes_out 0
tap PLAY
expect_out USB BF 6F 7F
out
# Octave revoice releases old and new notes correctly.
key 0 down
tap OCT+
expect_out USB 80 35 00
expect_out USB 90 41 64
key 0 up
expect_out USB 80 41 00
expect active 0
tap OCT-
out
# ARP works without a DAW clock.
tap ARP
expect arp 1
key 0 down
key 4 down
key 7 down
wait 600
expect notes_out >3
expect_out USB 90 35 64
expect_out USB 90 39 64
expect_out USB 90 3C 64
key 0 up
key 4 up
key 7 up
expect arp_note -1
expect pool 0
expect active 0
expect notes_on 0
out
key 0 down
key 1 down
key 2 down
key 3 down
key 4 down
key 5 down
key 6 down
key 7 down
key 8 down
key 9 down
key 10 down
key 11 down
key 12 down
key 13 down
key 14 down
key 15 down
key 16 down
key 17 down
key 18 down
key 19 down
key 20 down
key 21 down
key 22 down
key 23 down
key 24 down
key 25 down
key 26 down
expect pool 27
wait 3600
expect_out USB 90 4F 64
key 0 up
key 1 up
key 2 up
key 3 up
key 4 up
key 5 up
key 6 up
key 7 up
key 8 up
key 9 up
key 10 up
key 11 up
key 12 up
key 13 up
key 14 up
key 15 up
key 16 up
key 17 up
key 18 up
key 19 up
key 20 up
key 21 up
key 22 up
key 23 up
key 24 up
key 25 up
key 26 up
expect pool 0
expect active 0
expect notes_on 0
# ARP modifier knobs do not toggle ARP on release.
press ARP
spin K1 1
spin K2 1
spin K3 1
spin K4 5
wait 80
shot arp-edit.png
release ARP
expect arp 1
expect arp_rate 4
expect arp_mode 1
expect arp_oct 2
expect tempo 125
# Latch and panic.
tap FX
expect latch 1
key 0 down
wait 100
key 0 up
out
wait 400
expect notes_out >1
press FX
tap REC
release FX
expect latch 1
expect pool 0
expect active 0
tap FX
expect latch 0
# Clock loss and release without another F8 must not hang notes.
press GLO
tap PLAY
release GLO
expect clock 2
key 0 down
midi USB F8 00 00
wait 400
expect arp_note -1
key 0 up
expect pool 0
key 0 down
midi USB F8 00 00
key 0 up
expect active 0
press GLO
tap PLAY
release GLO
expect clock 1
# Held-note ARP toggle preserves releases.
key 2 down
tap ARP
expect arp 0
expect_out USB 90 37 64
key 2 up
expect active 0
# Only explicit, idle SAVE writes flash.
tap SAVE
wait 5000
expect saves 0
press SAVE
wait 1100
release SAVE
expect saves 1
reboot
wait 800
expect chan 0
expect profile 2
expect bank 0
expect scale 0
expect tempo 125
expect active 0
# Old DAW/user-bank settings migrate to one fixed MIDI preset.
legacy
reboot
wait 800
expect chan 0
expect bank 0
expect profile 2
expect scale 0
expect clock 1
expect cc0 20
expect sound 2
key 26 down
expect_out USB 90 4F 64
key 26 up
expect active 0
