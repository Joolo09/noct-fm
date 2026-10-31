# NOCT 0.5: end-to-end controls and fixed MIDI routing.
wait 800
expect profile 2
expect page 0
expect chan 0
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
out
spin K1 1
expect_out USB BF 14 41
spin K2 -1
expect_out USB BF 15 3F
tap SEL
expect page 1
expect chords 1
key 0 down
expect active 3
spin K1 1
expect chord_type 1
expect active 3
key 0 up
expect active 0
tap SEL
expect chords 0
tap HOME
press ENV
expect sustain 1
release ENV
expect sustain 0
press LFO
wait 700
expect_out USB B0 01 7F
release LFO
wait 700
expect_out USB B0 01 00
tap FX
expect_out USB BF 71 7F
tap REC
expect_out USB BF 70 7F
midi USB BF 75 00
expect recording 0
tap GLO
expect page 6
expect clock 0
spin K1 2
expect clock 2
spin K1 -2
expect clock 0
midi USB BF 72 0B
midi USB BF 73 39
expect bpm10 1465
tap SEQ
expect page 2
expect generator 1
expect playing 0
spin K1 2
expect gen_mode 2
wait 2200
expect gen_running 1
spin PRESET 1
expect page 3
spin K1 -3
expect density 5
spin K2 10
expect gate 95
tap EDIT
expect page 2
expect gen_pending 1
tap HOME
expect page 0
spin K1 1
expect gen_running 1
tap PLAY
expect playing 0
expect gen_running 0
expect active 0
tap PLAY
expect playing 1
expect gen_running 1
wait 400
midi USB FC 00 00
expect active 0
expect gen_running 0
midi USB FB 00 00
expect gen_running 1
wait 100
tap SEQ
expect generator 0
midi USB FC 00 00
tap ARP
expect arp 1
expect page 5
press ARP
tap ENV
release ARP
expect latch 1
key 0 down
wait 100
key 0 up
wait 300
press FX
tap REC
release FX
expect active 0
expect pool 0
press ARP
tap ENV
release ARP
expect latch 0
tap ARP
expect arp 0
tap HOME
wait 7000
expect saves 0
tap SAVE
expect saves 1
reboot
wait 800
expect page 0
expect generator 0
expect gen_mode 2
expect density 5
expect gate 95
expect bpm10 1465
legacy
reboot
wait 800
expect chan 0
expect profile 2
expect clock 0
expect active 0
shot midi-home.png
