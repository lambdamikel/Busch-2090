#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3
###Author: Michael Wessel
###License: GPL 3
###Hompage: [Author's Homepage](http://www.michael-wessel.info/)
###Version: 0.7

This is an emulator for the Busch 2090 Microtronic Computer System, an
educational single-board 4bit computer system from the early 1980,
manufactured by the company Busch Modellbau in Germany. There is [some
information about the Busch 2090 Microtronic available here, including
the manuals](http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de). 

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img2-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img3-small.jpg)

See busch2090.ino for further instructions. 

####Hardware requirments

- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs
- A 4x4 keypad with matrix encoding for hexadecimal input 

####Wiring 

    TM1638 module(14, 15, 16);

    byte colPins[COLS] = {5, 6, 7, 8}; // row pinouts of the keypad
    byte rowPins[ROWS] = {9, 10, 11, 12}; // column pinouts of the keypad

###Description 

The push buttons of the TM1638 are the function keys of the
Microtronic, in this order of sequence:

    #define HALT  1 
    #define NEXT  2 
    #define RUN   4
    #define CCE   8
    #define REG  16
    #define STEP 32
    #define BKP  64
    #define PGM 128 

Carry and Zero flag are LEDs 0 and 1, 1 Hz clock LED is LED 2.  LEDs 4
to 7 are used for DOT output (FEx op code). 

The leftmost digit of the FM1638 is used to display status (the
original 2090 doesn't do that):

- H: stopped 
- A: enter address 
- P: enter OP code 
- r: running or entering / inspecting register via REG  
- I: keypad input from user requested 

Unlike the original Busch Microtronic, this version uses blinking
digits to indicate cursor position. The CCE key works a little bit
differently, but editing should be comfortable enough. 

Typical operation sequences such as "HALT-NEXT-00-RUN" and
"HALT-NEXT-00-F10-NEXT-510-NEXT-C00-NEXT etc."  will work as
expected. 

Note that programs can also be storred in the Arduino sketch, see
variables `MAX_PROGRAMS, programs[]` and `startAddresses[]`.  First
string in `programs[]` is PGM 7, second string is PGM 8, etc. 

####Demo programs

- PGM 0, 1, 2, 3, 4: not implemented yet
- PGM 5 : clear memory
- PGM 6 : load NOPs into memory
- PGM 7 : simple input and output demo 
- PGM 8 : crazy counter 
- PGM 9 : F05 (random generator) demo 

Still working on adding Nim game and set and display clock. Will
probably also add lunar lander game. 

