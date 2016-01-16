/*

  A Busch 2090 Microtronic Emulator for Arduino Uno / R3

  Version 0.81 (c) Michael Wessel, January 14 2016

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  Hardware requirements:
  - 4x4 hex keypad (HEX keypad for data and program entry)
  - TM1638 8 digit 7segment display with 8 LEDs and 8 buttons (function keys)

  The Busch Microtronic 2090 is (C) Busch GmbH
  See http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de

  This program was written by Michael Wessel


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

#include <TM1638.h>
#include <Keypad.h>
#include <TM16XXFonts.h>
#include <avr/pgmspace.h>

//
// set up hardware - wiring
//

// use pins 14, 15, 16 for TM1638 module
TM1638 module(14, 15, 16);

// set up the 4x4 matrix - use pins 5 - 12
#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = { // plus one because 0 = no key pressed!
  {0xD, 0xE, 0xF, 0x10},
  {0x9, 0xA, 0xB, 0xC},
  {0x5, 0x6, 0x7, 0x8},
  {0x1, 0x2, 0x3, 0x4}
};

byte colPins[COLS] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte rowPins[ROWS] = {9, 10, 11, 12}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//
//
//

#define CPU_THROTTLE_ANALOG_PIN 5 // connect a potentiometer here for CPU speed throttle controll 
#define CPU_THROTTLE_DIVISOR 50 // potentiometer dependent 
#define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0

//
// some harcoded example programs
// via PGM 7 - F. More to be added soon.
// Nim game still missing here.
//

#define PGM7 "F10 FF0 FE0 C00 " // keyboard input and data out 
#define PGM7_ADR 0x00

#define PGM8 "F60 510 521 532 543 554 565 FE0 C00 " // crazy counter and data out 
#define PGM8_ADR 0x00

#define PGM9 "F3D F05 C00 " // random generator 
#define PGM9_ADR 0x00

#define PGMA "F30 510 FB1 FB2 FE1 FE1 C00 " // three digit counter with carry 
#define PGMA_ADR 0x00

#define PGMB "F08 F31 FF0 023 012 001 C01 " // shifting keyboard input  
#define PGMB_ADR 0x00

#define PGMC "110 F10 FE0 FA0 FB0 C02 " // scrolling light
#define PGMC_ADR 0x00

#define PGMD "F10 FD0 FE0 C01 " // digital input pin test D1 - D4 - connect to ground 
#define PGMD_ADR 0x00

byte program;

//
//
//

int cpu_delay = 0; 

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

#define CURSOR_OFF 5
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
// internal clock (not really working yet)
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

byte op[256] ;
byte arg1[256] ;
byte arg2[256] ;

byte pc = 0;
byte breakAt = 0;

//
// stack
//

#define STACK_DEPTH 5

byte stack[STACK_DEPTH] ;
byte sp = 0;

//
// registers
//

byte reg[16];
byte regEx[16];

//
// keypad key and function key input
//

boolean keypadPressed = false;

byte input = 0;
byte keypadKey = NO_KEY;
byte functionKey = NO_KEY;
byte previousFunctionKey = NO_KEY;
byte previousKeypadKey = NO_KEY;

//
// current mode / status of emulator
//

enum mode {
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
  ENTERING_PROGRAM

};

mode currentMode = STOPPED;

//
// OP codes
//

#define OP_MOV  0x0
#define OP_MOVI 0x1
#define OP_AND  0x2
#define OP_ANDI 0x3
#define OP_ADD  0x4
#define OP_ADDI 0x5
#define OP_SUB  0x6
#define OP_SUBI 0x7
#define OP_CMP  0x8
#define OP_CMPI 0x9
#define OP_OR   0xA

#define OP_CALL 0xB
#define OP_GOTO 0xC
#define OP_BRC  0xD
#define OP_BRZ  0xE

#define OP_MAS  0xF7
#define OP_INV  0xF8
#define OP_SHR  0xF9
#define OP_SHL  0xFA
#define OP_ADC  0xFB
#define OP_SUBC 0xFC

#define OP_DIN  0xFD
#define OP_DOT  0xFE
#define OP_KIN  0xFF

#define OP_HALT   0xF00
#define OP_NOP    0xF01
#define OP_DISOUT 0xF02
#define OP_HXDZ   0xF03
#define OP_DZHX   0xF04
#define OP_RND    0xF05
#define OP_TIME   0xF06
#define OP_RET    0xF07
#define OP_CLEAR  0xF08
#define OP_STC    0xF09
#define OP_RSC    0xF0A
#define OP_MULT   0xF0B
#define OP_DIV    0xF0C
#define OP_EXRL   0xF0D
#define OP_EXRM   0xF0E
#define OP_EXRA   0xF0F

#define OP_DISP 0xF

//
// setup Arduino
//

void setup() {

  Serial.begin(9600);
  randomSeed(analogRead(0));

  pinMode(0, INPUT_PULLUP); // reset pin
  pinMode(1, INPUT_PULLUP); // DIN 1
  pinMode(2, INPUT_PULLUP); // DIN 2
  pinMode(3, INPUT_PULLUP); // DIN 3
  pinMode(4, INPUT_PULLUP); // DIN 4

  //digitalWrite(0, LOW);
  //digitalWrite(1, LOW);
  //digitalWrite(2, LOW);
  //digitalWrite(3, LOW);
  //digitalWrite(4, LOW);

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  ready ");

}

//
//
//

void sendString(String string) {

  for (int i = 0; i < 8; i++) {

    module.sendChar(i, FONT_DEFAULT[string[i] - 32], false);

  }

  delay(DISP_DELAY);

}

void showMem() {

  module.sendChar(1, 0, false);

  if (cursor == 0)
    module.sendChar(2, blink ? NUMBER_FONT[pc / 16 ] : 0, true);
  else
    module.sendChar(2, NUMBER_FONT[pc / 16 ], false);

  if (cursor == 1)
    module.sendChar(3, blink ? NUMBER_FONT[pc % 16]  : 0, true);
  else
    module.sendChar(3, NUMBER_FONT[pc % 16], false);

  module.sendChar(4, 0, false);

  if (cursor == 2)
    module.sendChar(5, blink ? NUMBER_FONT[op[pc]] : 0, true);
  else
    module.sendChar(5, NUMBER_FONT[op[pc]], false);

  if (cursor == 3)
    module.sendChar(6, blink ? NUMBER_FONT[arg1[pc]] : 0, true);
  else
    module.sendChar(6, NUMBER_FONT[arg1[pc]], false);

  if (cursor == 4)
    module.sendChar(7, blink ? NUMBER_FONT[arg2[pc]] : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[arg2[pc]], false);

}

void showReg() {

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
    module.sendChar(7, blink ? NUMBER_FONT[reg[currentReg]]  : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[reg[currentReg]], false);

}

void showProgram() {

  displayOff();

  module.sendChar(7, blink ? NUMBER_FONT[program] : 0, true);


}

void showError() {

  displayOff();

  if (blink)
    sendString("  Error  ");

}


void showReset() {

  displayOff();
  sendString("  reset ");

}


void displayOff() {

  for (int i = 0; i < 8; i++)
    module.sendChar(i, 0, false);

}

void showDisplay() {

  for (int i = 0; i < showingDisplayDigits; i++)
    module.sendChar(7 - i, NUMBER_FONT[reg[i +  showingDisplayFromReg]], false);

}

void displayStatus() {

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;
  unsigned long delta2 = time - lastDispTime2;

  moduleLEDs = 0;

  if (delta >= 1000) {
    clock = !clock;
    lastDispTime = time;
  }

  if (delta2 > 300) {
    blink = ! blink;
    lastDispTime2 = time;
  }

  if (carry)
    moduleLEDs = 1;

  if (zero)
    moduleLEDs |= 2;

  if (clock)
    moduleLEDs |= 4;

  char status = ' ';

  if ( currentMode == STOPPED && ! error)
    status = 'H';
  else if (currentMode ==
           ENTERING_ADDRESS_HIGH ||
           currentMode ==
           ENTERING_ADDRESS_LOW )
    status = 'A';
  else if (currentMode == ENTERING_OP ||
           currentMode == ENTERING_ARG1 ||
           currentMode == ENTERING_ARG2 )
    status = 'P';
  else if (currentMode == RUNNING)
    status = 'r';
  else if (currentMode == ENTERING_REG ||
           currentMode == INSPECTING )
    status = 'i';
  else if (currentMode == ENTERING_VALUE )
    status = '?';
  else status = ' ' ;

  module.sendChar(0, FONT_DEFAULT[status - 32], false);

  moduleLEDs |= ( outputs << 4 );

  module.setLEDs( moduleLEDs);

  if ( currentMode == RUNNING || currentMode == ENTERING_VALUE )
    showDisplay();
  else if ( currentMode == ENTERING_REG || currentMode == INSPECTING )
    showReg();
  else if ( currentMode == ENTERING_PROGRAM )
    showProgram();
  else if ( error )
    showError();
  else
    showMem();


}

byte decodeHex(char c) {

  if (c >= 65)
    return c - 65 + 10;
  else
    return c - 48;

}

void enterProgram(String string, byte start) {

  int i = 0;
  byte origin = start;

  cursor = 0;

  while ( string[i] ) {
    op[start] = decodeHex(string[i++]);
    arg1[start] = decodeHex(string[i++]);
    arg2[start] = decodeHex(string[i++]);
    i++; // skip over "-"
    start++;
    pc = start;
    currentMode = STOPPED;
    outputs = pc;
    displayStatus();
    delay(5);
  };


  pc = origin;
  currentMode = STOPPED;

}

void showLoaded() {

  sendString(" loaded ");
  displayOff();
  module.sendChar(7, NUMBER_FONT[program], false);
  delay(DISP_DELAY);
  sendString("   at   ");
  module.sendChar(3, NUMBER_FONT[pc / 16], false);
  module.sendChar(4, NUMBER_FONT[pc % 16], false);
  delay(DISP_DELAY);

}

void clearStack() {

  sp = 0;

}

void reset() {

  showReset();
  delay(250);

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  for (currentReg = 0; currentReg < 16; currentReg++) {
    reg[currentReg] = 0;
    regEx[currentReg] = 0;
  }

  currentReg = 0;
  currentInputRegister = 0;

  carry = false;
  zero = false;
  error = false;
  pc = 0;
  clearStack();

  outputs = 0;

}

void clearMem() {

  cursor = 0;

  for (pc = 0; pc < 255; pc++) {
    op[pc] = 0;
    arg1[pc] = 0;
    arg2[pc] = 0;
    outputs = pc;
    displayStatus();
  }
  op[255] = 0;
  arg1[255] = 0;
  arg2[255] = 0;

  pc = 0;

}

void loadNOPs() {

  cursor = 0;

  for (pc = 0; pc < 255; pc++) {
    op[pc] = 15;
    arg1[pc] = 0;
    arg2[pc] = 1;

    outputs = pc;
    displayStatus();
  }
  op[255] = 15;
  arg1[255] = 0;
  arg2[255] = 1;

  pc = 0;

}

void interpret() {

  switch ( functionKey ) {

    case HALT :
      currentMode = STOPPED;
      cursor = CURSOR_OFF;
      break;

    case RUN :
      currentMode = RUNNING;
      displayOff();
      clearStack();
      //step();
      break;

    case NEXT :
      if (currentMode == STOPPED) {
        currentMode = ENTERING_ADDRESS_HIGH;
        cursor = 0;
      } else {
        pc++;
        cursor = 2;
        currentMode = ENTERING_OP;
      }

      break;

    case REG :

      if (currentMode != ENTERING_REG) {
        currentMode = ENTERING_REG;
        cursor = 0;
      } else {
        currentMode = INSPECTING;
        cursor = 1;
      }

      break;

    case STEP :

      break;

    case BKP :

      break;

    case CCE :

      if (cursor == 2) {
        cursor = 4;
        arg2[pc] = 0;
        currentMode = ENTERING_ARG2;
      } else if (cursor == 3) {
        cursor = 2;
        op[pc] = 0;
        currentMode = ENTERING_OP;
      } else {
        cursor = 3;
        arg1[pc] = 0;
        currentMode = ENTERING_ARG1;
      }

      break;

    case PGM :

      if ( currentMode != ENTERING_PROGRAM ) {
        cursor = 0;
        currentMode = ENTERING_PROGRAM;
      }

      break;

  }


  //
  //
  //

  switch (currentMode) {

    case STOPPED :
      cursor = CURSOR_OFF;
      break;

    case ENTERING_VALUE :

      if (keypadPressed) {
        input = keypadKey;
        reg[currentInputRegister] = input;
        carry = false;
        zero = reg[currentInputRegister] == 0;
        currentMode = RUNNING;
      }

      break;

    case ENTERING_PROGRAM :

      if (keypadPressed) {

        program = keypadKey;
        currentMode = STOPPED;

        switch ( program ) {

          case 0 :
          case 1 :
          case 2 :
            error = true;

          case 3 :   // enter clock time - not yet
            break;

          case 4 : // show clock - not yet
            break;

          case 5 : // clear mem

            clearMem();
            break;

          case 6 : // load NOPs

            loadNOPs();
            break;

          default : // load other

            switch (program) {
              case 7 : enterProgram(PGM7, PGM7_ADR); break;
              case 8 : enterProgram(PGM8, PGM8_ADR); break;
              case 9 : enterProgram(PGM9, PGM9_ADR); break;
              case 10 : enterProgram(PGMA, PGMA_ADR); break;
              case 11 : enterProgram(PGMB, PGMB_ADR); break;
              case 12 : enterProgram(PGMC, PGMC_ADR); break;
              case 13 : enterProgram(PGMD, PGMD_ADR); break;
              default :
                error = true;
            }
        }

        if (! error)
          showLoaded();

      }

      break;

    case ENTERING_ADDRESS_HIGH :

      if (keypadPressed) {
        cursor = 1;
        pc = keypadKey * 16;
        currentMode = ENTERING_ADDRESS_LOW;
      }

      break;

    case ENTERING_ADDRESS_LOW :

      if (keypadPressed) {
        cursor = 2;
        pc += keypadKey;
        currentMode = ENTERING_OP;
      }

      break;

    case ENTERING_OP :

      if (keypadPressed) {
        cursor = 3;
        op[pc] = keypadKey;
        currentMode = ENTERING_ARG1;
      }

      break;

    case ENTERING_ARG1 :

      if (keypadPressed) {
        cursor = 4;
        arg1[pc] = keypadKey;
        currentMode = ENTERING_ARG2;
      }

      break;

    case ENTERING_ARG2 :

      if (keypadPressed) {
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

    case INSPECTING :

      if (keypadPressed)
        reg[currentReg] = keypadKey;

      break;

  }

}

void run() {

  delay(cpu_delay);

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

  switch (op1) {
    case OP_MOV :

      reg[d] = reg[s];
      zero = reg[d] == 0;
      pc++;

      break;

    case OP_MOVI :

      reg[d] = n;
      zero = reg[d] == 0;
      pc++;

      break;

    case OP_AND :

      reg[d] &= reg[s];
      carry = false;
      zero = reg[d] == 0;
      pc++;

      break;

    case OP_ANDI :

      reg[d] &= n;
      carry = false;
      zero = reg[d] == 0;
      pc++;

      break;

    case OP_ADD :

      reg[d] += reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;
      pc++;

      break;

    case OP_ADDI :

      reg[d] += n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      pc++;

      break;

    case OP_SUB :

      reg[d] -= reg[s]; // +/- 1 if negative?
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      pc++;

      break;

    case OP_SUBI :

      reg[d] -= n; // +/- 1 if negative?
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      pc++;

      break;

    case OP_CMP :

      carry = reg[s] < reg[d];
      zero = reg[s] == reg[d];
      pc++;

      break;

    case OP_CMPI :

      carry = n < reg[d];
      zero = reg[d] = n;
      pc++;

      break;

    case OP_OR :

      reg[d] |= reg[s];
      carry = false;
      zero = reg[d] == 0;
      pc++;

      break;

    //
    //
    //


    case OP_CALL :

      if (sp < STACK_DEPTH - 1) {
        stack[sp] = pc;
        sp++;
        pc = adr;

      } else {

        error = true;
        currentMode = STOPPED;

      }
      break;

    case OP_GOTO :

      pc = adr;

      break;

    case OP_BRC :

      if (carry)
        pc = adr;
      else
        pc++;

      break;

    case OP_BRZ :

      if (zero)
        pc = adr;
      else
        pc++;

      break;

    //
    //
    //

    default : {

        switch (op2) {

          case OP_MAS :

            regEx[d] = reg[d];
            pc++;

            break;

          case OP_INV :

            reg[d] ^= 15;
            pc++;

            break;

          case OP_SHR :

            reg[d] >>= 1;
            carry = reg[d] & 16;
            reg[d] &= 15;
            zero = reg[d] == 0;
            pc++;

            break;

          case OP_SHL :

            reg[d] <<= 1;
            carry = reg[d] & 16;
            reg[d] &= 15;
            zero = reg[d] == 0;
            pc++;

            break;

          case OP_ADC :

            if (carry) reg[d]++;
            carry = reg[d] > 15;
            reg[d] &= 15;
            zero = reg[d] == 0;
            pc++;

            break;

          case OP_SUBC :

            if (carry) reg[d]--;
            carry = reg[d] > 15;
            reg[d] &= 15;
            zero = reg[d] == 0;
            pc++;

            break;

          //
          //
          //

          case OP_DIN :

            reg[d] = !digitalRead(1) | !digitalRead(2) << 1 | !digitalRead(3) << 2 | !digitalRead(4) << 3;
            carry = false;
            zero = reg[d] == 0;
            pc++;

            break;

          case OP_DOT :

            outputs = reg[dot_s];
            carry = false;
            zero = reg[dot_s] == 0;
            pc++;

            break;

          case OP_KIN :

            currentMode = ENTERING_VALUE;
            currentInputRegister = d;
            pc++;

            break;

          //
          //
          //

          default : {

              switch (op3) {

                case OP_HALT :

                  currentMode = STOPPED;
                  break;

                case OP_NOP :

                  pc++;
                  break;

                case OP_DISOUT :

                  showingDisplayDigits = 0;
                  pc++;

                  break;

                case OP_HXDZ :

                  num =
                    reg[0xD] +
                    16 * reg[0xE] +
                    256 * reg[0xF];

                  zero = num > 999;
                  carry = false;

                  num %= 1000;

                  reg[0xD] = num % 10;
                  reg[0xE] = ( num / 10 ) % 10;
                  reg[0xF] = ( num / 100 ) % 10;

                  pc++;

                  break;

                case OP_DZHX :

                  num =
                    reg[0xD] +
                    10 * reg[0xE] +
                    100 * reg[0xF];

                  carry = false;
                  zero = false;

                  reg[0xD] = num % 16;
                  reg[0xE] = ( num / 16 ) % 16;
                  reg[0xF] = ( num / 256 ) % 16;

                  pc++;

                  break;

                case OP_RND :

                  reg[0xD] = random(16);
                  reg[0xE] = random(16);
                  reg[0xF] = random(16);

                  pc++;

                  break;

                case OP_TIME :

                  reg[0xA] = timeSeconds1;
                  reg[0xB] = timeSeconds10;
                  reg[0xC] = timeMinutes1;
                  reg[0xD] = timeMinutes10;
                  reg[0xE] = timeHours1;
                  reg[0xF] = timeHours10;

                  pc++;

                  break;

                case OP_RET :

                  pc = stack[--sp] + 1;
                  break;

                case OP_CLEAR :

                  for (byte i = 0; i < 16; i ++)
                    reg[i] = 0;

                  carry = false;
                  zero = true;

                  pc++;

                  break;

                case OP_STC :

                  carry = true;
                  pc++;

                  break;

                case OP_RSC :

                  carry = false;
                  pc++;

                  break;

                case OP_MULT :

                  num =
                    reg[0] + 10 * reg[1] + 100 * reg[2] +
                    1000 * reg[3] + 10000 * reg[4] + 100000 * reg[5];

                  num2 =
                    regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
                    1000 * regEx[3] + 10000 * regEx[4] + 100000 * regEx[5];

                  num *= num2;

                  carry = num > 999999;
                  zero  = false;

                  num = num % 1000000;

                  if (carry) {

                    reg[0] = 0xE;
                    reg[1] = 0xE;
                    reg[2] = 0xE;
                    reg[3] = 0xE;
                    reg[4] = 0xE;
                    reg[5] = 0xE;

                  } else {

                    reg[0] = num % 10;
                    reg[1] = ( num / 10 ) % 10;
                    reg[2] = ( num / 100 ) % 10;
                    reg[3] = ( num / 1000 ) % 10;
                    reg[4] = ( num / 10000 ) % 10;
                    reg[5] = ( num / 100000 ) % 10;
                  }

                  pc++;
                  break;

                case OP_DIV :

                  num2 =
                    reg[0] + 10 * reg[1] + 100 * reg[2] +
                    1000 * reg[3];

                  num =
                    regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
                    1000 * regEx[3];

                  if (num2 == 0 || num == 0 // really?
                     ) {
                    carry = true;

                    reg[0] = 0xE;
                    reg[1] = 0xE;
                    reg[2] = 0xE;
                    reg[3] = 0xE;
                    reg[4] = 0xE;
                    reg[5] = 0xE;

                  } else {

                    carry = false;
                    num3 = num / num2;

                    reg[0] = num3 % 10;
                    reg[1] = ( num3 / 10 ) % 10;
                    reg[2] = ( num3 / 100 ) % 10;
                    reg[3] = ( num3 / 1000 ) % 10;
                    reg[4] = ( num3 / 10000 ) % 10;
                    reg[5] = ( num3 / 100000 ) % 10;

                    num3 = num % num2;

                    regEx[0] = num3 % 10;
                    regEx[1] = ( num3 / 10 ) % 10;
                    regEx[2] = ( num3 / 100 ) % 10;
                    regEx[3] = ( num3 / 1000 ) % 10;
                    regEx[4] = ( num3 / 10000 ) % 10;
                    regEx[5] = ( num3 / 100000 ) % 10;

                  }

                  zero = false;
                  pc++;

                  break;

                case OP_EXRL :

                  for (int i = 0; i < 8; i++) {
                    byte aux = reg[i];
                    reg[i] = regEx[i];
                    regEx[i] = aux;
                  }
                  pc++;

                  break;

                case OP_EXRM :

                  for (int i = 8; i < 16; i++) {
                    byte aux = reg[i];
                    reg[i] = regEx[i];
                    regEx[i] = aux;
                  }
                  pc++;

                  break;

                case OP_EXRA :

                  for (int i = 0; i < 8; i++) {
                    byte aux = reg[i];
                    reg[i]   = reg[i + 8];
                    reg[i + 8] = aux;
                  }
                  pc++;

                  break;

                default : // DISP!

                  showingDisplayDigits = disp_n;
                  showingDisplayFromReg = disp_s;
                  pc++;

                  break;

                  //
                  //
                  //

              }
            }
        }
      }
  }


}

//
// main loop / emulator / shell
//

void loop() {

  functionKey = module.getButtons();

  if (functionKey == previousFunctionKey) { // button held down pressed
    functionKey = NO_KEY;
  } else {
    previousFunctionKey = functionKey;
    error = false;
  }

  keypadKey = keypad.getKey();

  if (keypadKey == previousKeypadKey) { // button held down pressed
    keypadKey = NO_KEY;
  } else {
    previousKeypadKey = keypadKey;
  }

  //
  //
  //

  if (keypadKey != NO_KEY)  {
    keypadKey --;
    previousFunctionKey = NO_KEY;
    keypadPressed = true;
  } else
    keypadPressed = false;

  //
  //
  //

  displayStatus();
  interpret();

  if (!digitalRead(0)) {
    reset();
  }

  cpu_delay = analogRead(CPU_THROTTLE_ANALOG_PIN) / CPU_THROTTLE_DIVISOR; 
  if (cpu_delay < CPU_MIN_THRESHOLD) 
    cpu_delay = 0; 

}
