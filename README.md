#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3
####Author: Michael Wessel
####License: GPL 3
####Hompage: [Author's Homepage](http://www.michael-wessel.info/)
####Contributer: [Martin Sauter (PGM 7 Code)](http://mobilesociety.typepad.com/) 
####Version: 0.95
####[YouTube Videos](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

###Abstract

This is an emulator of the Busch 2090 Microtronic Computer System for
the Arduino R3. The Busch 2090 was an educational 4bit single-board
computer system of the early 1980s, manufactured by the company Busch
Modellbau in Germany. There is [some information about the Busch 2090
Microtronic available here, including PDFs of the
original manuals in German](http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de).

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img4-small.JPG)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img5-small.JPG)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img6-small.JPG)

See ``busch2090.ino`` sketch for further instructions, and 
[see the emulator in action here.](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

This project consists of two sketches. The main emulator code is in
``busch2090.ino``. However, the emulator will not run / initialize
properly if ``PGM-EEPROM.ino`` has not been uploaded at least once
into the Arduino prior to running ``busch2090.ino``. The
``PGM-EEPROM.ino`` sketch initializes the EEPROM with PGM example
programs (see below). Without prior EEPROM initialization, it is
likely that the emulator won't work.

###Acknowledgements

Many thanks to [Martin Sauter](http://mobilesociety.typepad.com/) for retrieving and
entering the original code of ``PGM 7``, the Nim game. It works!

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

Notice that the Arduino reset button will erase the emulator's program 
memory. To only reset emulator while keeping the program in memory, 
connect Arduino pin ``D0 (RX)`` to ground. 

The Arduino pins ``D1`` to ``D4`` are read by the Microtronic data in
op-code ``FDx (DIN)``. Connecting them to ground will set the
corresponding bit to 1. See ``PGM D``.

Analog pin ``A5`` is used as a CPU speed throttle. Connect a
potentiometer to adjust the speed of the CPU: 

    #define CPU_THROTTLE_ANALOG_PIN 5 
    #define CPU_THROTTLE_DIVISOR 50 // potentiometer dependent 
    #define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0 

Unlike the original Microtronic, this emulator uses the leftmost digit
of the 8digit FM1638 to display the **current system status** (the
original Microtronic only featured a 6digit display). Currently, the
**status codes** are:

- ``H``: stopped 
- ``A``: enter address 
- ``P``: enter op-code 
- ``r``: running program
- ``?``: keypad input from user requested  
- ``i``: entering / inspecting register via ``REG``  
- ``t`` : entering clock time (``PGM 3``) 
- ``C`` : showing clock time (``PGM 4``) 

Also unlike the original Microtronic, the emulator uses blinking
digits to indicate cursor position. The ``CCE`` key works a little bit
differently, but editing should be comfortable enough.

Typical operation sequences such as ``HALT-NEXT-00-RUN`` and
``HALT-NEXT-00-F10-NEXT-510-NEXT-C00-NEXT`` etc. will work as expected.
Also, try to load a demo program: ``HALT-PGM-7-RUN``.

Note that programs can be entered manually, using the keypad and
function keys, or you can load a fixed ROM program specified in the
Arduino sketch via the ``PGM`` button. These ROM programs are defined
in the ``busch2090.ino`` sketch as ``PGM7`` to ``PGMD`` macros. 

###``PGM`` Demo Programs are Stored in EEPROM  

Please first run the ``PGM-EEPROM.ino`` sketch. This will load 5
example programs into the Arduino's EEPROM. The emulator won't work /
won't initialize correctly if the EEPROM has not been prepared.  The
programs stored into and loaded from the EEPROM are ``PGM 7`` to ``PGM
B``:

- ``PGM 0, 1, 2`` : not implemented yet, not function
- ``PGM 3`` : set time / clock (not a real program, i.e., nothing is loaded into program memory for this function) 
- ``PGM 4`` : show time / clock  (not a real program, i.e., nothing is loaded into program memory for this function) 
- ``PGM 5`` : clear memory
- ``PGM 6`` : load ``F01`` (NOPs) into memory
- ``PGM 7`` : the Nim game as documented in the Busch Manual Vol. 1, page 7. Many thanks to Martin Sauter for
retrieving the code from an original Busch Microtronic and contributing it to this project! 
- ``PGM 8`` : crazy counter
- ``PGM 9`` : the electronic die, from Busch Manual Vol. 1, page 10
- ``PGM A`` : three digit counter 
- ``PGM B`` : scrolling LED DOT output light

The flash memory of an Uno cannot hold more programs it seems. 

Still working save and load (probably SDCard shield). Will probably
also add Lunar Lander game from Microtronic Manual Vol 1. Stay tuned!

###Required Third-Party Libraries 

The emulator requires the following libraries, which are the work of
others, and which are included in the ``library`` subdirectory: 

- ``Keypad`` library
- ``TM1638`` library - note that this is a modified version of the original one 

### Future Work 

1. Test all op-codes more thoroughly for correct behavior, correct Carry and Zero flag
behavior, etc. 
2. Add drivers to the DOT output LEDs such that they can be used as
output pins, like in the real Microtronic. This might require a simple
transitor or Darlington driver.
3. With 2. done, control a Speech Synthesizer from these ports. A
Speech Synthesizer extension board was announced as early as 1983 by
Busch, in the first Busch 2090 manual, but was never released.
4. Try to connect a character display, such as the Hitachi HD44780.
5. Implement ``BKP`` and ``STEP`` function keys (breakpoint and
step). I did not really use them a lot in 1983.

**Plenty of work to be done - let's go for it!**

### Some Insights and Problems

The Arduino Uno might not be powerful enough to go forward with this
project, considering I want to also add an SDCard and LCD Module.  The
drivers require SRAM memory, too.  I am running out of dynamic (SRAM)
memory. Lunar Lander won't fit in. I need at least 396 bytes of SRAM
to store Lunar Lander, or I will have to reorganize the Microtronic
program memory such that a word uses only 12 bits instead of 24 bits
as currently.  I can put these programs as strings into PROGMEM or
EEPROM, but then I need at least one string buffer as big as Lunar
Lander, and that is a problem.  I have ordered an Arduino Mega 2560
:-)


