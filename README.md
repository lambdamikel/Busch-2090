#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3
####Author: Michael Wessel
####License: GPL 3
####Hompage: [Author's Homepage](http://www.michael-wessel.info/)
####Version: 0.7

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
Microtronic**, in this order of sequence, from left to right:
``HALT, NEXT, RUN, CCE, REG, STEP, BKP, RUN``: 

    #define HALT  1 
    #define NEXT  2 
    #define RUN   4
    #define CCE   8
    #define REG  16
    #define STEP 32
    #define BKP  64
    #define PGM 128 

The 4x4 keypad keys are hex from `0` to `F`, in bottom-left to
top-right order. You might consider to relabel the keys on the pad 
(I haven't done that):

    7 8 9 A       C D E F 
    4 5 6 B  ==>  8 9 A B
    1 2 3 C  ==>  4 5 6 7
    * 0 # D       0 1 2 3

Microtronic's **Carry** and **Zero** flag are the LEDs 1 and 2 of the
TM1638, the 1 **Hz clock LED** is LED 3 (from left to right). The LEDs
5 to 8 are used as **DOT outputs** (set by the data out op-code
``FEx``).

Unlike the original Microtronic, this emulator uses the leftmost digit
of the 8digit FM1638 to display the **current system status** (the
original Microtronic only featured a 6digit display). Currently, the
**status codes** are:

- ``H``: stopped 
- ``A``: enter address 
- ``P``: enter op-code 
- ``r``: running or entering / inspecting register via ``REG``  
- ``I``: keypad input from user requested 

Also unlike the original Microtronic, the emulator uses blinking
digits to indicate cursor position. The ``CCE`` key works a little bit
differently, but editing should be comfortable enough.

Typical operation sequences such as ``HALT-NEXT-00-RUN`` and
``HALT-NEXT-00-F10-NEXT-510-NEXT-C00-NEXT`` etc. will work as expected.
Also, try to load a demo program: ``HALT-PGM-7-RUN``.

Note that programs can be entered manually, using the keypad and
function keys, or you can load a fixed ROM program specified in the
Arduino sketch via the ``PGM`` button. These ROM programs are defined
in the ``busch2090.ino`` sketch, see variables ``MAX_PROGRAMS,
programs[MAX_PROGRAMS]`` and ``startAddresses[MAX_PROGRAMS]``
there. The first string in ``programs[MAX_PROGRAMS]`` is ``PGM 7``,
the second string is ``PGM 8``, etc.

###Hardcoded Demo Programs

- ``PGM 0, 1, 2, 3, 4``: not implemented yet
- ``PGM 5`` : clear memory
- ``PGM 6`` : load ``F01`` (NOPs) into memory
- ``PGM 7`` : simple input and output demo 
- ``PGM 8`` : crazy counter 
- ``PGM 9`` : ``F05`` (random generator) demo 

Still working on adding Nim game and set and display clock. Will
probably also add lunar lander game. 

###Required Third-Party Libraries 

The emulator requires the following libraries, which are the work of
others, and which are included in the ``library`` subdirectory: 

- ``Keypad`` library
- ``TM1638`` library - note that this is a modified version of the original one 

### Future Work 

1. Test all op-codes for correct behavior, correct Carry and Zero flag
behavior, etc.
2. The R3 reset button unfortunately also clears the RAM memory of
the emulator. Hence, add a reset button that doesn't do that.
3. Add the "real" ``PGM 7``, which is the Nim game. I need to get the
source code from somebody having a real Microtronic 2090 arround, as
there is no listing of this game in the Busch manuals :-(
4. Add some more ``PGM`` programs, e.g., the Lunar Lander from the
manual.
5. Implement the ``PGM 3`` and ``PGM 4`` clock programs (enter time
and display time).
6. Add drivers to the DOT output LEDs such that they can be used as
output pins, like in the real Microtronic. This might require a simple
transitor or Darlington driver.
7. Add four digital inputs, like in the real Busch Microtronic, for
hardware hacking. These inputs are read via the DIN data in op-code,
``FDx``, which is currently useless due to the lack of these
inputs. Perhaps I can use the still unassigned ``D0`` - ``D3`` Arduino
pins for that.
8. With 7. done, control a Speech Synthesizer from these ports! A
Speech Synthesizer extension board was announced as early as 1983 by
Busch, in the first Busch 2090 manual, but was never released.
9. Try to connect a character display, such as the Hitachi HD44780.
10. Implement ``BKP`` and ``STEP`` function keys (breakpoint and
step). I did not really use them a lot in 1983.

**Plenty of work to be done - let's go for it!**






