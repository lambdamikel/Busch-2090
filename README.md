#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3
###Author: Michael Wessel
###License: GPL 3
###Hompage: [Author's Homepage](http://www.michael-wessel.info/)
###Version: 0.7

###Abstract

This is an emulator of the Busch 2090 Microtronic Computer System for
the Arduino R3. The Busch 2090 was an educational 4bit single-board
computer system of the early 1980s, manufactured by the company Busch
Modellbau in Germany. There is [some information about the Busch 2090
Microtronic available here, including PDFs of the
original manuals in German](http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de).

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img2-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img3-small.jpg)

See ``busch2090.ino`` sketch for further instructions. 

###Hardware Requirements

- An Arduino Uno R3 
- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs
- A 4x4 keypad with matrix encoding for hexadecimal input 

###Wiring 

    TM1638 module(14, 15, 16);

    byte colPins[COLS] = {5, 6, 7, 8}; // row pinouts of the keypad
    byte rowPins[ROWS] = {9, 10, 11, 12}; // column pinouts of the keypad

###Description 

The **push buttons of the TM1638 are the function keys of the
Microtronic**, in this order of sequence:

    #define HALT  1 
    #define NEXT  2 
    #define RUN   4
    #define CCE   8
    #define REG  16
    #define STEP 32
    #define BKP  64
    #define PGM 128 

The 4x4 keypad keys are hex from `0` to `F`, in left-to-right,
bottom-left to top-right order.  Hence, ``* -> 0``, ``0 -> 1``, ...,
``D -> 3``, ..., and ``A -> F``. You might consider using a labeler to
relabel the keys on the pad: 

    C D E F 
    8 9 A B
    4 5 6 7
    0 1 2 3

**Carry** and **Zero** flag are the LEDs 0 and 1, 1 Hz **clock LED**
is LED 2.  LEDs 4 to 7 are used for **DOT output** (FEx op code).

Unlike the original Microtronic, this emulator uses the leftmost digit
of the 8digit FM1638 to display the **current system status** (the
original Microtronic only featured a 6digit display). Currently, the
**status codes** are:

- H: stopped 
- A: enter address 
- P: enter OP code 
- r: running or entering / inspecting register via REG  
- I: keypad input from user requested 

Unlike the original Busch 2090 Microtronic, this emulator uses
blinking digits to indicate cursor position. The CCE key works a
little bit differently, but editing should be comfortable enough.

Typical operation sequences such as "HALT-NEXT-00-RUN" and
"HALT-NEXT-00-F10-NEXT-510-NEXT-C00-NEXT etc."  will work as expected.
Also, try to load a demo program: "HALT-PGM-7-RUN".

Note that programs can be entered manually, using the keypad and
function keys, or you can load a fixed program from the sketch Arduino
program using the PGM button. See the `MAX_PROGRAMS, programs[]` and
`startAddresses[]` in the sketch. The first string in `programs[]` is
PGM 7, the second string is PGM 8, etc. 

###Hardcoded Demo Programs

- PGM 0, 1, 2, 3, 4: not implemented yet
- PGM 5 : clear memory
- PGM 6 : load NOPs into memory
- PGM 7 : simple input and output demo 
- PGM 8 : crazy counter 
- PGM 9 : F05 (random generator) demo 

Still working on adding Nim game and set and display clock. Will
probably also add lunar lander game. 

###Required Third-Party Libraries 

The emulator requires the following libraries, which are the work of
others, and which are included in the ``library`` subdirectory: 

- ``Keypad`` library
- ``TM1638`` library - note that this is a modified version of the original one 



