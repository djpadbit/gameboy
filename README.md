# FXGB
## This is not a real emulator but more of a POC
FXGB is a port of a gameboy emulator (look at the source of the fork for original emulator) for the casio FX range of calculators.
The program is currently running at ~2fps (~11fps overclocked max on tetris) so it's not really useable.
I made it mostly for fun so don't really think it's gonna be maintained much.

Fun fact: this is my first add-in with gint.

# Current features:
* It does run games (at blazingly fast 2fps)
* I guess it's pretty cool ?
* Ehhhh

# How to use:
Put a gameboy rom on the root of the calculator with a short name preferably. (Only tested with Tetris and Super Mario Land)

Launch the add-in, then enter the rom file name like it says. Then just press any key until you reach a screen with 6 numbers on the side.
If you are interested, the first number is the fps, the second is the mode, the 2 next are the x and y offsets and the 2 last are the x and y scale resolution.

The controles are 

SHIFT - A

ALPHA - B

OPTN - Select 

VARS - Start

F1 - Set mode to 0 (playing mode)

F2 - Set mode to 1 (Offset adjusting mode)

F3 - Set mode to 2 (Scaling adjusting mode)

F4 - Reset scaling and offset to default

F5 - Toggle debug display (numbers on the left)

The arrows control the game in mode 0, the offset in mode 1 and the scaling in mode 2