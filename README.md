#Busch-2090
##An emulator of the Busch 2090 Microtronic Computer System for Arduino Uno R3 and Arduino Mega 2560 
####Author: Michael Wessel
####License: GPL 3
####Hompage: [Author's Homepage](http://www.michael-wessel.info/)
####Contributer: [Martin Sauter (PGM 7 Code)](http://mobilesociety.typepad.com/) 
####Version: 2.0 
####[YouTube Videos](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

###Abstract

This is an emulator of the Busch 2090 Microtronic Computer System for
the Arduino Uno R3 and Arduino Mega 2560. 

The Busch 2090 was an educational 4bit single-board computer system of
the early 1980s, manufactured by the company Busch Modellbau in
Germany. There is [some information about the Busch 2090 Microtronic
available here, including PDFs of the original manuals in
German](http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de).

The designer of the original Busch Microtronic, Mr. Jörg Vallen of Busch, 
was also so kind to grant permission to include a full copy of the
manual set in the ``manuals/`` directory of this project. 

![Busch 2090 Microtronic Emulator for Arduino Mega 2560 Version 2](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v2-1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega 2560 Version 2](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v2-5-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Uno](https://github.com/lambdamikel/Busch-2090/blob/master/images/img4-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-6-small.jpg)

See ``busch2090.ino`` or ``busch2090-mega.ino``, or
``busch2090-mega-v2.ino``sketch for further instructions, and [see the
emulator in action
here.](https://www.youtube.com/channel/UC1dEZ22WoacesGdSWdVqfTw)

This project consists of two sketches. The main emulator code is in
``busch2090.ino`` for the Uno, and ``busch2090-mega.ino`` or
``busch2090-mega-v2.ino`` for the Mega 2560. However, the emulator
will not run / initialize properly if ``PGM-EEPROM.ino`` has not been
uploaded at least once into the Arduino first. The ``PGM-EEPROM.ino``
sketch initializes the EEPROM with PGM example programs (see
below). Without prior EEPROM initialization, it is likely that the
emulator won't work. Important - if you are using ``busch2090-mega-v2.ino``, 
then please use ``PGM-EEPROM-v2.ino``. 

The ``busch2090-mega-v2.ino`` is the Busch Microtronic mega emulator
version 2.  This version uses different hardware, see below, and is
meant to be cased.  The first pictures show the mega emulator version
2. 

Also, you will find some programs in the ``software``
subdirectory. See below for instructions how to use them, and
for a brief explanation of the ``.MIC`` file format. 

###Acknowledgements

Many thanks to [Martin Sauter](http://mobilesociety.typepad.com/) for
retrieving and entering the original code of ``PGM 7``, the Nim
game. It works!

###Hardware Requirements

For the Uno version, ``busch2090.ino``: 

- An Arduino Uno R3 
- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs
- A 4x4 keypad with matrix encoding for hexadecimal input 

For the Mega 2560 version, ``busch2090-mega.ino``, you will need 

- An Arduino Mega 2560 R3 
- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs
- A 4x4 keypad with matrix encoding for hexadecimal input 
- An LCD+Keypad shield
- An Ethernet+SDCard shield 

For the Mega 2560 version 2, ``busch2090-mega-v2.ino``, which is meant
to be housed in a case, you will need 

- An Arduino Mega 2560 R3 
- A 4x4 keypad with matrix encoding for hexadecimal input 
- A 3x4 telephone keypad, for function buttons and DIN input, NOT matrix encoded 
- 8 LEDs and matching resistors (for 5 V) 
- 8 N.O. momentary push buttons
- A power switch
- 2 Adafruit 7Segment LED backpacks 
- 2 potentiometer, one for LCD contrast (100 Ohms), one for CPU speed (200 Ohms)
- An Ethernet+SDCard shield 
- A 4x20 LCD display, standard Hitachi HD44780 
- A laser-cut / laser-printer face place. The blueprint / layout is in the faceplate subdirectory.

###Wiring 

For the Uno version:

    TM1638 module(14, 15, 16);

    byte colPins[COLS] = {5, 6, 7, 8};
    byte rowPins[ROWS] = {9, 10, 11, 12}; 
    
    #define DIN_PIN_1 1
    #define DIN_PIN_2 2
    #define DIN_PIN_3 3
    #define DIN_PIN_4 4

    #define RESET_PIN 0

    #define CPU_THROTTLE_ANALOG_PIN 5 // connect a potentiometer here for CPU speed throttle controll  
    #define CPU_THROTTLE_DIVISOR 10 // potentiometer dependent 
    #define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0
    
For the Mega 2560 version 1: 
    
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

    // these are analog values read from the LCD+Keypad shield, adjust if necessary 
    // the read analog values must be greater than these, they are lower bounds: 
    #define NOTHING_KEY 1000
    #define SELECT_KEY  770
    #define LEFT_KEY    540
    #define DOWN_KEY    360
    #define UP_KEY      160

For the Mega version 1, please note that you will have to clip or
disconnect PIN 10 from the LCD+Keypad, otherwise the SDCard will not
function properly. I am using extension headers for this (just bent
PIN 10 out of the way such it doesn't make contact). See this image: 

![Bent Pin 10 of LCD+Keypad Shield](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega5-small.jpg)

For the Mega 2560 version 2 - there is more freedom how to set up the hardware, but I did it as follows: 

     #define RESET  47 // soft reset 

     #define BACK   63
     #define RIGHT  64
     #define UP     65
     #define DOWN   66
     #define LEFT   67
     #define CANCEL 68
     #define ENTER  69

     //
     // status LEDs
     //

     #define DOT_LED_1 55
     #define DOT_LED_2 56
     #define DOT_LED_3 57
     #define DOT_LED_4 58

     #define CLOCK_LED     39
     #define CLOCK_1HZ_LED 41
     #define CARRY_LED     43
     #define ZERO_LED      45

     //
     // DOT digital output
     //

     #define DOT_1 1
     #define DOT_2 2
     #define DOT_3 3 // we need pin 4 for SD card!
     #define DOT_4 5

     //
     // DIN digital input
     //

     #define DIN_1 17
     #define DIN_2 16
     #define DIN_3 15
     #define DIN_4 14

     //
     // telephone keypad buttons for DIN input
     //

     #define DIN_BUTTON_1 42 // telephone keypad # 
     #define DIN_BUTTON_2 44 // telephone keypad 9 
     #define DIN_BUTTON_3 46 // telephone keypad 6
     #define DIN_BUTTON_4 48 // telephone keypad 3 

     //
     // remaining telephone keypad buttons
     //

     #define CCE  26 // telephone keypad *
     #define RUN  28 // telephone keypad 7
     #define BKP  30 // telephone keypad 4
     #define NEXT 32 // telephone keypad 1
     #define PGM  34 // telephone keypad 0 
     #define HALT 36 // telephone keypad 8 
     #define STEP 38 // telephone keypad 5
     #define REG  40 // telephone keypad 2 

     //
     // CPU speed throttle potentiometer
     //

     #define CPU_THROTTLE_ANALOG_PIN A0
     #define CPU_THROTTLE_DIVISOR 10   // potentiometer dependent 
     #define CPU_MIN_THRESHOLD 5       // if smaller than this, CPU = max speed  
     #define CPU_MAX_THRESHOLD 99      // if higher than this, CPU = min speed 
     #define CPU_DELTA_DISP 3          // if analog value changes more than this, update CPU delay display 

     //
     // for initialization of random generator
     //

     #define RANDOM_ANALOG_PIN A5

     //
     // LCD panel pins
     //

     LiquidCrystal lcd(13, 12, 11, 7, 9, 8); // we need pin 10 for SD card!

     //
     // 2 Adafruit 7Segment LED backpacks
     //

     Adafruit_7segment right = Adafruit_7segment();
     Adafruit_7segment left  = Adafruit_7segment();

     //
     // HEX 4x4 matrix keypad
     //

     #define ROWS 4
     #define COLS 4

     char keys[ROWS][COLS] = { // plus one because 0 = no key pressed!
     {0xD, 0xE, 0xF, 0x10},
     {0x9, 0xA, 0xB, 0xC},
     {0x5, 0x6, 0x7, 0x8},
     {0x1, 0x2, 0x3, 0x4}
     };

     byte colPins[COLS] = {37, 35, 33, 31}; // columns
     byte rowPins[ROWS] = {29, 27, 25, 23}; // rows

     Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

This picture might provide some ideas how to set up / wire the hardware: 

![Busch 2090 Microtronic Emulator for Arduino Mega 2560 Version 2](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v2-3-small.jpg)

Also notice that there is a blueprint of the faceplate in the 
``faceplate/`` directory of this project. 

###Description 

For the Arduino Uno and Mega Version 1, the TM1638 module is used. 
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
and ``D26`` on the Arduino Mega version 1) are read by the Microtronic
data in op-code ``FDx (DIN)``. Connecting them to ground will set the
corresponding bit to 1. See ``PGM D``.

Analog pin ``A5`` on the Uno (or ``A15`` on the Mega version 1) is
used as a CPU speed throttle. Connect a potentiometer to adjust the
speed of the CPU:

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

The Mega version 1 uses the select button of the LCD+Keypad shield to
toggle between PC + current op-code display, register display,
extra-register display, and display off. Note that the emulator slows
down considerably with LCD being on.

![Program Counter and Op-Code Display](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-6-small.jpg)
![Register Content Display](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-7-small.jpg)

For the Mega Version 2, I have used a telephone keypad for the function buttons:

    #define CCE  26 // telephone keypad *
    #define RUN  28 // telephone keypad 7
    #define BKP  30 // telephone keypad 4
    #define NEXT 32 // telephone keypad 1
    #define PGM  34 // telephone keypad 0 
    #define HALT 36 // telephone keypad 8 
    #define STEP 38 // telephone keypad 5
    #define REG  40 // telephone keypad 2 

The remaining 4 buttons can be used to provide digital input to
``DIN`` command; in addition, there are also real digital inputs
reserved for hardware experiments:

   #define DIN_BUTTON_1 42 // telephone keypad # 
   #define DIN_BUTTON_2 44 // telephone keypad 9 
   #define DIN_BUTTON_3 46 // telephone keypad 6
   #define DIN_BUTTON_4 48 // telephone keypad 3 

   #define DIN_1 17
   #define DIN_2 16
   #define DIN_3 15
   #define DIN_4 14

The ``DOT`` output LEDs are discrete LEDs, and so are carry, zero, 1 hz clock, 
and CPU clock: 

   #define DOT_LED_1 55
   #define DOT_LED_2 56
   #define DOT_LED_3 57
   #define DOT_LED_4 58

   #define CLOCK_LED     39
   #define CLOCK_1HZ_LED 41
   #define CARRY_LED     43
   #define ZERO_LED      45

For hardware experiments, there are additional outputs for ``DOT`` as
well:

   #define DOT_1 1
   #define DOT_2 2
   #define DOT_3 3 // we need pin 4 for SD card!
   #define DOT_4 5

Since there is no LCD+Keypad shield being used, there are discrete
N.O. buttons that take on these functions (SD card):

    #define BACK   63
    #define RIGHT  64
    #define UP     65
    #define DOWN   66
    #define LEFT   67
    #define CANCEL 68
    #define ENTER  69

  


###Load and Save Files to SDCard (Mega version only) 

The Mega version supports saving a memory dump to SDCard via ``PGM
2``.  The LCD+Keypad shield offers a primitive file name editor. Use
``Select`` key to confirm current file name; ``Left`` and ``Right``
keys to move cursor, ``Up`` and ``Down`` keys to change character at
cursor position. 

On the mega version 2, there is no keypad, but discrete N.O. buttons
(see above) take on the functions for SD card operations. 

Files are loaded from SDCard via ``PGM 1``. Here, the LCD+Keypad
shield is used to browse through the directory of files. Use ``Select``
key to confirm selection, and ``Left`` key to abort loading.

![Load Program from SDCard](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-2-small.jpg)
![Save Program to SDCard](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-8-small.jpg)

###The ``.MIC`` File Format and Example Programs 

The ``sofware`` subdirectory contains some programs from the
Microtronic manuals and from the book "Computer Games (2094)". With
the Mega version, you can put these files on a FAT16 formatted SDCard,
and load them via ``PGM1``. Please refer to the original manuals on
how to use these programs.

In ``.MIC`` file, in addition to hexadecimal instructions, you can
find comments (a line starting with ``#``), as well as the origin
address instruction ``@ xx``. This means that the instructions
following ``@ xx`` will be stored from address ``xx`` on. Most of the
time, you will find ``@ 00`` at the beginning of the file. If a
``.MIC`` file does not contain a ``@ xx``, then the program will be
loaded at the current PC. That way, programs could be relocatable
(e.g., for subroutines). Also, a ``.MIC`` can contain more than one ``@
xx``. An example is the ``DAYS.MIC`` program. 

Note that there are a couple of programs in the Manual Vol. 2 which
require incremental loading, i.e., first load ``DAYS.MIC``, and then
additionally load ``WEEKDAY.MIC``. The programs automatically load at
the correct addresses.

The example programs in the ``sofware`` subdirectory have been
automatically converted from the above linked PDFs with the help of an
OCR (Optical Character Recognition) program, so they may contain some
strange characters and OCR artifacts and errors. Not all programs have
been tested by the author yet. Programs which have been successfully
tested contain a ``# tested`` comment.  To compensate for OCR
artifacts, the ``.MIC`` loader recognizes an extended character set
for hexadecimal input, e.g., not only 1, but also I and l are accepted
for 1, the O character is accepted for 0, etc.

###``PGM`` Demo Programs are Stored in EEPROM  

Please first run the ``PGM-EEPROM.ino`` sketch. This will load 5
example programs into the Arduino's EEPROM. The emulator won't work /
won't initialize correctly if the EEPROM has not been prepared.  The
programs stored into and loaded from the EEPROM are ``PGM 7`` to ``PGM
B``:

- ``PGM 0`` : self-test not yet implemented. 
- ``PGM 1`` : on Mega, this loads a program memory dump from SDCard. Use LCD+Keypad shield to select file via ``Select`` key, ``Left`` key to abort loading. 
- ``PGM 2`` : on Mega, this saves a complete memory dump to SDCard (``.MIC`` file extension is used). The LCD+Keypad shield offers a simple filename editor. Use ``Left`` and ``Right`` to change cursor position, ``Up`` and ``Down`` to change character at cursor position, and ``Select`` to confirm and save. The files are stored as text. You can also use a simple text editor to create files (CR+LF end of line coding), and load them back via ``PGM 2``. Whitespace is ignored. 
- ``PGM 3`` : set time / clock (not a real program, i.e., nothing is loaded into program memory for this function) 
- ``PGM 4`` : show time / clock  (not a real program, i.e., nothing is loaded into program memory for this function) 
- ``PGM 5`` : clear memory
- ``PGM 6`` : load ``F01`` (NOPs) into memory
- ``PGM 7`` : the Nim game as documented in the Microtronic Manual Vol. 1, page 7. Many thanks to Martin Sauter for
retrieving the code from an original Busch Microtronic and contributing it to this project! 
- ``PGM 8`` : crazy counter
- ``PGM 9`` : the electronic die, from Microtronic Manual Vol. 1, page 10
- ``PGM A`` : the three digit counter from Microtronic Manual Vol. 1, page 19 
- ``PGM B`` : moving LED light from Manul Vol. 1, page 48 
- ``PGM C`` : digitial input ``DIN`` test

###Required Third-Party Libraries 

The emulator requires the following libraries, which are the work of
others, and which are included in the ``library`` subdirectory: 

- ``Keypad`` library
- ``TM1638`` library - note that this is a modified version of the original one 
- ``TM16XXFonts`` for alphanumeric 7segment display fonts
- ``EEPROM`` library 

For the Mega version one, the following standard libraries are used, and
already part of the Arduino distribution (version 1.6.6):

- ``LiquidCrystal`` library
- ``SPI`` library
- ``SD`` library

For the Mega version two, the following standard libraries are used, and
already part of the Arduino distribution (version 1.6.6):

- ``LiquidCrystal`` library
- ``SPI`` library
- ``SD`` library

These libraries are available from Adafruit homepage: 
- ``Adafruit_LEDBackpack``library
- ``<Adafruit_GFX`` library

### Future Work 

1. Add drivers to the DOT output LEDs such that they can be used as
output pins, like in the real Microtronic (or use a additional digital
outputs for this - would work for Mega version only, though). This
might also require a simple transistor or Darlington driver.  
2. With 1. done, control a Speech Synthesizer from these ports. A Speech
Synthesizer extension board was announced as early as 1983 by Busch,
in the Microtronic Manual Vol. 1, but was never released.
3. Implement ``BKP`` and ``STEP`` function keys (breakpoint and
step). I did not really use them a lot in 1983.


