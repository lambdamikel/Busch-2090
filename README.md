#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3 and Arduino Mega 2560 
####Author: Michael Wessel
####License: GPL 3
####Hompage: [Author's Homepage](http://www.michael-wessel.info/)
####Contributer: [Martin Sauter (PGM 7 Code)](http://mobilesociety.typepad.com/) 
####Version: 0.95
####[YouTube Videos](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

###Abstract

This is an emulator of the Busch 2090 Microtronic Computer System for
the Arduino Uno R3 and Arduino Mega 2560. 

The Busch 2090 was an educational 4bit single-board computer system of
the early 1980s, manufactured by the company Busch Modellbau in
Germany. There is [some information about the Busch 2090 Microtronic
available here, including PDFs of the original manuals in
German](http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de).

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img4-small.JPG)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img5-small.JPG)

![Busch 2090 Microtronic Emulator for Arduino R3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega1-small.JPG)

See ``busch2090.ino`` or ``busch2090-mega.ino`` sketch for further
instructions, and [see the emulator in action
here.](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

This project consists of two sketches. The main emulator code is in
``busch2090.ino`` for the Uno, and ``busch2090-mega.ino`` for the Mega
2560. However, the emulator will not run / initialize properly if
``PGM-EEPROM.ino`` has not been uploaded at least once into the
Arduino first. The ``PGM-EEPROM.ino`` sketch initializes the EEPROM
with PGM example programs (see below). Without prior EEPROM
initialization, it is likely that the emulator won't work.

###Acknowledgements

Many thanks to [Martin Sauter](http://mobilesociety.typepad.com/) for
retrieving and entering the original code of ``PGM 7``, the Nim
game. It works!

###Hardware Requirements

For the Uno version: 

- An Arduino Uno R3 
- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs
- A 4x4 keypad with matrix encoding for hexadecimal input 

For the Mega 2560 version, you will also need

- An Arduino Mega 2560 R3 
- An LCD+Keypad shield
- An Ethernet+SDCard shield 

###Wiring 

For the Uno version:

    TM1638 module(14, 15, 16);

    byte colPins[COLS] = {5, 6, 7, 8};
    byte rowPins[ROWS] = {9, 10, 11, 12}; 

    pinMode(0, INPUT_PULLUP); // reset pin
  
    pinMode(1, INPUT_PULLUP); // digital input pins read by DIN instruction 
    pinMode(2, INPUT_PULLUP); 
    pinMode(3, INPUT_PULLUP); 
    pinMode(4, INPUT_PULLUP); 

    #define CPU_THROTTLE_ANALOG_PIN 5 // connect a potentiometer here for CPU speed throttle controll  
    #define CPU_THROTTLE_DIVISOR 10 // potentiometer dependent 
    #define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0
    
For the Mega 2560: 
    
    LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

    TM1638 module(49, 47, 45);

    byte rowPins[ROWS] = {37, 35, 33, 31}; 
    byte colPins[COLS] = {36, 34, 32, 30}; 

    #define DIN_PIN_1 22 // digital input pins read by DIN instruction 
    #define DIN_PIN_2 24
    #define DIN_PIN_3 26
    #define DIN_PIN_4 28

    #define RESET_PIN 53

    #define CPU_THROTTLE_ANALOG_PIN 15 // connect a potentiometer here for CPU speed throttle controll 
    #define CPU_THROTTLE_DIVISOR 10 // potentiometer dependent 
    #define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0

    // these are analog read values for LCD keypad keys, adjust if necessary
    #define NOTHING_KEY 1000
    #define SELECT_KEY  770
    #define LEFT_KEY    540
    #define DOWN_KEY    360
    #define UP_KEY      160

For the Mega, please note that you will have to clip or disconnect PIN
10 from the LCD shield, otherwise the SDCard will interfere with the
LCD shield. I am using extension headers for this, and just bent PIN
10 out of the way such it doesn't make contact.

###Description 

The **push buttons of the TM1638 are the function keys of the
Microtronic**, in this order of sequence, from left to right: ``HALT,
NEXT, RUN, CCE, REG, STEP, BKP, RUN``:

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

The Arduino Uno pins ``D1`` to ``D4`` (or ``D22``, ``D24``, ``D24``
and ``D26`` on the Arduino Mega) are read by the Microtronic data in
op-code ``FDx (DIN)``. Connecting them to ground will set the
corresponding bit to 1. See ``PGM D``.

Analog pin ``A5`` on the Uno (or ``A15`` on the Mega) is used as a CPU
speed throttle. Connect a potentiometer to adjust the speed of the
CPU:

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

The Mega version uses the select button of the LCD Keypad shield to
toggle between PC + current op-code display, register display,
extra-register display, and display off. Note that the emulator slows
down considerably with LCD being on.

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

For the Mega version, the following standard libraries are used, and
already part of the Arduino distribution (version 1.6.6):

- ``LiquidCrystal`` library
- ``SPI`` library
- ``SD`` library


### Future Work 

1. Test all op-codes more thoroughly for correct behavior, correct
Carry and Zero flag behavior, etc.
2. Add drivers to the DOT output LEDs such that they can be used as
output pins, like in the real Microtronic. This might require a simple
transitor or Darlington driver.
3. With 2. done, control a Speech Synthesizer from these ports. A
Speech Synthesizer extension board was announced as early as 1983 by
Busch, in the first Busch 2090 manual, but was never released.
4. Try to connect a character display, such as the Hitachi HD44780.
5. Implement ``BKP`` and ``STEP`` function keys (breakpoint and
step). I did not really use them a lot in 1983.

For the Mega version:

1. Implement ``PGM1`` and ``PGM2`` (load and save program) using the
SDCard. Use the LCD keypad buttons to select program. For save we
might just use a HEX number instead of a real file name.

