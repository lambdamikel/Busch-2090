/*

  A Busch 2090 Microtronic Emulator for Arduino Uno R3

  Version 1.8 (c) Michael Wessel, February 1st 2021 
  https://github.com/lambdamikel/Busch-2090
  
  With contributions from Lilly (Germany): 
  https://github.com/ducatimaus/Busch-2090 
  (STEP & BKP Functionality Integration) 

  michael_wessel@gmx.de
  miacwess@gmail.com
  https://www.michael-wessel.info

  Hardware requirements:
  - 4x4 hex keypad (HEX keypad for data and program entry)
  - TM1638 8 digit 7segment display with 8 LEDs and 8 buttons (function keys)
  - 10 kOhm Potentiometer for CPU Throttle / Speed Control 

  The Busch Microtronic 2090 is (C) Busch GmbH
  See http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <avr/pgmspace.h>

#include <TM1638.h>
#include <Keypad.h>
#include <TM16XXFonts.h>
#include <EEPROM.h>
#include <Wire.h>

#define EXT_EEPROM 0x50    //Address of 24LC256 eeprom chip

//
// Uncomment this if you want 4 DOT outputs instead of 3: 
//

// #define RESET_BUTTON_AT_PIN_0 

//
// Uncomment this if you want A5 CPU THROTTLE POTENTIOMETER; ELSE THIS IS USED FOR CLOCK 1HZ OUTPUT 
//

// #define USE_CPU_THROTTLE_POT_AT_PIN_A5 

//
// PGM ROM Programs - Adjust as you like!
//

// PGM 7 - NIM GAME
const char PGM7[] PROGMEM = "F08 FE0 F41 FF2 FF1 FF4 045 046 516 FF4 854 D19 904 E19 B3F F03 0D1 0E2 911 E15 C1A 902 D1A 1F0 FE0 F00 F02 064 10C 714 B3F 11A 10B C24 46A FBB 8AD E27 C29 8BE E2F 51C E2C C22 914 E2F C1C F03 0D1 0E2 F41 902 D09 911 E38 C09 1E2 1E3 1F5 FE5 105 FE5 C3A 01D 02E F04 64D FCE F07 ";

// PGM 8 - crazy counter
const char PGM8[] PROGMEM = "F60 510 521 532 543 554 565 FE0 C00 ";

// PGM 9 - electronic die
const char PGM9[] PROGMEM = "F05 90D E00 96D D00 F1D FF0 C00 ";

// PGM A - three digit counter with carry
const char PGMA[] PROGMEM = "F30 510 FB1 FB2 FE1 FE1 C00 ";

// PGM B - scrolling LED light
const char PGMB[] PROGMEM = "110 F10 FE0 FA0 FB0 C02 ";

// PGM C - DIN digital input test
const char PGMC[] PROGMEM = "F10 FD0 FE0 C00 ";

// PGM D - Lunar Lander
const char PGMD[] PROGMEM = "F02 F08 FE0 142 1F3 114 125 136 187 178 1A1 02D 03E 04F F03 F5D FFB F02 1B1 10F 05D 06E F03 F5D FFB F02 1C1 07D 08E F03 F5D FFB 10D 10E F2D FFB 99B D29 0DE 0BD C22 F02 10F F04 6D7 FC8 D69 6E8 D69 75D FCE D5A 4D2 FB3 FB4 4E3 FB4 652 FC3 FC4 D7B 663 FC4 D7B 6D5 FC6 D80 6E6 D80 904 D0A 903 D0A 952 D0A 906 D0A 955 D0A 1E0 1E1 1E2 1E3 1E4 1E5 1F6 FE6 F60 FF0 C00 6DF 6F2 FC3 FC4 D7B 652 FC3 FC4 D7B 663 FC4 D7B 4F5 FB6 C45 1E0 1A1 1E2 1A3 1FF 1FE FEF 71E D73 C70 F40 FEF 10D FED 71E D58 F02 C73 1A0 1A1 1A2 1A3 C6D 1F0 1A1 1F2 1A3 C6D ";

// PGM E - Primes
const char PGME[] PROGMEM = "F08 FEF F50 FFE FE5 9BE D09 E0C C00 9CE D02 C16 FFF 9AF EDA D18 034 023 012 001 0F0 C0C F02 B77 F02 B90 F0D F08 168 F0D F0F B77 F0F 904 D2A 903 D33 902 D36 901 D3C C3F 92A D2D C41 909 D30 C41 958 D3F C41 90A D3F C41 929 D39 C41 918 D3F C41 909 D3F C41 1FF C01 F70 F71 F72 F73 F74 680 D49 C4C 760 711 D4F 691 D50 C53 691 761 712 D56 6A2 D57 C5D 6A2 762 713 D58 C5D 763 714 900 D65 010 021 032 043 104 C5D 904 D46 903 D46 82A D1D E6D C46 819 D1D E71 C46 808 D1D E75 C46 F0D C17 510 990 D7B C90 560 511 991 D80 C90 561 512 992 D85 C90 562 513 993 D8A C90 563 514 994 D8F C90 564 910 EA9 920 E9D 930 E9D 970 EA9 990 EA9 950 E9D C77 904 DA6 903 DA6 902 DA6 901 DA6 CCF 930 EA9 C77 F70 F71 F72 F73 F74 410 101 10F DD2 990 DD2 420 102 11F DD2 990 DD2 430 103 12F DD2 990 DD2 440 104 13F DD2 990 DD2 901 DAE 930 ED0 960 ED0 990 ED0 F0D F07 F0D C77 560 511 92F DC4 EBE 90F DB8 CB2 F08 C0C ";

// PGM F - 17+4 Black Jack
const char PGMF[] PROGMEM = "F08 FE0 14A 1DB 1DC 1AD 17E 11F F6A FFF F02 B6C C12 F40 B6C FFF F02 E2C 80D E15 C1E 9A0 E18 C1E 902 E1B C1E 1A2 1A3 C5A 0D0 402 D24 992 D24 C26 513 562 923 E29 C2C 912 E5A D60 917 D33 E30 C36 966 D33 C36 90F E53 C0D 84E E39 C42 9A4 E3C C42 903 E3F C42 1A6 1A7 C60 0E4 446 D48 996 D48 C4A 566 517 927 E4F 90F E53 C0D 916 E60 D5A C4C 837 E57 D60 C5A 826 E60 D60 1DC 10D 10E 16F F4C C64 1DD 1AE 1BF F3D 1FB FEB FFB 104 FE4 F62 FF0 C00 F05 4FE 9AD D71 C73 57D C6E 9AE D76 F07 57E C73 ";

//
//
//

#define PROGRAMS 9

const char *const PGMROM[PROGRAMS] PROGMEM = {
    PGM7, PGM8, PGM9, PGMA, PGMB, PGMC, PGMD, PGME, PGMF};

byte programs = PROGRAMS;

byte program = 0;

//
// Set up the hardware
//

//
// TM1638 module
//

TM1638 module(14, 15, 16);

//
// Keypad 4 x 4 matrix
//

#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = { // plus one because 0 = no key pressed!
  {0xD, 0xE, 0xF, 0x10},
  {0x9, 0xA, 0xB, 0xC},
  {0x5, 0x6, 0x7, 0x8},
  {0x1, 0x2, 0x3, 0x4}
}; 

byte colPins[COLS] = {5, 6, 7, 8}; // columns
byte rowPins[ROWS] = {9, 10, 11, 12}; // rows

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned long lastFuncKeyTime = 0;

#define FUNCTION_KEY_DEBOUNCE_TIME 50

//
// these are the digital input pins used for DIN instructions
// Enable this if you want inverted inputs (INPUT_PULLUP vs. INPUT):
// Default is non-inverted inputs by now: 

#define INVERTED_INPUTS 

#define DIN_PIN_1 1
#define DIN_PIN_2 2
#define DIN_PIN_3 3
#define DIN_PIN_4 4

//
// these are the digital output pins used for DOT instructions
//

#define DOT_PIN_1 13
#define DOT_PIN_2 17
#define DOT_PIN_3 18
#define DOT_PIN_4 0 // only used if RESET_BUTTON_AT_PIN_0 is not defined!

#define CLOCK_1HZ A5 // only used if USE_CPU_THROTTLE_POT_AT_PIN_A5 is not defined!

//
// reset Microtronic (not Arduino) by pulling this to GND
// only used if RESET_BUTTON_AT_PIN_0 is defined!
//

#define RESET_PIN 0

//
// CPU throttle 
// only used if USE_CPU_THROTTLE_POT_AT_PIN_A5
// 

#define CPU_THROTTLE_ANALOG_PIN A5 // connect center pin of 10 KOhme potentiometer for CPU speed control; connect other 2 pins of potentiometer to GND and 5V!
#define CPU_THROTTLE_DIVISOR 10
#define CPU_MIN_THRESHOLD 10 // if smaller than this, CPU delay = 0

//
//
//

int cpu_delay = 12;

//                    0  1  2  3   4   5   6   7   8   9  10  11   12   13   14  15
int cpu_delays[16] = {0, 3, 6, 9, 12, 15, 18, 21, 30, 40, 50, 80, 120, 150, 200, 500 }; 

//
//
//

#define DISP_DELAY 400

//
// Microtronic 2090 function keys are mapped to the TM1638 buttons
// from left to right: HALT, NEXT, RUN, CCE, REG, STEP, BKP, PGM
//

#define HALT 1
#define NEXT 2
#define RUN 4
#define CCE 8
#define REG 16
#define STEP 32
#define BKP 64
#define PGM 128

//
// display and status variables
//

unsigned long lastDispTime = 0;
unsigned long lastDispTime2 = 0;

#define CURSOR_OFF 8
byte cursor = CURSOR_OFF;
boolean blink = true;

byte showingDisplayFromReg = 0;
byte showingDisplayDigits = 0;

byte currentReg = 0;
byte currentInputRegister = 0;

boolean clock = false;
boolean carry = false;
boolean zero = false;
boolean error = false;

byte moduleLEDs = 0;
byte outputs = 0;

//
// internal clock
//

byte timeSeconds1 = 0;
byte timeSeconds10 = 0;
byte timeMinutes1 = 0;
byte timeMinutes10 = 0;
byte timeHours1 = 0;
byte timeHours10 = 0;

//
// auxilary helper variables
//

unsigned long num = 0;
unsigned long num2 = 0;
unsigned long num3 = 0;

//
// RAM program memory
//

byte op[256];
byte arg1[256];
byte arg2[256];

boolean jump = false;
byte pc = 0;
byte breakAt = 0; // != 0 -> breakpoint set

boolean singleStep = false;
boolean ignoreBreakpointOnce = false;
boolean isDISP = false;

//
// Stack
//

#define STACK_DEPTH 5

byte stack[STACK_DEPTH];
byte sp = 0;

//
// Registers
//

byte reg[16];
byte regEx[16];

//
// keypad key and function key input
//

boolean keypadPressed = false;

byte input = 0;
byte keypadKey = NO_KEY;
byte keypadKeyRaw = NO_KEY;
byte functionKey = NO_KEY;
byte functionKeyRaw = NO_KEY;
byte previousFunctionKey = NO_KEY;
byte previousKeypadKey = NO_KEY;

//
// current mode / status of emulator
//

enum mode
{
  STOPPED,
  RESETTING,

  ENTERING_ADDRESS_HIGH,
  ENTERING_ADDRESS_LOW,

  ENTERING_BREAKPOINT_HIGH,
  ENTERING_BREAKPOINT_LOW,

  ENTERING_OP,
  ENTERING_ARG1,
  ENTERING_ARG2,

  RUNNING,
  STEPING,

  ENTERING_REG,
  INSPECTING,

  ENTERING_VALUE,
  ENTERING_PROGRAM,

  ENTERING_TIME,
  SHOWING_TIME

};

mode currentMode = STOPPED;

//
// OP codes
//

#define OP_MOV 0x0
#define OP_MOVI 0x1
#define OP_AND 0x2
#define OP_ANDI 0x3
#define OP_ADD 0x4
#define OP_ADDI 0x5
#define OP_SUB 0x6
#define OP_SUBI 0x7
#define OP_CMP 0x8
#define OP_CMPI 0x9
#define OP_OR 0xA

#define OP_CALL 0xB
#define OP_GOTO 0xC
#define OP_BRC 0xD
#define OP_BRZ 0xE

#define OP_MAS 0xF7
#define OP_INV 0xF8
#define OP_SHR 0xF9
#define OP_SHL 0xFA
#define OP_ADC 0xFB
#define OP_SUBC 0xFC

#define OP_DIN 0xFD
#define OP_DOT 0xFE
#define OP_KIN 0xFF

#define OP_HALT 0xF00
#define OP_NOP 0xF01
#define OP_DISOUT 0xF02
#define OP_HXDZ 0xF03
#define OP_DZHX 0xF04
#define OP_RND 0xF05
#define OP_TIME 0xF06
#define OP_RET 0xF07
#define OP_CLEAR 0xF08
#define OP_STC 0xF09
#define OP_RSC 0xF0A
#define OP_MULT 0xF0B
#define OP_DIV 0xF0C
#define OP_EXRL 0xF0D
#define OP_EXRM 0xF0E
#define OP_EXRA 0xF0F

#define OP_DISP 0xF

//
// External 24LC512 EEPROM 
//

void init_EEPROM(void)
{
  Wire.begin();  
}

void end_EEPROM(void)
{
  Wire.end();  

  pinMode(DOT_PIN_3, OUTPUT); // DOT 3

  #ifndef USE_CPU_THROTTLE_POT_AT_PIN_A5 
  pinMode(CLOCK_1HZ, OUTPUT); // CLOCK
  #endif 

  initializeClock(); 

}
  
void writeEEPROM(unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(EXT_EEPROM);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
 
byte readEEPROM(unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
 
  Wire.beginTransmission(EXT_EEPROM);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EXT_EEPROM,1);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}


//
// setup Arduino
//

void setup()
{

  // Careful! Serial logging makes DIN input PIN 1 unusable! 
  // Serial.begin(9600);

  randomSeed(analogRead(0));

  #ifdef RESET_BUTTON_AT_PIN_0 
  pinMode(RESET_PIN, INPUT_PULLUP);
  #endif

  #ifdef INVERTED_INPUTS 
  pinMode(DIN_PIN_1, INPUT_PULLUP); // DIN 1
  pinMode(DIN_PIN_2, INPUT_PULLUP); // DIN 2
  pinMode(DIN_PIN_3, INPUT_PULLUP); // DIN 3
  pinMode(DIN_PIN_4, INPUT_PULLUP); // DIN 4
  #endif 

  #ifndef INVERTED_INPUTS 
  pinMode(DIN_PIN_1, INPUT); // DIN 1
  pinMode(DIN_PIN_2, INPUT); // DIN 2
  pinMode(DIN_PIN_3, INPUT); // DIN 3
  pinMode(DIN_PIN_4, INPUT); // DIN 4
  #endif 

  //
  // DOT Outputs
  //

  pinMode(DOT_PIN_1, OUTPUT); // DOT 1
  pinMode(DOT_PIN_2, OUTPUT); // DOT 2
  pinMode(DOT_PIN_3, OUTPUT); // DOT 3

  #ifndef RESET_BUTTON_AT_PIN_0 
  pinMode(DOT_PIN_4, OUTPUT); // DOT 4
  #endif 
  
  //
  // 1 Hz Clock Output if not used for CPU Throttle Potentiometer 
  // 

  #ifndef USE_CPU_THROTTLE_POT_AT_PIN_A5 
  pinMode(CLOCK_1HZ, OUTPUT); // CLOCK
  #endif 

  //
  //
  //

  sendString(" Micro- ");
  delay(200);
  sendString(" tronic ");
  delay(200);
  sendString(" UNO R3 ");
  delay(200);
  sendString(" V 2021 ");
  delay(200);
  sendString("  ready ");
  delay(200);

  //
  //
  //

  initializeClock();
}

//
//
//

void sendString(String string)
{

  for (int i = 0; i < 8; i++)
  {
    module.sendChar(i, FONT_DEFAULT[string[i] - 32], false);
  }

  delay(DISP_DELAY);
}

void showMem()
{

  int adr = pc;

  if (currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW)
    adr = breakAt;

  module.sendChar(1, 0, false);

  if (cursor == 0)
    module.sendChar(2, blink ? NUMBER_FONT[adr / 16] : 0, true);
  else
    module.sendChar(2, NUMBER_FONT[adr / 16], false);

  if (cursor == 1)
    module.sendChar(3, blink ? NUMBER_FONT[adr % 16] : 0, true);
  else
    module.sendChar(3, NUMBER_FONT[adr % 16], false);

  module.sendChar(4, 0, false);

  if (cursor == 2)
    module.sendChar(5, blink ? NUMBER_FONT[op[adr]] : 0, true);
  else
    module.sendChar(5, NUMBER_FONT[op[adr]], false);

  if (cursor == 3)
    module.sendChar(6, blink ? NUMBER_FONT[arg1[adr]] : 0, true);
  else
    module.sendChar(6, NUMBER_FONT[arg1[adr]], false);

  if (cursor == 4)
    module.sendChar(7, blink ? NUMBER_FONT[arg2[adr]] : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[arg2[adr]], false);
}

//
// 1 Hz Clock Timer
//

void initializeClock()
{

  // Timer1 interrupt at 1Hz

  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 15624 / 2;

  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect)
{

  clock = !clock;

  if (clock)
  {

    timeSeconds1++;

    if (timeSeconds1 > 9)
    {
      timeSeconds10++;
      timeSeconds1 = 0;

      if (timeSeconds10 > 5)
      {
        timeMinutes1++;
        timeSeconds10 = 0;

        if (timeMinutes1 > 9)
        {
          timeMinutes10++;
          timeMinutes1 = 0;

          if (timeMinutes10 > 5)
          {
            timeHours1++;
            timeMinutes10 = 0;

            if (timeHours10 < 2)
            {
              if (timeHours1 > 9)
              {
                timeHours1 = 0;
                timeHours10++;
              }
            }
            else if (timeHours10 == 2)
            {
              if (timeHours1 > 3)
              {
                timeHours1 = 0;
                timeHours10 = 0;
              }
            }
          }
        }
      }
    }
  }
}

//
//
//

void showTime()
{

  module.sendChar(1, 0, false);

  if (cursor == 0)
    module.sendChar(2, blink ? NUMBER_FONT[timeHours10] : 0, true);
  else
    module.sendChar(2, NUMBER_FONT[timeHours10], false);

  if (cursor == 1)
    module.sendChar(3, blink ? NUMBER_FONT[timeHours1] : 0, true);
  else
    module.sendChar(3, NUMBER_FONT[timeHours1], false);

  if (cursor == 2)
    module.sendChar(4, blink ? NUMBER_FONT[timeMinutes10] : 0, true);
  else
    module.sendChar(4, NUMBER_FONT[timeMinutes10], false);

  if (cursor == 3)
    module.sendChar(5, blink ? NUMBER_FONT[timeMinutes1] : 0, true);
  else
    module.sendChar(5, NUMBER_FONT[timeMinutes1], false);

  if (cursor == 4)
    module.sendChar(6, blink ? NUMBER_FONT[timeSeconds10] : 0, true);
  else
    module.sendChar(6, NUMBER_FONT[timeSeconds10], false);

  if (cursor == 5)
    module.sendChar(7, blink ? NUMBER_FONT[timeSeconds1] : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[timeSeconds1], false);
}

void showReg()
{

  module.sendChar(1, 0, false);
  module.sendChar(2, 0, false);

  if (cursor == 0)
    module.sendChar(3, blink ? NUMBER_FONT[currentReg] : 0, true);
  else
    module.sendChar(3, NUMBER_FONT[currentReg], false);

  module.sendChar(4, 0, false);
  module.sendChar(5, 0, false);
  module.sendChar(6, 0, false);

  if (cursor == 1)
    module.sendChar(7, blink ? NUMBER_FONT[reg[currentReg]] : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[reg[currentReg]], false);
}

void showProgram()
{

  displayOff();
  module.sendChar(7, blink ? NUMBER_FONT[program] : 0, true);
}

void showError()
{

  displayOff();
  if (blink)
    sendString("  Error  ");
}

void showReset()
{

  displayOff();
  sendString("  reset ");
}

void displayOff()
{

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  for (int i = 0; i < 8; i++)
    module.sendChar(i, 0, false);
}

void showDisplay()
{

  for (int i = 0; i < showingDisplayDigits; i++)
    module.sendChar(7 - i, NUMBER_FONT[reg[(i + showingDisplayFromReg) % 16]], false);
}

void displayStatus()
{

  unsigned long time = millis();
  unsigned long delta2 = time - lastDispTime2;

  moduleLEDs = 0;

  if (delta2 > 300)
  {
    blink = !blink;
    lastDispTime2 = time;
  }

  if (carry)
    moduleLEDs = 1;

  if (zero)
    moduleLEDs |= 2;

  if (clock)
    moduleLEDs |= 4;

  char status = ' ';

  if (currentMode == STOPPED && !error)
    status = 'H';
  else if (currentMode ==
               ENTERING_ADDRESS_HIGH ||
           currentMode ==
               ENTERING_ADDRESS_LOW)
    status = 'A';
  else if (currentMode ==
               ENTERING_BREAKPOINT_HIGH ||
           currentMode ==
               ENTERING_BREAKPOINT_LOW)
    status = 'b';
  else if (currentMode == ENTERING_OP ||
           currentMode == ENTERING_ARG1 ||
           currentMode == ENTERING_ARG2)
    status = 'P';
  else if (currentMode == RUNNING)
    status = 'r';
  else if (currentMode == ENTERING_REG ||
           currentMode == INSPECTING)
    status = 'i';
  else if (currentMode == ENTERING_VALUE)
    status = '?';
  else if (currentMode == ENTERING_TIME)
    status = 't';
  else if (currentMode == SHOWING_TIME)
    status = 'C';
  else
    status = ' ';

  module.sendChar(0, FONT_DEFAULT[status - 32], false);

  moduleLEDs |= (outputs << 4);

  module.setLEDs(moduleLEDs);

  digitalWrite(DOT_PIN_1, outputs & 1);
  digitalWrite(DOT_PIN_2, outputs & 2);
  digitalWrite(DOT_PIN_3, outputs & 4);
  #ifndef RESET_BUTTON_AT_PIN_0 
  digitalWrite(DOT_PIN_4, outputs & 8);
  #endif 
 
  digitalWrite(CLOCK_1HZ, clock);

  if ( currentMode == RUNNING || currentMode == ENTERING_VALUE )
    showDisplay();
  else if ( currentMode == ENTERING_REG || currentMode == INSPECTING )
    showReg();
  else if (currentMode == ENTERING_PROGRAM)
    showProgram();
  else if (currentMode == ENTERING_TIME || currentMode == SHOWING_TIME)
    showTime();
  else if (error)
    showError();
  else if ( ! singleStep  || ! isDISP ) {     
    showMem();
  }

}

byte decodeHex(char c)
{

  if (c >= 65 && c <= 70)
    return c - 65 + 10;
  else if (c >= 48 && c <= 67)
    return c - 48;
  else
    return -1;
}

void enterProgram(int pgm, int start)
{

  cursor = CURSOR_OFF;
  int origin = start;

  char *pgm_string = (char *)pgm_read_word(&PGMROM[pgm]);

  while (pgm_read_byte(pgm_string))
  {

    op[start] = decodeHex(pgm_read_byte(pgm_string++));
    arg1[start] = decodeHex(pgm_read_byte(pgm_string++));
    arg2[start] = decodeHex(pgm_read_byte(pgm_string++));

    pc = start;
    start++;

    // currentMode = STOPPED;
    //outputs = pc;
    //displayOff();
    //displayStatus();

    showMem(); 
    delay(5);

    if (start == 256)
    {
      exit(1);
    }

    pgm_string++; // skip over space
  };

  pc = origin;
  currentMode = STOPPED;

  outputs = 0;
}

void clearStack()
{
  sp = 0;
}

void reset()
{

  showReset();
  delay(250);

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  for (currentReg = 0; currentReg < 16; currentReg++)
  {
    reg[currentReg] = 0;
    regEx[currentReg] = 0;
  }

  currentReg = 0;
  currentInputRegister = 0;

  carry = false;
  zero = false;
  error = false;
  pc = 0;
  singleStep = false;

  clearStack();

  outputs = 0;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;
}

void clearMem()
{

  cursor = 0;

  for (pc = 0; pc < 255; pc++)
  {
    op[pc] = 0;
    arg1[pc] = 0;
    arg2[pc] = 0;
    //outputs = pc;
    //displayStatus();
    showMem(); 
    delay(5); 
  }
  op[255] = 0;
  arg1[255] = 0;
  arg2[255] = 0;

  pc = 0;
  outputs = 0;
}

void loadNOPs()
{

  cursor = 0;

  for (pc = 0; pc < 255; pc++)
  {
    op[pc] = 15;
    arg1[pc] = 0;
    arg2[pc] = 1;
    //outputs = pc;
    //displayStatus();
    showMem(); 
    delay(5); 
  }
  op[255] = 15;
  arg1[255] = 0;
  arg2[255] = 1;

  pc = 0;
  outputs = 0;
}

void loadCore()
{

  sendString(" PGM1   ");
  sendString(" CORE   ");
  sendString(" LOAD?  ");

  unsigned int keypadKey = 0; 
  unsigned int keypadKey16 = 0; 

  do {
    keypadKey16 = keypad.getKey();	
  } while (keypadKey16 == NO_KEY); 

  keypadKey16--; 
  sendString(" LOAD   ");
  module.sendChar(6, NUMBER_FONT[keypadKey16], false);

  delay(100); 

  do {
    keypadKey = keypad.getKey();	
  } while (keypadKey == NO_KEY); 

  keypadKey--; 
  module.sendChar(7, NUMBER_FONT[keypadKey], false);

  // 256 memory addresses + 2 signature bytes (program slot) 
  unsigned int adr = ( keypadKey + keypadKey16 * 16 ) * ( 3 * 256 + 2);  

  delay(DISP_DELAY*2);  

  init_EEPROM(); 

  if (readEEPROM(adr++) == keypadKey &&
      readEEPROM(adr++) == keypadKey16 )
  {

    for (int i = 0; i < 256; i++)
    {
      op[i] = readEEPROM(adr++);
      arg1[i] = readEEPROM(adr++);
      arg2[i] = readEEPROM(adr++);

      pc = i; 
      showMem();
    }

    sendString(" LOADED ");
  }
  else
  {

    error = true;
  }

  displayOff();
  delay(DISP_DELAY);

  end_EEPROM(); 

}

void saveCore()
{

  sendString(" PGM2   ");
  sendString(" CORE   ");
  sendString(" DUMP?  ");

  unsigned int keypadKey = 0; 
  unsigned int keypadKey16 = 0; 

  do {
    keypadKey16 = keypad.getKey();	
  } while (keypadKey16 == NO_KEY); 

  keypadKey16--; 
  sendString(" DUMP   ");
  module.sendChar(6, NUMBER_FONT[keypadKey16], false);

  delay(100); 

  do {
    keypadKey = keypad.getKey();	
  } while (keypadKey == NO_KEY); 

  keypadKey--; 
  module.sendChar(7, NUMBER_FONT[keypadKey], false);

  // 256 memory addresses + 2 signature bytes (program slot) 
  unsigned int adr = ( keypadKey + keypadKey16 * 16 ) * ( 3 * 256 + 2);  

  delay(DISP_DELAY*2);  

  init_EEPROM(); 

  writeEEPROM(adr++, keypadKey);
  writeEEPROM(adr++, keypadKey16);

  for (int i = 0; i < 256; i++)
  {
    writeEEPROM(adr++, op[i]);
    writeEEPROM(adr++, arg1[i]);
    writeEEPROM(adr++, arg2[i]);

    pc = i; 
    showMem();
  };

  sendString(" DUMPED ");
  displayOff();
  delay(DISP_DELAY);

  end_EEPROM(); 
}

/* 
void loadCore()
{

  sendString("  PGM1  ");
  sendString("  CORE  ");
  sendString("  LOAD  ");

  int adr = 0;

  if (EEPROM.read(adr++) == 1 &&
      EEPROM.read(adr++) == 255)
  {

    for (int i = 0; i < 256; i++)
    {
      module.setLEDs(i % 16);
      op[i] = EEPROM.read(adr++);
      arg1[i] = EEPROM.read(adr++);
      arg2[i] = EEPROM.read(adr++);
    }

    sendString(" LOADED ");
  }
  else
  {

    error = true;
  }

  displayOff();
  delay(DISP_DELAY);
}

void saveCore()
{

  sendString("  PGM2  ");
  sendString("  CORE  ");
  sendString("  DUMP  ");

  int adr = 0;

  EEPROM.write(adr++, 1);
  EEPROM.write(adr++, 255);

  for (int i = 0; i < 256; i++)
  {
    module.setLEDs(i % 16);
    EEPROM.write(adr++, op[i]);
    EEPROM.write(adr++, arg1[i]);
    EEPROM.write(adr++, arg2[i]);
  };

  sendString(" DUMPED ");
  displayOff();
  delay(DISP_DELAY);
}
*/

