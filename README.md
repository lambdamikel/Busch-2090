#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3
###Author: Michael Wessel
###License: GPL 3
###Hompage: http://www.michael-wessel.info
###Version: 0.7

This is an emulator for the Busch 2090 Microtronic Computer System, an
educational single-board 4bit computer system from the early 1980,
manufactured by the company Busch Modellbau in Germany. See
http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de

See busch2090.ino for further instructions. 

A TM1638 module with 8 7segment digits, 8 push buttons and 8 LEDs are
required, as well as a matrix 4x4 keypad for hexadecimal input. 
Wiring:

    TM1638 module(14, 15, 16);

    byte colPins[COLS] = {5, 6, 7, 8}; // row pinouts of the keypad
    byte rowPins[ROWS] = {9, 10, 11, 12}; // column pinouts of the keypad

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

Unlike the origin Busch Microtronic, this version uses blinking of LED
digits to indicate cursor position. CCE key works a bit
differently. Other than that, typical operation sequences such as
"HALT-NEXT-00-RUN" and "HALT-NEXT-00-<ENTER OP CODE 1>-NEXT-<ENTER OP
CODE 2>-NEXT- ... etc." will work comfortably. Programs can also be
storred in the Arduino sketch, see MAX_PROGRAMS, programs[] and
startAddresses[]. First string in programs array corresponds to PGM 7,
second string to PGM 8, etc.

Demo programs:

- PGM 0, 1, 2, 3, 4: not implemented yet
- PGM 5 : clear memory
- PGM 6 : load NOPs into memory
- PGM 7 : simple input and output demo 
- PGM 8 : crazy counter 
- PGM 9 : F05 (random generator) demo 

Still working on adding Nim game and set and display clock. Will
probably also add lunar lander game. 

