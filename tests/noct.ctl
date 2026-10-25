# NOCT 0.3: every button, MIDI routes, chords, generator, tempo and transport.
wait 800
expect profile 2
expect chan 0
expect clock 0
expect bpm10 1200
expect chords 0
expect generator 0
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
tap SEL
expect chords 1
key 0 down
expect_out USB 90 35 64
expect_out USB 90 39 64
expect_out USB 90 3C 64
key 4 down
expect active 5
key 0 up
expect active 3
key 4 up
expect active 0
spin ALGO 1
expect chord_type 1
key 0 down
expect_out USB 90 38 64
tap OCT+
expect_out USB 80 35 00
expect_out USB 90 41 64
key 0 up
expect active 0
tap OCT-
tap SEL
expect chords 0
# Fixed CCs are unchanged by all musical controls.
spin K1 1
expect_out USB BF 14 41
expect cc0 20
expect cc3 23
midi USB BF 72 0B
midi USB BF 73 2F
expect bpm10 1455
midi USB BF 73 00
expect bpm10 1455
spin SELECT 1
expect bpm10 1465
expect_out USB BF 72 0B
expect_out USB BF 73 39
# GLO cycles AUTO -> INT -> EXT -> AUTO; FX is DAW effects, not latch.
tap GLO
expect clock 1
tap GLO
expect clock 2
tap GLO
expect clock 0
tap FX
expect_out USB BF 71 7F
expect latch 0
midi USB BF 71 00
expect fx 0
press ENV
expect sustain 1
expect_out USB B0 40 7F
release ENV
expect sustain 0
expect_out USB B0 40 00
press LFO
wait 700
expect_out USB B0 01 7F
release LFO
wait 700
expect_out USB B0 01 00
tap REC
expect_out USB BF 70 7F
midi USB BF 75 00
expect recording 0
# Generator uses a scale, editable density, root, range, and shared transport.
tap SEQ
expect generator 1
expect gen_running 1
expect_out USB BF 6E 7F
out
wait 1000
expect notes_out >2
tap PLAY
expect playing 0
expect gen_running 0
expect active 0
out
wait 400
expect notes_out 0
tap PLAY
expect gen_running 1
wait 300
expect notes_out >0
tap EDIT
press SEQ
spin K2 4
expect density 12
spin K3 1
spin K4 1
key 7 down
key 7 up
release SEQ
expect generator 1
tap SEQ
expect generator 0
tap PLAY
expect playing 0
expect active 0
spin PRESET 1
tap HOME
expect chan 0
expect bank 0
expect program 0
# ARP hold is now explicit ARP+ENV. FX+REC remains emergency panic.
tap ARP
expect arp 1
press ARP
tap ENV
release ARP
expect arp 1
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
expect active 0
expect pool 0
expect latch 1
press ARP
tap ENV
release ARP
expect latch 0
tap ARP
expect arp 0
# Incoming DAW stop gates generator; a fresh start resumes its grid.
tap SEQ
wait 100
midi USB BF 74 00
expect gen_running 0
expect active 0
out
wait 400
expect notes_out 0
midi USB BF 74 7F
expect gen_running 1
wait 200
expect notes_out >0
tap SEQ
midi USB FC 00 00
expect active 0
# No autosave. Both short and held SAVE work only while idle.
wait 6000
expect saves 0
tap SAVE
expect saves 1
reboot
wait 800
expect chan 0
expect bank 0
expect profile 2
expect bpm10 1465
expect generator 0
legacy
reboot
wait 800
expect chan 0
expect profile 2
expect clock 0
expect active 0
wait 1600
shot midi-home.png