void interpret()
{

  switch (functionKey)
  {

  case HALT:
    currentMode = STOPPED;
    cursor = CURSOR_OFF;

    showMem();

    break;

  case RUN:

    if (currentMode != RUNNING) {
      currentMode = RUNNING;
      displayOff();
      singleStep = false;
      ignoreBreakpointOnce = true;
      clearStack();
      jump = true; // don't increment PC !
    }

    break;

  case NEXT:
    if (currentMode == STOPPED)
    {
      currentMode = ENTERING_ADDRESS_HIGH;
      cursor = 0;
    }
    else
    {
      pc++;
      cursor = 2;
      currentMode = ENTERING_OP;
    }

    break;

  case REG:

    if (currentMode != ENTERING_REG)
    {
      currentMode = ENTERING_REG;
      cursor = 0;
    }
    else
    {
      currentMode = INSPECTING;
      cursor = 1;
    }

    break;

  case STEP:

    currentMode = RUNNING;
    
    //  if (! singleStep) 
    // displayStatus();

    singleStep = true;
    jump = true; // don't increment PC !

    break;

  case BKP:

    if (currentMode != ENTERING_BREAKPOINT_LOW)
    {
      currentMode = ENTERING_BREAKPOINT_HIGH;
      cursor = 0;
    }
    else
    {
      cursor = 1;
      currentMode = ENTERING_BREAKPOINT_LOW;
    }

    break;

  case CCE:

    if (cursor == 2)
    {
      cursor = 4;
      arg2[pc] = 0;
      currentMode = ENTERING_ARG2;
    }
    else if (cursor == 3)
    {
      cursor = 2;
      op[pc] = 0;
      currentMode = ENTERING_OP;
    }
    else
    {
      cursor = 3;
      arg1[pc] = 0;
      currentMode = ENTERING_ARG1;
    }

    break;

  case PGM:

    if (currentMode != ENTERING_PROGRAM)
    {
      cursor = 0;
      currentMode = ENTERING_PROGRAM;
    }

    break;
  }

  //
  //
  //

  switch (currentMode)
  {

  case STOPPED:
    cursor = CURSOR_OFF;
    break;

  case ENTERING_VALUE:

    if (keypadPressed)
    {
      input = keypadKey;
      reg[currentInputRegister] = input;
      carry = false;
      zero = reg[currentInputRegister] == 0;
      currentMode = RUNNING;
    }

    break;

  case ENTERING_TIME:

    if (keypadPressed)
    {

      input = keypadKey;
      switch (cursor)
      {
      case 0:
        if (input < 3)
        {
          timeHours10 = input;
          cursor++;
        }
        break;
      case 1:
        if (timeHours10 == 2 && input < 4 || timeHours10 < 2 && input < 10)
        {
          timeHours1 = input;
          cursor++;
        }
        break;
      case 2:
        if (input < 6)
        {
          timeMinutes10 = input;
          cursor++;
        }
        break;
      case 3:
        if (input < 10)
        {
          timeMinutes1 = input;
          cursor++;
        }
        break;
      case 4:
        if (input < 6)
        {
          timeSeconds10 = input;
          cursor++;
        }
        break;
      case 5:
        if (input < 10)
        {
          timeSeconds1 = input;
          cursor++;
        }
        break;
      default:
        break;
      }

      if (cursor > 5)
        cursor = 0;
    }

    break;

  case ENTERING_PROGRAM:

    if (keypadPressed)
    {

      program = keypadKey;
      currentMode = STOPPED;
      cursor = CURSOR_OFF;

      switch (program)
      {

      case 0:
        error = true;
        break;

      case 1:
        loadCore();
        reset();
        break;

      case 2:
        saveCore();
        reset();
        break;

      case 3:

        currentMode = ENTERING_TIME;
        cursor = 0;
        break;

      case 4:

        currentMode = SHOWING_TIME;
        cursor = CURSOR_OFF;
        break;

      case 5: // clear mem

        clearMem();
        break;

      case 6: // load NOPs

        loadNOPs();
        break;

      default: // load other

        if (program - 7 < programs)
        {
          enterProgram(program - 7, 0);
        }
        else
          error = true;
      }
    }

    break;

  case ENTERING_ADDRESS_HIGH:

    if (keypadPressed)
    {
      cursor = 1;
      pc = keypadKey * 16;
      currentMode = ENTERING_ADDRESS_LOW;
    }

    break;

  case ENTERING_ADDRESS_LOW:

    if (keypadPressed)
    {
      cursor = 2;
      pc += keypadKey;
      currentMode = ENTERING_OP;
    }

    break;

  case ENTERING_BREAKPOINT_HIGH:

    if (keypadPressed)
    {
      cursor = 1;
      breakAt = keypadKey * 16;
      currentMode = ENTERING_BREAKPOINT_LOW;
    }

    break;

  case ENTERING_BREAKPOINT_LOW:

    if (keypadPressed)
    {
      cursor = 2;
      breakAt += keypadKey;
      currentMode = ENTERING_BREAKPOINT_HIGH;
    }

    break;

  case ENTERING_OP:

    if (keypadPressed)
    {
      cursor = 3;
      op[pc] = keypadKey;
      currentMode = ENTERING_ARG1;
    }

    break;

  case ENTERING_ARG1:

    if (keypadPressed)
    {
      cursor = 4;
      arg1[pc] = keypadKey;
      currentMode = ENTERING_ARG2;
    }

    break;

  case ENTERING_ARG2:

    if (keypadPressed)
    {
      cursor = 2;
      arg2[pc] = keypadKey;
      currentMode = ENTERING_OP;
    }

    break;

  case RUNNING:
    run();
    break;

  case ENTERING_REG:

    if (keypadPressed)
      currentReg = keypadKey;

    break;

  case INSPECTING:

    if (keypadPressed)
      reg[currentReg] = keypadKey;

    break;
  }
}

