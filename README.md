# Arduino-Based Busch 2090 Microtronic Emulators 
## Arduino-based Emulators of the Vintage Busch 2090 Microtronic Computer System from 1981 
#### Authors: 
- [Michael Wessel](http://www.michael-wessel.info/): Original Emulator & Next Generation Emulator 
- Frank de Jaeger: 2nd Generation Microtronic PCB 
- Manfred Henf: 2nd Generation Microtronic 3D Design & Printing 
- Martin Sauter: Busch 2095 Cassette Interface Protocol Reengineering & Research 
#### License: GPL 3
#### [YouTube Videos](https://www.youtube.com/playlist?list=PLvdXKcHrGqhekyx81EoCwQij1Lqylp0dB) 

## Abstract

This repository contains a number of Arduino-based emulator of the 
Busch 2090 Microtronic Computer System. 

The latest PCB version - "The Microtronic Next Generation" - also povides
an emulation of the Busch 2095 Cassette Interface that plugs into
a real Microtronic to provide SD-card based file storage. 

Recently, the latest iteration of this project, "The Microtronic Next
Generation", was featured on the Hackaday front page:

![Hackaday 1](./hackaday/microtronic-hackaday-1.jpg) 
![Hackaday 2](./hackaday/microtronic-hackaday-2.jpg) 
![Hackaday 3](./hackaday/microtronic-hackaday-3.jpg) 

## History 

The Busch 2090 was an educational 4bit single-board computer system of
the early 1980s, manufactured by the company Busch Modellbau in
Germany. There is [some information about the Busch 2090 Microtronic
available here, including PDFs of the original manuals in
German](https://www.busch-model.info/service/historie-microtronic/)

The designer of the original Busch Microtronic, Mr. Jörg Vallen of Busch, 
was also so kind to grant permission to include a full copy of the
manual set in the [`manuals`](./manuals/) directory of this project. 

## Emulator Versions 

### The "Microtronic Next Generation" Project 

The current version of the Microtronic Emulator is called the "Micotronic Next Generation", and it comes as a PCB: 

![Busch 2090 Microtronic Next Generation - SH1106 SPI OLED Version](./microtronic-nextgen-sh1106-spi/nextgen-spi-5.jpg) 

![Busch 2090 Microtronic Next Generation - SH1106 SPI OLED Version](./microtronic-nextgen-sh1106-spi/nextgen-spi-1.jpg) 

![Busch 2090 Microtronic Next Generation - Nokia Version Front](./microtronic-nextgen-nokia/pcb1.jpg) 

![Busch 2090 Microtronic Next Generation - Nokia Version Back](./microtronic-nextgen-nokia/pcb2.jpg) 

![Busch 2090 Microtronic Next Generation - Nokia Version](./microtronic-nextgen-nokia/pcb1.jpg) 

![Busch 2090 Microtronic Next Generation - 2095 Emulation](./microtronic-nextgen-nokia/2095-1.jpg) 

![Busch 2090 Microtronic Next Generation - 2095 Emulation](./microtronic-nextgen-nokia/2095-2.jpg) 

### The "Microtronic 2nd Generation" Sister Project  

The "sister project", created by **Frank de Jaeger from Belgium and Manfred Henf from Germany,** ist called the **"Microtronic 2nd Generation"**. Please consider this great project if you wish to create a more professional Microtronic emulator that neatly and professionally installs into an original Busch electronics console, including a 3D-printed keyboad that mounts onto the console hole raster on the top! 

The project uses the same firmware as the "Microtronic Next Generation" presented here, but has a slightly different pin layout (i.e., requires some slight adjustments to the `hardware.h` pin configuration file). The firmware can be found in the [`microtronic-2nd-generation`](./microtronic-2nd-generation/) folder. The 2nd Generation version uses the Nokia 5110, does not support the real time clock (RTC), and does not have a 1Hz output port. The loudspeaker can be
added between A0 and GND over a 75 Ohm resistor as an "after market"
hack / mod (it wasn't anticipated in the original 2nd Generation design). 

**Thanks to Frank and Manfred for this great piece of engineering.** They also *exactly replicated the input and output transistor-stages of the original Microtronic,* so the 2nd Generation project is electrically maximally compatible with the original. It is hence the best choice for a fully compatible  Microtronic emulator that blends perfectly with the Busch electronics system, and for conducting the plenty electronics experiments described in the original Microtronic Busch manuals. 

More details about this great project can be found on the [homepage of the Microtronic 2nd Generation project.](https://www.rigert.com/ee-forum/viewtopic.php?t=2497)

*The following pictures are courtesy of Frank de Jaeger:* 

![Microtronic 2nd Generation Pic 1](./microtronic-2nd-generation/pic1.jpg)
![Microtronic 2nd Generation Pic 2](./microtronic-2nd-generation/pic2.jpg)
![Microtronic 2nd Generation Pic 3](./microtronic-2nd-generation/pic3.jpg)
![Microtronic 2nd Generation Pic 4](./microtronic-2nd-generation/pic4.jpg)


### The 2021 Arduino Uno R3 Version  

This is a new take on the 2016 Arduino Uno R3 version with some improvements over the 2016 version. 

In a nutshell, it offers: 

- High-speed Microtronic emulation with an authentic retro user experience (LED 7segment display etc.)
- Extended PGM program library in AVR ``PGMSPACE``, e.g. Blackjack, Prime Numbers, Lunar Lander, and the Nim Game. Unlike previous versions of the R3 emulator, the PGM programs are now no longer stored in the AVR's EEPROM; hence, more and longer programs can be accessed with the push of a PGM button - more fun! Note that the ``PGM-EEPROM.INO`` loader is no longer required with that version and is considered obsolete by now. 
- PGM 2 & PGM 1 functionality: the EEPROM is now used to store & restore the Microtronic memory contents! Before powering down the emulator, simply dump the current memory contents into the EEPROM via ``PGM 2``, and resume your work with ``PGM 1``. Better than a 2095 Cassette Interface! 
- CPU Speed Control / Throttle: go turbo Microtronic (Prime Numbers have never been computed faster on a Microtronic!), or experience the cozy processing speed of the original Microtronic by tuning the emulation speed with this 10 kOhm potentiometer. 
- A simple build - you can set this up in 30 minutes.
- Still missing: ``BKP`` and ``STEP`` functionality. Soon - Lilly (Germany) is working on that (see below). 

![Busch 2090 Microtronic Emulator for Arduino Uno R3 2021 Version - Pic 1](https://github.com/lambdamikel/Busch-2090/blob/master/images/2090-2021-1.jpg)

![Busch 2090 Microtronic Emulator for Arduino Uno R3 2021 Version - Pic 2](https://github.com/lambdamikel/Busch-2090/blob/master/images/2090-2021-2.jpg)

![Busch 2090 Microtronic Emulator for Arduino Uno R3 2021 Version - Pic 3](https://github.com/lambdamikel/Busch-2090/blob/master/images/2090-2021-3.jpg)

**Thanks to Lilly (Germany) for pointing out that the R3 version was still compiling and working; this motivated me to take a fresh look at the project.** In fact, Lilly built her own 2090 R3 emulator: embracing the true hacker spirit, she used an old Agfa photo box and recycled it as the emulator case. I don't think she went dumpster diving for components though. **Great build, Lilly, and thanks for motivating the 2021 version and some new functionalities to make it more useful. This is her emulator, it certainly has the words "retro" and "vintage" written all over it:** 

![Lilly Pic 1](https://github.com/lambdamikel/Busch-2090/blob/master/images/lilly-1.jpg)

![Lilly Pic 2](https://github.com/lambdamikel/Busch-2090/blob/master/images/lilly-2.jpg)


#### Hardware Requirements

You will need: 

- An Arduino Uno R3 
- A TM1638 module with 8 7segment digits, 8 push buttons, and 8 LEDs 
- A 4x4 keypad with matrix encoding for hexadecimal input
- A 10k Ohm potentiometer for CPU speed / throttle control 

#### Hardware Setup / Wiring 

For the Uno version:

    //
    // TM1638 module
    //

    TM1638 module(14, 15, 16);

    //
    // Keypad 4 x 4 matrix
    //

    byte colPins[COLS] = {5, 6, 7, 8}; // columns
    byte rowPins[ROWS] = {9, 10, 11, 12}; // rows

    //
    // these are the digital input pins used for DIN instructions
    //

    #define DIN_PIN_1 1
    #define DIN_PIN_2 2
    #define DIN_PIN_3 3
    #define DIN_PIN_4 4

    //
    // these are the digital output pins used for DOT instructions
    // (in addition to the TM1638 LEDs)
    //

    #define DOT_PIN_1 13
    #define DOT_PIN_2 17
    #define DOT_PIN_3 18 
    // #define DOT_PIN_4 0 

    //
    // reset Microtronic (not Arduino) by pulling this to GND
    //

    #define RESET_PIN 0

    //
    // CPU throttle
    //

    #define CPU_THROTTLE_ANALOG_PIN 5 // center pin of 10k Ohm potentiometer
    #define CPU_THROTTLE_DIVISOR 10 
    #define CPU_MIN_THRESHOLD 10 // if smaller than this, CPU delay = 0

	
#### Description 

The TM1638 "Led & Key" module is being used.  The **eight push buttons
of the TM1638 are the function keys of the Microtronic**, in this
order of sequence, from left to right: ``HALT, NEXT, RUN, CCE, REG,
STEP, BKP, PGM``:

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

There are also three PINs for DOT outputs: ``13``, ``A3`` and ``A4``
are used for the first 3 bits of the DOT outputs. For the 4th bit, you
can use pin ``0``, but then the ``RESET`` button (see next paragraph)
will have to be sacrificed (due to a shortage of R3 pins).

Notice that the Arduino reset button will erase the emulator's program
memory. To only reset emulator while keeping the program in memory,
connect Arduino pin ``D0 (RX)`` to ground. You can always dump the memory
contents to the EEPROM via ``PGM 2``, and load it back via ``PGM 1``. 

The Arduino Uno pins ``D1`` to ``D4`` are read by the Microtronic data
in op-code ``FDx (DIN)``. Connecting them to ground will set the
corresponding bit to one (high). See ``PGM D``.

Analog pin ``A5`` on the Uno is used as a CPU speed throttle. Connect
a potentiometer to adjust the speed of the CPU.  **Important: All
three pins of the potentiometer need to be connected!  The center pin
of the potentiometer goes to ``A5`` (``CPU_THROTTLE_ANALOG_PIN``), and
the outer remaining two pins connect to ``5V (VCC)`` and ``GND``.**
Otherwise, the analog input is left "floating" and no analog value can
be read. The ``analogRead`` should return a value between 0 and 1023;
adjust the ``CPU_THROTTLE_DIVISOR 10`` if required. I am using a 1
kOhm potentiometer; don't use values smaller than 1 kOhm because of
the VCC -> GND current leakage over the potentiometer.

Unlike the original Microtronic, this emulator uses the leftmost digit
of the 8digit FM1638 to display the **current system status**; the
original Microtronic only featured a 6digit display. Currently, the
**status codes** are:

- ``H``: stopped 
- ``A``: enter address 
- ``P``: enter op-code 
- ``r``: running program
- ``?``: keypad input from user requested  
- ``i``: entering / inspecting register via ``REG``  
- ``t`` : entering clock time (``PGM 3``) 
- ``C`` : showing clock time (``PGM 4``) 

Moreover, unlike the original Microtronic, the emulator uses blinking
digits to indicate cursor position. The ``CCE`` key works a little bit
differently, but editing should be comfortable enough.

Typical operation sequences such as ``HALT-NEXT-00-RUN`` and
``HALT-NEXT-00-F10-NEXT-510-NEXT-C00-NEXT`` etc. will work as
expected.  Also, try to load a demo program: ``HALT-PGM-7-RUN``.

Note that programs can be entered manually, using the keypad and
function keys, or you can load a fixed ROM program specified in the
Arduino sketch via the ``PGM`` button. These ROM programs are defined
in the ``busch2090.ino`` sketch and are stored in the ``PGMSPACE``.
The ROM programs ``PGM 7`` to ``PGM F`` are defined:

The following PGM programs from ``PGM 7`` to ``PGM F`` are stored in
the sketch using ``PGMSPACE`` strings.  Note that ``PGM 1`` to ``PGM
6`` are built-in special functions that do not correspond to
``PGMSPACE`` programs. If you wish, you can exchange these ``7`` to
``F`` programs with you own:

- ``PGM 1`` : restore Microtronic memory from EEPROM ("core restore") 
- ``PGM 2`` : store / dump Microtronic memory to EEPROM  ("core dump") 
- ``PGM 3`` : set clock 
- ``PGM 4`` : show clock 
- ``PGM 5`` : clear memory
- ``PGM 6`` : load ``F01`` (NOPs) into memory 
- ``PGM 7`` : Nim Game 
- ``PGM 8`` : Crazy Counter 
- ``PGM 9`` : the Electronic Dice, from Microtronic Manual Vol. 1, page 10
- ``PGM A`` : the Three Digit Counter from Microtronic Manual Vol. 1, page 19 
- ``PGM B`` : moving LED Light from the Microtronic Manul Vol. 1, page 48  
- ``PGM C`` : digital input DIN Test Program
- ``PGM D`` : Lunar Lander (Moon Landing) from the Microtronic Manual Vol. 1, page 23 
- ``PGM E`` : Prime Numbers, from the "Computerspiele 2094" book, page 58
- ``PGM F`` : Game 17+4 BlackJack, from the "Computerspiele 2094" book, page 32

Have fun! 

## Current Arduino Sketches & Gerbers 

Please check the sub-directories
- [`microtronic-nextgen-nokia`](./microtronic-nextgen-nokia/) for the Nokia 5510 Display version, 
- [`microtronic-nextgen-sh1106-spi`](./microtronic-nextgen-sh1106-spi/) for the SH1106 SPI OLED version, 
- [`microtronic-2nd-generation`](./microtronic-2nd-generation/) for the Nokia 5110-based Microtronic 2nd Generation project, and 
- [`busch2090`](./busch2090/) for the 2021 Arduino Uno R3 version. 

The *SH1106 SPI OLED is the latest version* and we have identified this
display as the best option for the project. The Nokia 5510 is a good
choice too, but it is less reactive and gets blurry with very fast
screen updates. 

The 2nd Generation version does not support the real time clock (RTC),
and does not have a 1Hz output port. Note that the loudspeaker can be
added between A0 and GND over a 75 Ohm resistor (as an "after market"
hack / mod).

The SH1106 I2C SPI OLED was also experimented with; see
[`microtronic-nextgen-sh1106-i2c`](./obsolete/microtronic-nextgen-sh1106-i2c/).
It is **no longer supported** because it is **significantly slower than
both the Nokia 5110 and the SH1106 SPI OLED.**

Only the
[`microtronic-nextgen-sh1106-spi`](./microtronic-nextgen-sh1106-spi/)
and the [`busch2090`](./busch2090/) 2021 Arduino Uno R3 version will
be continued.  The PCB Gerbers will follow shortly.


## 2016 Versions - DEPRECATED and NOT RECOMMENDED 

**Please note that these version have note been tested recently (they
are from 2016, and Arduino has changed since then).  I strongly
recommend to use the "Micotronic Next Generation" version given above 
instead.**

![Busch 2090 Microtronic Emulator for Arduino Mega 2560 Version 3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v3-1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega 2560 Version 3](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v2-5-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Uno](https://github.com/lambdamikel/Busch-2090/blob/master/images/img4-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-1-small.jpg)

![Busch 2090 Microtronic Emulator for Arduino Mega](https://github.com/lambdamikel/Busch-2090/blob/master/images/img-mega-v1-6-small.jpg)

[More info about these 2016 Microtronic emulator projects can be found here.](./README.old.md) 

## Required Third-Party Libraries 

Please check the [`libraries.zip`](./libraries/libraries.zip) that contains everything needed (and more) for the different Arduino emulator versions discussed here. 