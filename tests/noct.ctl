# Real app, real renderer, simulated hardware.
fixed
wait 800
expect profile 0
expect chan 0
expect sound 2
expect cc0 20
shot fl-studio.png
out
spin K1 1
expect_out USB BF 14 41
tap PLAY
expect_out USB BF 6E 7F
tap PLAY
expect_out USB BF 6F 7F
tap REC
expect_out USB BF 70 7F
midi USB BF 75 00
expect recording 0
key 0 down
wait 40
out
spin PRESET 1
expect profile 1
expect cc0 40
expect_out USB 80 35 00
expect_out USB BF 77 01
key 0 up
wait 800
shot ableton-live.png
midi USB BF 28 64
expect val0 100
spin K1 1
expect_out USB BF 28 65
midi USB BF 74 7F
expect playing 1
midi USB BF 74 00
expect playing 0
turn SELECT 1
expect bank 1
expect cc0 44
turn SELECT 1
expect bank 2
expect cc0 48
tap SAVE
reboot
fixed
wait 800
expect profile 1
expect bank 2
expect cc0 48
spin PRESET -1
expect profile 0
expect cc0 20
tap GLO
wait 100
shot setup.png
tap HOME
wait 100
shot home-return.png
press FX
tap REC
release FX
expect bank 0