void run()
{
  isDISP = false;

  if (!singleStep)
    delay(cpu_delay);

  if (!jump)
    pc++;

  if (!singleStep && breakAt == pc && breakAt > 0 && !ignoreBreakpointOnce)
  {
    currentMode = STOPPED;
    return;
  }

  ignoreBreakpointOnce = false;

  jump = false;

  byte op1 = op[pc];
  byte hi = arg1[pc];
  byte lo = arg2[pc];

  byte s = hi;
  byte d = lo;
  byte n = hi;

  byte disp_n = hi;
  byte disp_s = lo;

  byte dot_s = lo;

  byte adr = hi * 16 + lo;
  byte op2 = op1 * 16 + hi;
  unsigned int op3 = op1 * 256 + hi * 16 + lo;

  switch (op1)
  {
  case OP_MOV:

    reg[d] = reg[s];
    zero = reg[d] == 0;

    break;

  case OP_MOVI:

    reg[d] = n;
    zero = reg[d] == 0;

    break;

  case OP_AND:

    reg[d] &= reg[s];
    carry = false;
    zero = reg[d] == 0;

    break;

  case OP_ANDI:

    reg[d] &= n;
    carry = false;
    zero = reg[d] == 0;

    break;

  case OP_ADD:

    reg[d] += reg[s];
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;

    break;

  case OP_ADDI:

    reg[d] += n;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;

    break;

  case OP_SUB:

    reg[d] -= reg[s];
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;

    break;

  case OP_SUBI:

    reg[d] -= n;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;

    break;

  case OP_CMP:

    carry = reg[s] < reg[d];
    zero = reg[s] == reg[d];

    break;

  case OP_CMPI:

    carry = n < reg[d];
    zero = reg[d] == n;

    break;

  case OP_OR:

    reg[d] |= reg[s];
    carry = false;
    zero = reg[d] == 0;

    break;

    //
    //
    //

  case OP_CALL:

    if (sp < STACK_DEPTH - 1)
    {
      stack[sp] = pc;
      sp++;
      pc = adr;
      jump = true;
    }
    else
    {

      error = true;
      currentMode = STOPPED;
    }

    break;

  case OP_GOTO:

    pc = adr;
    jump = true;

    break;

  case OP_BRC:

    if (carry)
    {
      pc = adr;
      jump = true;
    }

    break;

  case OP_BRZ:

    if (zero)
    {
      pc = adr;
      jump = true;
    }

    break;

    //
    //
    //

  default:
  {

    switch (op2)
    {

    case OP_MAS:

      regEx[d] = reg[d];

      break;

    case OP_INV:

      reg[d] ^= 15;

      break;

    case OP_SHR:

      carry = reg[d] & 1;
      reg[d] >>= 1;
      zero = reg[d] == 0;

      break;

    case OP_SHL:

      reg[d] <<= 1;
      carry = reg[d] & 16;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

    case OP_ADC:

      if (carry)
        reg[d]++;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

    case OP_SUBC:

      if (carry)
        reg[d]--;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

      //
      //
      //

    case OP_DIN:

#ifndef INVERTED_INPUTS 
      reg[d] = digitalRead(DIN_PIN_1) | digitalRead(DIN_PIN_2) << 1 | digitalRead(DIN_PIN_3) << 2 | digitalRead(DIN_PIN_4) << 3;
#endif 

#ifdef INVERTED_INPUTS 
      reg[d] = ! digitalRead(DIN_PIN_1) | ! digitalRead(DIN_PIN_2) << 1 | ! digitalRead(DIN_PIN_3) << 2 | ! digitalRead(DIN_PIN_4) << 3;
#endif 

      carry = false;
      zero = reg[d] == 0;

      break;

    case OP_DOT:

      outputs = reg[dot_s];
      carry = false;
      zero = reg[dot_s] == 0;

      break;

    case OP_KIN:

      currentMode = ENTERING_VALUE;
      currentInputRegister = d;

      break;

      //
      //
      //

    default:
    {

      switch (op3)
      {

      case OP_HALT:

        currentMode = STOPPED;
        break;

      case OP_NOP:

        break;

      case OP_DISOUT:

        showingDisplayDigits = 0;
        displayOff();

        break;

      case OP_HXDZ:

        num =
            reg[0xD] +
            16 * reg[0xE] +
            256 * reg[0xF];

        zero = num > 999;
        carry = false;

        num %= 1000;

        reg[0xD] = num % 10;
        reg[0xE] = (num / 10) % 10;
        reg[0xF] = (num / 100) % 10;

        break;

      case OP_DZHX:

        num =
            reg[0xD] +
            10 * reg[0xE] +
            100 * reg[0xF];

        carry = false;
        zero = false;

        reg[0xD] = num % 16;
        reg[0xE] = (num / 16) % 16;
        reg[0xF] = (num / 256) % 16;

        break;

      case OP_RND:

        reg[0xD] = random(16);
        reg[0xE] = random(16);
        reg[0xF] = random(16);

        break;

      case OP_TIME:

        reg[0xA] = timeSeconds1;
        reg[0xB] = timeSeconds10;
        reg[0xC] = timeMinutes1;
        reg[0xD] = timeMinutes10;
        reg[0xE] = timeHours1;
        reg[0xF] = timeHours10;

        break;

      case OP_RET:

        pc = stack[--sp] + 1;
        jump = true;
        break;

      case OP_CLEAR:

        for (byte i = 0; i < 16; i++)
          reg[i] = 0;

        carry = false;
        zero = true;

        break;

      case OP_STC:

        carry = true;

        break;

      case OP_RSC:

        carry = false;

        break;

      case OP_MULT:

        num =
            reg[0] + 10 * reg[1] + 100 * reg[2] +
            1000 * reg[3] + 10000 * reg[4] + 100000 * reg[5];

        num2 =
            regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
            1000 * regEx[3] + 10000 * regEx[4] + 100000 * regEx[5];

        num *= num2;

        carry = num > 999999;

        for (int i = 0; i < 6; i++)
        {
          carry |= (reg[i] > 9 || regEx[i] > 9);
        }

        zero = false;

        num = num % 1000000;

        if (carry)
        {

          reg[0] = 0xE;
          reg[1] = 0xE;
          reg[2] = 0xE;
          reg[3] = 0xE;
          reg[4] = 0xE;
          reg[5] = 0xE;
        }
        else
        {

          reg[0] = num % 10;
          reg[1] = (num / 10) % 10;
          reg[2] = (num / 100) % 10;
          reg[3] = (num / 1000) % 10;
          reg[4] = (num / 10000) % 10;
          reg[5] = (num / 100000) % 10;
        }

        for (int i = 0; i < 6; i++) // not documented in manual, but true!
          regEx[i] = 0;

        break;

      case OP_DIV:

        num2 =
            reg[0] + 10 * reg[1] + 100 * reg[2] +
            1000 * reg[3];

        num =
            regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
            1000 * regEx[3];

        carry = false;

        for (int i = 0; i < 6; i++)
        {
          carry |= (reg[i] > 9 || regEx[i] > 9);
        }

        if (num2 == 0 || carry)
        {

          carry = true;
          zero = false,

          reg[0] = 0xE;
          reg[1] = 0xE;
          reg[2] = 0xE;
          reg[3] = 0xE;
          reg[4] = 0xE;
          reg[5] = 0xE;
        }
        else
        {

          carry = false;
          num3 = num / num2;

          reg[0] = num3 % 10;
          reg[1] = (num3 / 10) % 10;
          reg[2] = (num3 / 100) % 10;
          reg[3] = (num3 / 1000) % 10;

          num3 = num % num2;
          zero = num3 > 0;

          regEx[0] = num3 % 10;
          regEx[1] = (num3 / 10) % 10;
          regEx[2] = (num3 / 100) % 10;
          regEx[3] = (num3 / 1000) % 10;
        }

        break;

      case OP_EXRL:

        for (int i = 0; i < 8; i++)
        {
          byte aux = reg[i];
          reg[i] = regEx[i];
          regEx[i] = aux;
        }

        break;

      case OP_EXRM:

        for (int i = 8; i < 16; i++)
        {
          byte aux = reg[i];
          reg[i] = regEx[i];
          regEx[i] = aux;
        }

        break;

      case OP_EXRA:

        for (int i = 0; i < 8; i++)
        {
          byte aux = reg[i];
          reg[i] = reg[i + 8];
          reg[i + 8] = aux;
        }

        break;

      default: // DISP!

        displayOff();
        showingDisplayDigits = disp_n;
        showingDisplayFromReg = disp_s;
        isDISP = true;

	if ( singleStep ) 
	   showDisplay();

        break;

      }
    }
    }
  }
  }

  if (singleStep)
  {
    currentMode = STOPPED;
    if (!jump)
    {
      pc++;
    }
  }
}

//
// Main Loop
//

void loop()
{

  functionKeyRaw = module.getButtons();
  functionKey = functionKeyRaw; 

  if (functionKey == previousFunctionKey)
  { // button held down pressed
    functionKey = NO_KEY;
  }
  else if (millis() - lastFuncKeyTime > FUNCTION_KEY_DEBOUNCE_TIME)
  { // debounce
    previousFunctionKey = functionKey;
    error = false;
    lastFuncKeyTime = millis();
  }
  else
    functionKey = NO_KEY;

  keypadKeyRaw = keypad.getKey();
  keypadKey = keypadKeyRaw; 

  if (keypadKey == previousKeypadKey)
  { // button held down pressed
    keypadKey = NO_KEY;
  }
  else
  {
    previousKeypadKey = keypadKey;
  }

  //
  //
  //

  if (keypadKey != NO_KEY)
  {
    keypadKey--;
    previousFunctionKey = NO_KEY;
    keypadPressed = true;
  }
  else
    keypadPressed = false;

  //
  //
  //

  displayStatus();
  interpret();

#ifdef RESET_BUTTON_AT_PIN_0 
  if (!digitalRead(RESET_PIN))
  {
    reset();
  }
#endif

  if (functionKey == (HALT | CCE))
  {
    reset();
  }

#ifdef USE_CPU_THROTTLE_POT_AT_PIN_A5 
  cpu_delay = analogRead(CPU_THROTTLE_ANALOG_PIN) / CPU_THROTTLE_DIVISOR;
  if (cpu_delay < CPU_MIN_THRESHOLD)
    cpu_delay = 0;
#endif 

  // RUN + HEX = Set CPU Throttle: 

  if (functionKeyRaw == RUN && keypadKeyRaw != NO_KEY) {
    cpu_delay = cpu_delays[keypadKeyRaw-1]; 
    keypadPressed = false;
    functionKey = NO_KEY;
    keypadKey = NO_KEY;    
  }

}
