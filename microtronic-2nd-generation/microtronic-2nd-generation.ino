/*

  A Busch 2090 Microtronic Emulator for Arduino Mega 2560

  Version 32 (c) Michael Wessel, February 1st, 2021

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  The Busch Microtronic 2090 is (C) Busch GmbH
  See https://www.busch-model.info/service/historie-microtronic/

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

#define VERSION "32" 
#define DATE "02-01-2021"  
 
//
//
//

// wiring 
#include "hardware.h" 

//
//
//

#include <Wire.h>
#include <ds3231.h>
#include <avr/pgmspace.h>
#include <SdFat.h>
#include <SPI.h>
#include <Adafruit_PCD8544.h>
#include <NewTone.h>

//
// RTC Current Time
//

struct ts RTC; 

//
// SD Card
//

SdFat SD;
SdFile root;
SdFile init_file;

//
// Nokia Display 
//

Adafruit_PCD8544 display = Adafruit_PCD8544(NOKIA5, NOKIA4, NOKIA3, NOKIA2, NOKIA1); 

//
// HEX 4x4 Matrix Keypad 
//

#define NO_KEY 255

#define HEX_KEYPAD_ROWS 4
#define HEX_KEYPAD_COLS 4

byte hex_keys[HEX_KEYPAD_ROWS][HEX_KEYPAD_COLS] = { // plus one because 0 = no key pressed!
  {12, 13, 14, 15},
  {8, 9, 10, 11},
  {4, 5, 6, 7},
  {0, 1, 2, 3}
};

byte hex_keypad_col_pins[HEX_KEYPAD_COLS] = HEX_COL_PINS; // columns
byte hex_keypad_row_pins[HEX_KEYPAD_ROWS] = HEX_ROW_PINS; // rows

//
// MOV: 15 Notes (0 = Tone Off!) 
// C2, C#2, D2, D#2, E2, F2, F#2, G2, G#2, A2, B#2, B2, C3, C#3, D3
//  1   2    3   4    5   6   7    8   9    A   B    C   D   E    F 
//

int note_frequencies_mov[ ] = { 65, 69, 73, 78, 82, 87, 93, 98, 104, 110, 117, 123, 131, 139, 147 };

// ADDI: 16 Notes 
// D#3, E3, F3, F#3, G3, G#3, A3, B#3, B3, C4, C#4, D4, D#4, E4, F4, F#4
//  0    1   2   3    4   5    6   7    8   9   A    B   C    D   E   F
//

int note_frequencies_addi[] = { 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370 }; 

// SUBI: 16 Notes 
// G4, G#4, A4, B#4, B4, C5, C#5, D5, D#5, E5, F5, F#5, G5,  G#5, A5,  B#5
//  0   1    2   3    4   5   6    7   8    9   A   B    C    D    E    F 
// 
 
int note_frequencies_subi[] = { 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932 }; 

//
// Function 4x4 Matrix Keypad 
//

#define CCE  1 
#define PGM  2 
#define RUN  3
#define HALT 4  
#define BKP  5
#define STEP 6  
#define NEXT 7
#define REG  8 

#define LEFT   9
#define RIGHT  10
#define UP     11
#define DOWN   12
#define ENTER  13
#define CANCEL 14
#define BACK   15
#define LIGHT  16

//
//
//

#define FN_KEYPAD_ROWS 4
#define FN_KEYPAD_COLS 4 
 
byte fn_keys[FN_KEYPAD_ROWS][FN_KEYPAD_COLS] = { 
  {NEXT, REG,  LEFT,  RIGHT },
  {BKP,  STEP, UP,    DOWN },
  {RUN,  HALT, ENTER, CANCEL },
  {CCE,  PGM,  BACK,  LIGHT }
};

byte fn_keypad_col_pins[FN_KEYPAD_COLS] = FN_COL_PINS ; 
byte fn_keypad_row_pins[FN_KEYPAD_ROWS] = FN_ROW_PINS ; 

//
// Display mode
//

enum LCDmode {
  UNDEFLCD, 
  DISP,
  DISP_LARGE, 
  MEM, 
  MEM_MNEM,
  REGWR, 
  REGAR
};

LCDmode init_displayMode = MEM;

LCDmode displayMode = init_displayMode;

typedef char LCDLine[15]; // +1 for End of String!

LCDLine mnemonic;
LCDLine file;
LCDLine autoloadsave_file;

boolean init_autorun = false; 
byte init_autorun_address = 0; 
int autosave_every_seconds = 0; 
int autosave_ticker = 0; 

int lcdCursor = 0;
int textSize = 1; 

uint8_t status_row = 0; 
uint8_t status_col = 0; 


//
// Hex keypad
//

#define REPEAT_TIME 200
#define DEBOUNCE_TIME 20

unsigned long funKeyTime  = 0;
unsigned long funKeyTime0 = 0;
unsigned long dispFunKeyTime = 0;

unsigned long hexKeyTime  = 0;

int curHexKey    = NO_KEY;
int curHexKeyRaw = NO_KEY;

//
// Function keypad 
//

int curFunKey    = NO_KEY;
int curFunKeyRaw = NO_KEY;

int displayCurFunKey = NO_KEY; // for LCD display feedback only

//
// current PGM program
//

byte program = 7;

//
// display and status variables
//

#define CURSOR_OFF 8

unsigned long lastDispTime = 0;
byte cursor = CURSOR_OFF;
boolean blink = true;

boolean dispOff = false;
boolean lastInstructionWasDisp = false;
byte showingDisplayFromReg = 0;
byte showingDisplayDigits = 0;
boolean refreshLCD = false;

char status = ' ';
char last_status = '*';

byte currentReg = 0;
byte currentInputRegister = 0;

boolean clock1hz = false;

boolean carry = false;
boolean zero = false;
boolean error = false;

boolean init_light_led = true;
boolean init_keybeep = true;

boolean light_led = init_light_led;
boolean keybeep = init_keybeep;

//
// DIN / DOT
//

byte outputs = 0;
byte inputs = 0;

//
// KIN, REG etc. input
//

byte curInput = 0;

//
// internal clock
//

byte timeSeconds1 = 0;
byte timeSeconds10 = 0;
byte timeMinutes1 = 0;
byte timeMinutes10 = 0;
byte timeHours1 = 0;
byte timeHours10 = 0;

byte timeYears1000  = 0;
byte timeYears1000R = 0; 
byte timeYears100  = 0; 
byte timeYears100R = 0; 
byte timeYears10 = 0; 
byte timeYears1  = 0; 
byte timeMonths10 = 0; 
byte timeMonths1  = 0;
byte timeDays10 = 0;
byte timeDays1  = 0; 

//
// auxilary helper variables
//

unsigned long num = 0;
unsigned long num2 = 0;
unsigned long num3 = 0;

//
// PGM ROM Programs 
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

#define PROGRAMS 9 

const char * const PGMROM[PROGRAMS] PROGMEM = {
  PGM7, PGM8, PGM9, PGMA, PGMB, PGMC, PGMD, PGME, PGMF 
};

//
// RAM program memory
//

byte op[256] ;
byte arg1[256] ;
byte arg2[256] ;

boolean jump = false;
byte pc = 0; // program counter
byte lastPc = 1;
byte breakAt = 0; // != 0 -> breakpoint set

boolean oneStepOnly = false;
boolean ignoreBreakpointOnce = false;

//
// CPU Throttle
//

int cpu_delay_delta = 5; 
int init_cpu_delay = 0; 
int cpu_delay = init_cpu_delay; 

//int cpu_speed = 0; 

boolean cpu_changed = false; 

//
// stack
//

#define STACK_DEPTH 5

byte stack[STACK_DEPTH] ;
byte sp = 0; // stack pointer

//
// register memory
//

byte reg[16];
byte regEx[16];

// for detecting changes in showDisp
byte dispReg[16]; 

//
// current mode / status of emulator
//

enum mode {
  UNDEFINED, 
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

  ENTERING_REG,
  INSPECTING,

  ENTERING_VALUE,
  ENTERING_PROGRAM,

  ENTERING_TIME,
  SHOWING_TIME, 
  ENTERING_DATE 

};

mode currentMode = STOPPED;

//
// I want to avoid all String() constructor calls - String library leads to memory leaks in Arduino!
//

const String hexStringChar[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F" };

const String decString[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15" };

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
// 1 Hz Clock Timer 4 ISR  
//

void initializeClock() { 

  // Timer4 interrupt at 1Hz

  cli(); 

  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4  = 0;
  
  OCR4A = 15624/2;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  
  // turn on CTC mode
  TCCR4B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR4B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK4 |= (1 << OCIE4A);

  sei(); 

}

ISR(TIMER4_COMPA_vect){

  clock1hz = ! clock1hz; 

  if (clock1hz) {

    timeSeconds1++;

    if (autosave_every_seconds) 
      autosave_ticker++;       

    if (timeSeconds1 > 9) {
      timeSeconds10++;
      timeSeconds1 = 0;

      if (timeSeconds10 > 5) {
	timeMinutes1++;
	timeSeconds10 = 0;

	if (timeMinutes1 > 9) {
	  timeMinutes10++;
	  timeMinutes1 = 0;

	  if (timeMinutes10 > 5) {
	    timeHours1++;
	    timeMinutes10 = 0;

	    if (timeHours10 < 2) {
	      if (timeHours1 > 9) {
		timeHours1 = 0;
		timeHours10++;
	      }
	    } else if (timeHours10 == 2) {
	      if (timeHours1 > 3) {
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

byte hex_keypad_getKey() {

  for (int x=0;x<HEX_KEYPAD_ROWS;x++)  {
    digitalWrite(hex_keypad_row_pins[x],HIGH);            
  }

  for (int x=0;x<HEX_KEYPAD_ROWS;x++)  {
    digitalWrite(hex_keypad_row_pins[x],LOW);          
    for (int y=0;y<HEX_KEYPAD_COLS;y++)  {
      if ( ! digitalRead( hex_keypad_col_pins[y])  ) {
	return hex_keys[x][y]; 
      }
    }
    digitalWrite(fn_keypad_row_pins[x],HIGH);
  }

  return NO_KEY; 

}


byte fn_keypad_getKey() {

  for (int x=0;x<FN_KEYPAD_ROWS;x++)  {
    digitalWrite(fn_keypad_row_pins[x],HIGH);            
  }

  for (int x=0;x<FN_KEYPAD_ROWS;x++)  {
    digitalWrite(fn_keypad_row_pins[x],LOW);          
    for (int y=0;y<FN_KEYPAD_COLS;y++)  {
      if ( ! digitalRead( fn_keypad_col_pins[y])  ) {
	return fn_keys[x][y]; 
      }
    }
    digitalWrite(fn_keypad_row_pins[x],HIGH);
  }

  return NO_KEY; 

}


//
//
//

void readFunKeys() {

  int button = fn_keypad_getKey(); 
   
  // some button currently pressed? 
  if (button != NO_KEY) {
    // button changed from NO_KEY to some key? 
    if ( curFunKeyRaw == NO_KEY && ((millis() - funKeyTime0) > DEBOUNCE_TIME) || 
         curFunKeyRaw != NO_KEY && ((millis() - funKeyTime) > REPEAT_TIME)) {

      if (keybeep) 
	NewTone(TONEPIN, FUNKEYTONE, KEYTONELENGTH); 

      if (curFunKeyRaw == NO_KEY) 
	dispFunKeyTime = millis(); 

      curFunKeyRaw = button; 
      curFunKey = button;

      displayCurFunKey = button;
      funKeyTime = millis(); 
	  

    }
  } else {
    curFunKeyRaw = NO_KEY; 
    funKeyTime0 = millis(); 
  }
}

boolean funKeyPressed(int key) {
  // check for key and consume it when found 

  if (curFunKey == key) {
    curFunKey = NO_KEY; 
    return true; 
  }

  return false; 

}

void readHexKeys() {

  int button = hex_keypad_getKey(); 
   
  // some button currently pressed? 
  if (button != NO_KEY) {
    // button changed from NO_KEY to some key? 
    if ( curHexKeyRaw == NO_KEY && ((millis() - hexKeyTime) > DEBOUNCE_TIME)) {
      if (keybeep & currentMode != ENTERING_VALUE) 
	NewTone(TONEPIN, HEXKEYTONE, KEYTONELENGTH); 

      curHexKeyRaw = button; 
      curHexKey = button;
    }
  } else {
    curHexKeyRaw = NO_KEY; 
    hexKeyTime = millis(); 
  }

}

boolean hexKeyPressed(int key) {
  // check for key and consume it when found 

  if (curHexKey == key) {
    curHexKey = NO_KEY; 
    return true; 
  }

  return false; 

}

boolean someHexKeyPressed() {
  return (curHexKey != NO_KEY); 
}

int getCurHexKey() {
  // check for key and consume it when found 

  if (curHexKey != NO_KEY) {
    int x = curHexKey; 
    curHexKey = NO_KEY; 
    return x; 
  }
  
  return NO_KEY; 

}

//
//
//

byte readDIN() {

  return ( ! digitalRead(DIN_1)      |
           ! digitalRead(DIN_2) << 1 |
           ! digitalRead(DIN_3) << 2 |
           ! digitalRead(DIN_4) << 3   );

}

//
//
//

void clearFileBuffer() {

  for (int i = 0; i < 14; i++)
    file[i] = ' ';

  file[14] = 0;

  lcdCursor = 0;

}


int selectFileNo(int no) {

  int count = 0;

  clearFileBuffer();

  SD.vwd()->rewind();
  while (root.openNext(SD.vwd(), O_READ)) {
    if (! root.isDir()) {
      count++;
      if (count == no ) {
	root.getName(file, 14);
        root.close();
        return count;
      }
    }
    root.close();
  }

  return 0;

}


int countFiles() {

  int count = 0; 

  SD.vwd()->rewind();

  while (root.openNext(SD.vwd(), O_READ)) {
    if (  !root.isDir()) {      
      count++; 
    }
    root.close();
  }

  return count;

}

int selectFile() {

  int no = 1;
  int count = countFiles(); 
  int lastNo = 0; 

  selectFileNo(no);

  unsigned long last = millis();
  boolean blink = false;

  readFunKeys();

  while ( true ) {

    if ( lastNo != no || ( millis() - last ) > 500 ) {

      last = millis();

      display.clearDisplay(); 
      setTextSize(1); 
      displaySetCursor(0, 0);
      display.print("Load ");
      display.print(no);
      display.print("/");
      display.print(count);

      displaySetCursor(0, 2);
      sep(); 

      blink = !blink;

      if (blink) {
	display.setTextColor(WHITE, BLACK); 
      } else {
	display.setTextColor(BLACK, WHITE); 
      }

      displaySetCursor(0, 3);
      display.print(file);

      display.setTextColor(BLACK, WHITE); 

      displaySetCursor(0, 4);
      sep(); 
   
      display.display(); 

      lastNo = no; 

    }

    readFunKeys();

    if ( funKeyPressed(UP)) {
      if (no < count) 
	no = selectFileNo(no + 1); 
      else 
	no = selectFileNo(1); 
    } else if (funKeyPressed(DOWN)) {
      if (no > 1) 
	no = selectFileNo(no - 1); 
      else 
	no = selectFileNo(count); 
    } else if (funKeyPressed(CANCEL)) {
      announce(1,1,"CANCEL"); 
      return -1; 
    } else if (funKeyPressed(ENTER)) {
      return no; 
    }
  }

}

int askQuestion(String q) {
 
  display.clearDisplay();
  setTextSize(2); 
  displaySetCursor(0, 0);	
  display.print(q);
  displaySetCursor(0, 1);
  setTextSize(1); 
  sep(); 
  displaySetCursor(0, 4);	
  display.print("  ENTER YES");
  displaySetCursor(0, 5);	
  display.print("  CANCEL NO");

  display.display(); 

  curFunKey = NO_KEY;
  while ( curFunKey != ENTER && curFunKey != CANCEL) {
    readFunKeys();      
  }	
    
  return curFunKey; 

} 

int createName() {

  display.clearDisplay();

  clearFileBuffer();
  file[0] = 'P';

  int cursor = 0;
  int length = 1;

  unsigned long last = millis();
  boolean blink = false;
  boolean change = true; 

  while ( true ) {

    if ( change || ( millis() - last ) > 500 ) {

      last = millis();

      display.clearDisplay(); 
      setTextSize(1); 
      displaySetCursor(0, 0);
      display.print("Create File");
      displaySetCursor(0, 1);
      display.print("Save Prog");
      displaySetCursor(0, 2);
      sep(); 
      displaySetCursor(0, 3);
      display.print(file); 
      
      blink = !blink;
      if (blink) {
	display.setTextColor(WHITE, BLACK); 
      } else {
	display.setTextColor(BLACK, WHITE); 
      }
      displaySetCursor(cursor, 3);
      display.print(file[cursor]); 
      display.setTextColor(BLACK, WHITE); 

      displaySetCursor(0, 4);
      sep(); 

      display.display(); 

      change = false; 

    }

    readFunKeys();

    if ( funKeyPressed(UP)) {
      file[cursor]++; 
      change = true;  
    } else if ( funKeyPressed(DOWN)) {
      file[cursor]--; 
      change = true; 
    } else if ( funKeyPressed(LEFT)) {
      cursor--; 
      change = true; 
    } else if ( funKeyPressed(RIGHT)) {
      cursor++; 
      file[cursor] = file[cursor-1]; 
      change = true; 
    } else if ( funKeyPressed(BACK)) {
      if (cursor == length - 1 && cursor > 0) {
	file[cursor] = 0;
	length--;
	cursor--;
	change = true; 
      } 
    } else if ( funKeyPressed(CANCEL)) {
      announce(1,1,"CANCEL"); 
      return -1;
    } else if ( funKeyPressed(ENTER)) {
      cursor++;
      file[cursor++] = '.';
      file[cursor++] = 'M';
      file[cursor++] = 'I';
      file[cursor++] = 'C';
      file[cursor++] = 0;	

      return 0;
    } 
    
    if (cursor < 0)
      cursor = 0;
    else if (cursor > 7)
      cursor = 7;

    if (cursor > length - 1 && length < 9 ) {
      file[cursor] = file[cursor-1];
      length++;	
    }    
  }

}

void saveProgram() {

  int oldPc = pc;

  int aborted = createName();

  if ( aborted == -1 ) {
    reset(false);
    pc = oldPc;
    return;
  }

  saveProgram1(false, false); 

  // and also save once more, AUTO.MIC for recovery:
  saveProgram1(true, true); 

}

void saveProgram1(boolean autosave, boolean quiet) {   

  int oldPc = pc;
  // always start at 0 for save! to confusing otherwise! 
  pc = 0; 

  boolean fastLoad = false; 
  boolean transfer_mode = !autosave ? ( askQuestion("2095?") == ENTER ) : false; 

  if (! transfer_mode ) {
    if (autosave) 
      fastLoad = true; 
    else 
      fastLoad = ( askQuestion("Turbo?") == ENTER) ; 
  } else 
    fastLoad = false;   

  if (! autosave) {
    if (SD.exists(file)) {
      if ( askQuestion("Rplace?") == ENTER ) {      
	announce(0,1,"REPLACE");
	SD.remove(file); 
      } else {
	announce(1,1,"CANCEL"); 
	reset(false);
	pc = oldPc;
	return;
      }
    } else {
      announce(0,1,"SAVING");
    }

    if (fastLoad) {
      display.clearDisplay();
      setTextSize(1); 
      displaySetCursor(0, 0);
      display.print("Saving");
      displaySetCursor(0, 1);
      sep(); 
      displaySetCursor(0, 2);
      display.print(file);
      displaySetCursor(0, 3);
      sep(); 
    }

    cursor = CURSOR_OFF;

  } else if (! quiet) {
    displaySetCursor(0, 0);
    display.print("***");    
    display.display();
    delay(200); 
  }

  if (autosave) {
    if (SD.exists(autoloadsave_file)) {
      SD.remove(autoloadsave_file);
    }	
  }

  File myFile = SD.open( autosave ? autoloadsave_file : file , FILE_WRITE);

  if (transfer_mode) {
    resetPins();  
    delay(READ_DELAY_NEXT_VALUE);
    // start signal for transmission
    clock(BUSCH_IN3);
  }
  
  if (myFile) {

    myFile.print("# PC = ");
    myFile.print(pc);
    myFile.println();

    for (int i = pc; i < 256; i++) {

      if (! fastLoad) {
	display.clearDisplay();
	setTextSize(1); 
	displaySetCursor(0, 0);
	display.print("Saving");
	displaySetCursor(0, 1);
	sep(); 
	displaySetCursor(0, 2);
	display.print(file);
	displaySetCursor(0, 3);
	sep(); 
      }

      if (transfer_mode) {
	// read data from Microtronic 

	delay(READ_DELAY_NEXT_VALUE);

	int bit = clock(BUSCH_IN4);

	delay(READ_CLOCK_DELAY);

	int nibble = bit;

#ifdef MICRO_SECOND_GEN_BOARD 
// these are INPUT_PULLUP - hence, inverted!
	if ( digitalRead(BUSCH_OUT3) ) {
	  break;
	}
#endif 

#ifndef MICRO_SECOND_GEN_BOARD 
	if ( ! digitalRead(BUSCH_OUT3) ) {
	  break;
	}
#endif 

	setTextSize(2); 
	displaySetCursor(0, 2);
	display.print(pc / 16, HEX);
	display.print(pc % 16, HEX);
	display.print(" ");

	for (int ibit = 1; ibit < 12; ibit++) {
	  bit = clock(BUSCH_IN2);
	  int pos = ibit % 4;
	  nibble |= (bit << pos);
	  if (pos == 3) {
	    // here we have the read nibble: 
	    myFile.print(nibble, 16);
	    display.print(nibble,HEX); 
	    display.display(); 
	    nibble = 0;
	  }
	}

	clock(BUSCH_IN2);
	clock(BUSCH_IN2);

	myFile.println();

	if (i % 16 == 15)
	  myFile.println();

	pc = i;

      } else {
	myFile.print(op[i], HEX);
	myFile.print(arg1[i], HEX);
	myFile.print(arg2[i], HEX);
	myFile.println();

	if (i % 16 == 15)
	  myFile.println();

	pc = i;
      
	if (! fastLoad) {
	  setTextSize(2); 
	  status_row = 2; 
	  showMem(0); 
	  display.display(); 
	}
      }
    }

    if (! autosave) 
      announce(0, 1, "SAVED");

  } else {

    announce(0, 1, "ERROR");
    
  }

  myFile.close();

  if (transfer_mode) 
    resetPins();  

  if (! autosave) 
    reset(false);

  pc = oldPc;

}

void loadProgram() {

  // to enable loading via PGM1 Microtronic transfer... 
  // note inverted logic otherwise, they have to start with 
  // 0. First enter PGM1 on Microtronic 2nd Generation, 
  // then PGM 1 on Microtronic!
  resetPins();

  int aborted = selectFile();

  if ( aborted == -1 ) {
    reset(false);
    return;
  }

  loadProgram1(false, false); 

}

void loadProgram1(boolean load_autoloadsave_file, boolean quiet) {

  if (! quiet) {
    display.clearDisplay();
    setTextSize(1); 
    displaySetCursor(0, 0);
    display.print("Loading");
    displaySetCursor(0, 1);
    sep(); 
    displaySetCursor(0, 2);
    display.print(file);
    displaySetCursor(0, 3);
    sep(); 

    cursor = CURSOR_OFF;
  }

  boolean fastLoad = false; 
  boolean transfer_mode = !load_autoloadsave_file ? ( askQuestion("2095?") == ENTER ) : false; 

  if (! transfer_mode ) {
    if (load_autoloadsave_file) 
      fastLoad = true; 
    else 
      fastLoad = ( askQuestion("Turbo?") == ENTER) ; 
  } else 
    fastLoad = false;   

  if (! quiet)
    announce(0,1,"LOADING"); 

  SdFile myFile;

  myFile.open( load_autoloadsave_file ? autoloadsave_file : file, FILE_READ);

  int count = 0;
  int firstPc = -1;
  int oldPc = pc;

  if ( myFile.isOpen()) {

    boolean readingComment = false;
    boolean readingOrigin = false;
    boolean done = false; 

    if (transfer_mode) {
      resetPins();  
      delay(WRITE_DELAY_NEXT_VALUE);
    }

    while (! done) {

      int b = myFile.read();
      if (b == -1)
        break;

      if (b == '\n' || b == '\r') {
        readingComment = false;
        readingOrigin = false;
      } else if (b == '#') {
        readingComment = true;
      } else if (b == '@') {
        readingOrigin = true;
      }

      if (!readingComment && b != '\r' && b != '\n' && b != '\t' && b != ' ' && b != '@' ) { // skip whitespace

        switch ( b ) {
	case 'I' : b = '1'; break; // correct for some common OCR errors
	case 'l' : b = '1'; break;
	case 'P' : b = 'D'; break;
	case 'Q' : b = '0'; break;
	case 'O' : b = '0'; break;

          /*
            case 'a' : b = 'A'; break; // also allow lowercase hex
            case 'b' : b = 'B'; break;
            case 'c' : b = 'C'; break;
            case 'd' : b = 'D'; break;
            case 'e' : b = 'E'; break;
            case 'f' : b = 'F'; break; */
	default : break;
        }

        int decoded = decodeHex(b);
	
        if ( decoded == -1) {
	  if (! quiet) {
	    display.clearDisplay();
	    setTextSize(2); 
	    displaySetCursor(0, 0);
	    display.print("ERROR@");
	    displaySetCursor(0, 1);
	    display.print(pc, HEX);
	    display.print(':');
	    display.print(count, HEX);
	    displaySetCursor(0, 2);
	    display.print((char) b, HEX);
	    display.display(); 
	    delay(3000);
	  }
	  reset(quiet);
	  pc = firstPc;

	  if (transfer_mode) {
	    resetPins();  
	  }

	  return; 

        }

        if ( readingOrigin ) {
          switch ( count ) {
	  case 0 : pc = decoded * 16; count = 1; break;
	  case 1 :
	    pc += decoded;
	    count = 0;
	    readingOrigin = false;
	    break;
	  default : break;
          }

        } else {


          //
          // write data
          //

          switch ( count ) {

	  case 0 : 
	    if (transfer_mode) { 
	      delay(WRITE_DELAY_NEXT_VALUE);
	      storeNibble(decoded, true);           
	    } 
	    op[pc] = decoded; 
	    count = 1; 
	    break;

	  case 1 : 
	    if (transfer_mode) {
	      storeNibble(decoded, false);           
	    }
	    arg1[pc] = decoded; 
	    count = 2; 
	    break;

	  case 2 : 
	    if (transfer_mode) {
	      storeNibble(decoded, false);           	      
	      delay(WRITE_CLOCK_DELAY);
	    }
	    arg2[pc] = decoded; 
	    count = 0;

	    if (pc == 255)    
	      done = true; 
	    if (firstPc == -1) 
	      firstPc = pc;

	    pc++;
	    break;

	  default : break;
          }
        }

	if (! fastLoad) {
	  setTextSize(2); 
	  status_row = 2; 
	  showMem(0); 
	  display.display(); 
	}
      }
    }

    myFile.close();

    if (transfer_mode) 
      resetPins();  

    pc = firstPc;

    if (! quiet) 
      announce(0, 1, "LOADED");

    reset(quiet);
    pc = firstPc;
    return;

  } else {

    if (! quiet) 
      announce(0, 1, "ERROR");    

    reset(quiet); 
    pc = oldPc; 

    return;

  }

  // file couldn't be opened: 

  reset(quiet); 
  pc = oldPc; 

}



int initfile_readInt() {   
  int n = 0; 
  while (true) {
    int x = init_file.read(); 
    if (x == -1)
      return n; 
    if ( x == ' ') {
      return n;
    } else {
      n = n*10 + decodeHex(x); 
    }
  }	      
}


int initfile_readHex() {   
  int n = 0; 
  while (true) {
    int x = init_file.read(); 
    if (x == -1)
      return n; 
    if ( x == ' ') {
      return n;
    } else {
      n = n*16 + decodeHex(x); 
    }
  }	      
}

void initfile_readAutorunFilename() {   
  int n = 0; 
  while (true) {
    autoloadsave_file[n] = 0; 
    int x = init_file.read(); 
    if (x == -1 || x == ' ' || n == 14) {
      return; 
    } else {
      autoloadsave_file[n++] = (char) x; 
    }
  }	      
}

boolean loadMicrotronicInitFile() {

  init_file.open( "MICRO.INI", FILE_READ);

  if ( init_file.isOpen()) {

    init_cpu_delay = initfile_readInt();
    cpu_delay_delta = initfile_readInt(); 
    init_light_led = initfile_readInt(); 
    init_keybeep = initfile_readInt(); 
    init_displayMode = (LCDmode) initfile_readInt(); 
    autosave_every_seconds = initfile_readInt(); 
    init_autorun = initfile_readInt(); 
    init_autorun_address = initfile_readHex();       
    initfile_readAutorunFilename();       
    init_file.close();

    display.clearDisplay();
    setTextSize(1); 
    displaySetCursor(0, 0);
    display.print("MICRO.INI 1/2");
    displaySetCursor(0, 1);
    sep(); 
    displaySetCursor(0, 2);
    display.print("CPU       ");
    display.print(init_cpu_delay);
    displaySetCursor(0, 3);
    display.print("CPU DELTA ");
    display.print(cpu_delay_delta);
    displaySetCursor(0, 4);
    display.print("LIGHT     ");
    display.print(init_light_led);
    displaySetCursor(0, 5);
    display.print("KEYBEEP   ");
    display.print(init_keybeep);
    display.display(); 

    display.display(); 
    delay(2000); 

    display.clearDisplay();
    setTextSize(1); 
    displaySetCursor(0, 0);
    display.print("MICRO.INI 2/2");
    displaySetCursor(0, 1);
    sep(); 
    displaySetCursor(0, 2);
    display.print("AUTOSAVE  ");
    display.print(autosave_every_seconds);
    displaySetCursor(0, 3);
    display.print("AUTORUN   ");
    display.print(init_autorun);
    displaySetCursor(0, 4);
    display.print("AUTO ADDR ");
    display.print(init_autorun_address, HEX);
    displaySetCursor(0, 5);
    display.print(autoloadsave_file);
    display.display(); 
    delay(2000); 

    return true; 

  } else {
    display.clearDisplay();
    setTextSize(2); 
    displaySetCursor(0,0);
    display.print("SD OK!");
    setTextSize(1); 
    displaySetCursor(0, 2);
    sep(); 
    displaySetCursor(0, 3);
    display.print("  INIT FILE");
    displaySetCursor(0, 4);
    display.print("* MICRO.INI *");
    displaySetCursor(0, 5);
    display.print("  NOT FOUND!");
    display.display(); 
    delay(2000);

    return false; 

  }
 
}


//
//
//


void showMem(uint8_t col) {

  // show one line of RAM at PC 

  int adr = pc;

  clearLine(5); 
  refreshLCD = true;

  sendCharRow(0, status_row, status, false);  

  if ( currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW )
    adr = breakAt;

  sendHex(col++, adr / 16, blink & (cursor == 0));  
  sendHex(col++, adr % 16, blink & (cursor == 1));

  col++;

  sendHex(col++, op[adr], blink & (cursor == 2));
  sendHex(col++, arg1[adr], blink & (cursor == 3));
  sendHex(col++, arg2[adr], blink & (cursor == 4));

}


void showMemMore(uint8_t col) {

  // show 3 lines of RAM (PC-1, PC, PC+1) 

  int adr = pc;
  int adr0 = pc == 0 ? 255 : pc-1; 
  int adr1 = pc == 255 ? 0 : pc+1; 
  
  clearLines(3,5); 
  refreshLCD = true;

  sendCharRow(0, status_row, status, false);  

  if ( currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW )
    adr = breakAt;

  display.setTextColor(WHITE, BLACK); // 'inverted' text

  uint8_t col0 = col; 

  sendHexCol(col++, 4, adr / 16, blink & (cursor == 0));   
  sendHexCol(col++, 4, adr % 16, blink & (cursor == 1));

  col++;

  sendHexCol(col++, 4, op[adr], blink & (cursor == 2));
  sendHexCol(col++, 4, arg1[adr], blink & (cursor == 3));
  sendHexCol(col++, 4, arg2[adr], blink & (cursor == 4));

  //
  //

  col = col0; 

  display.setTextColor(BLACK, WHITE); // 'inverted' text

  sendHexCol(col++, 3, adr0 / 16, false);
  sendHexCol(col++, 3, adr0 % 16, false);

  col++; 

  sendHexCol(col++, 3, op[adr0]  , false);
  sendHexCol(col++, 3, arg1[adr0], false);
  sendHexCol(col++, 3, arg2[adr0], false);

  col = col0; 

  sendHexCol(col++, 5, adr1 / 16, false);
  sendHexCol(col++, 5, adr1 % 16, false);

  col++;

  sendHexCol(col++, 5, op[adr1]  , false);
  sendHexCol(col++, 5, arg1[adr1], false);
  sendHexCol(col++, 5, arg2[adr1], false);

}


//
//
//

void showTime(uint8_t col) {

  sendHex(col++, timeHours10, blink & (cursor == 0));
  sendHex(col++, timeHours1, blink & (cursor == 1));
  sendHex(col++, timeMinutes10, blink & (cursor == 2));
  sendHex(col++, timeMinutes1, blink & (cursor == 3));
  sendHex(col++, timeSeconds10, blink & (cursor == 4));
  sendHex(col++, timeSeconds1, blink & (cursor == 5));

}

void showDate(uint8_t col) {

  sendHex(col++, timeMonths10, blink & (cursor == 0));
  sendHex(col++, timeMonths1, blink & (cursor == 1));
  sendHex(col++, timeDays10, blink & (cursor == 2));
  sendHex(col++, timeDays1, blink & (cursor == 3));
  sendHex(col++, timeYears10, blink & (cursor == 4));
  sendHex(col++, timeYears1, blink & (cursor == 5));

}


void showReg(uint8_t col) {

  sendHex(col++, currentReg, blink & (cursor == 0));
  sendHex(col+3, reg[currentReg], blink & (cursor == 1));

}

void showProgram(uint8_t col) {
  sendHex(col, program, blink);
}

void showError(uint8_t col) {

  displaySetCursor(col, status_row); 
  
  if (blink) {
    display.setTextColor(WHITE, BLACK); 
  } else {
    display.setTextColor(BLACK, WHITE); 
  }
  display.print("ERROR"); 
  display.setTextColor(BLACK, WHITE); 

}

void announce(uint8_t x, uint8_t y, String message ) {

  display.clearDisplay();
  setTextSize(2); 
  displaySetCursor(x, y);	
  display.print(message);
  display.display(); 
  delay(250);
  display.clearDisplay();
  display.display(); 

}

void announce1(uint8_t x, uint8_t y, String message, byte num) {

  display.clearDisplay();
  setTextSize(2); 
  displaySetCursor(x, y);	
  display.print(message);
  if (num < 16)
    display.print(0);
  display.print(num,HEX);
  display.display(); 
  delay(500);
  display.clearDisplay();
  display.display(); 

}

//
// 
//


LCDmode lastDisplayMode = UNDEFLCD;
mode lastCurrentMode = UNDEFINED;
byte lastShowingDisplayFromReg = 0;
byte lastShowingDisplayDigits = 0; 
boolean lastDispOff = false;
boolean forceFullRefresh = false;

void showDISP(uint8_t col) {

  boolean mode_change = 
    forceFullRefresh ||
    lastDisplayMode != displayMode || 
    lastCurrentMode != currentMode || 
    lastShowingDisplayFromReg != showingDisplayFromReg || 
    lastShowingDisplayDigits != showingDisplayDigits || 
    lastDispOff != dispOff;

  showStatus(); 
  
  for (int i = 0; i < showingDisplayDigits; i++) {
    byte idx = (i +  showingDisplayFromReg ) % 16; 
    byte dreg = reg[idx];
    if ( mode_change || dispReg[idx] != dreg ) {
      sendHex(5 - i + col, dreg, false);
      dispReg[idx] = dreg; 
      refreshLCD = true;
    }
  }

  lastShowingDisplayFromReg = showingDisplayFromReg; 
  lastShowingDisplayDigits = showingDisplayDigits; 
  lastDispOff = dispOff; 
  forceFullRefresh = false; 
  
}

//
//
//

void displayStatus(boolean force_refresh) {

  refreshLCD = force_refresh;

  if (force_refresh) {
    display.clearDisplay(); 
    display.display(); 
    refreshLCD = true;
    forceFullRefresh = true; 
  }

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;

  if (delta > 200) {
    blink = !blink;
    lastDispTime = time;
  }

  //
  //
  //

  digitalWrite(CARRY_LED, carry);
  digitalWrite(ZERO_LED, zero);
  digitalWrite(CLOCK_1HZ_LED, clock1hz);

  // normal output for Microtronic Next Generation Board
#ifndef MICRO_SECOND_GEN_BOARD
  digitalWrite(DOT_LED_1, outputs & 1);
  digitalWrite(DOT_LED_2, outputs & 2);
  digitalWrite(DOT_LED_3, outputs & 4);
  digitalWrite(DOT_LED_4, outputs & 8);
#endif 

  // inverted output for Microtronic 2nd Generation board
#ifdef MICRO_SECOND_GEN_BOARD
  digitalWrite(DOT_LED_1, ! (outputs & 1));
  digitalWrite(DOT_LED_2, ! (outputs & 2));
  digitalWrite(DOT_LED_3, ! (outputs & 4));
  digitalWrite(DOT_LED_4, ! (outputs & 8));
#endif 

  //
  //
  //

#ifndef MICRO_SECOND_GEN_BOARD
  digitalWrite(CLOCK_OUT, clock1hz);

  digitalWrite(DOT_1, outputs & 1);
  digitalWrite(DOT_2, outputs & 2);
  digitalWrite(DOT_3, outputs & 4);
  digitalWrite(DOT_4, outputs & 8);
#endif 

  //
  // 
  // 

  if ( funKeyPressed(ENTER) || funKeyPressed(RIGHT)) {
    refreshLCD = true;
    forceFullRefresh = true; 
    switch ( displayMode  ) {
    case DISP    : displayMode = DISP_LARGE; display.clearDisplay(); break;
    case DISP_LARGE   : displayMode = MEM; display.clearDisplay(); break;
    case MEM   : displayMode = MEM_MNEM; display.clearDisplay(); break;
    case MEM_MNEM  : displayMode = REGWR; display.clearDisplay(); break;
    case REGWR  : displayMode = REGAR; display.clearDisplay(); break;
    default     : displayMode = DISP; display.clearDisplay(); break;
    }
  } else if ( funKeyPressed(LEFT)) {
    refreshLCD = true;
    forceFullRefresh = true; 
    switch ( displayMode  ) {
    case DISP    : displayMode =  REGWR; display.clearDisplay(); break;
    case DISP_LARGE   : displayMode = DISP; display.clearDisplay(); break;
    case MEM   : displayMode = DISP_LARGE; display.clearDisplay(); break;
    case MEM_MNEM  : displayMode = MEM; display.clearDisplay(); break;
    case REGWR  : displayMode = MEM_MNEM; display.clearDisplay(); break;
    default     : displayMode = REGWR; display.clearDisplay(); break;
    }
  } else if ( funKeyPressed(LIGHT)) {
    light_led = ! light_led; 
    digitalWrite(LIGHT_LED, light_led); 
  } 

  //
  //
  //

  if ( currentMode == RUNNING && dispOff && 
       displayMode != REGWR && displayMode != REGAR && displayMode != MEM_MNEM ) { 

    // no refresh / display updates when RUNNING and DISOUT and neither register nor mnemonics display mode
    // in these cases, we want to see what's going on, even with DISOUT 

  } else {

    status = ' ';

    if (currentMode == RUNNING)
      status = 'R';
    else if ( currentMode == STOPPED && ! error)
      status = 'H';
    else if (currentMode ==
	     ENTERING_ADDRESS_HIGH ||
	     currentMode ==
	     ENTERING_ADDRESS_LOW )
      status = 'A';
    else if (currentMode ==
	     ENTERING_BREAKPOINT_HIGH ||
	     currentMode ==
	     ENTERING_BREAKPOINT_LOW )
      status = 'B';
    else if (currentMode == ENTERING_OP ||
	     currentMode == ENTERING_ARG1 ||
	     currentMode == ENTERING_ARG2 )
      status = 'P';
    else if (currentMode == ENTERING_REG ||
	     currentMode == INSPECTING )
      status = 'I';
    else if (currentMode == ENTERING_VALUE )
      status = '?';
    else if (currentMode == ENTERING_TIME )
      status = 'T';
    else if (currentMode == ENTERING_DATE )
      status = 'D';
    else if (currentMode == SHOWING_TIME )
      status = 'C';
    else if (currentMode == ENTERING_PROGRAM )
      status = 'X';
    else status = ' ' ;

    //
    // set font size and cursor positions for potential display updates
    // 
    
    switch ( displayMode ) {
    case MEM :
    case MEM_MNEM : setTextSize(1); status_row = 4; status_col = 2; break; 
    case DISP_LARGE : setTextSize(2); status_row = 1; status_col = 1; break; 
    default : setTextSize(1); status_row = 5; status_col = 2; 
    }

    //
    // 
    //

    if (dispOff) {
      // display = F02 (DISOUT) 
      // we are in MNEMONICS or REGISTER DISPLAY mode -> show memory! 

      if (displayMode == MEM || displayMode == MEM_MNEM ) { 
	showMemMore(status_col); 
      } else {
	showMem(status_col); 
      }       
    } else {

      // display is on 
      // Fnm was sent 
      
      if ( currentMode == RUNNING && showingDisplayDigits > 0 ) { 
	showDISP(status_col);
      } else if ( currentMode == ENTERING_VALUE ) { 
        refreshLCD = true; 
	showDISP(status_col);
      } else if ( currentMode == ENTERING_REG || currentMode == INSPECTING ) {
        refreshLCD = true; 
	sendCharRow(0, status_row, status, false);      
	showReg(status_col);
      } else if ( currentMode == ENTERING_PROGRAM ) {
	refreshLCD = true; 
	sendCharRow(0, status_row, status, false);      
	showProgram(status_col);
      } else if ( currentMode == ENTERING_TIME || currentMode == SHOWING_TIME ) {
	refreshLCD = true; 
	sendCharRow(0, status_row, status, false);      
	showTime(status_col);
      } else if ( currentMode == ENTERING_DATE ) {
	refreshLCD = true; 
	sendCharRow(0, status_row, status, false);      
	showDate(status_col);
      } else if ( error ) {
	refreshLCD = true; 
	sendCharRow(0, status_row, status, false);      
	showError(status_col);
      } else if ( ! lastInstructionWasDisp || oneStepOnly ) {     
	if (displayMode == MEM || displayMode == MEM_MNEM ) { 
	  showMemMore(status_col); 
	} else {
	  showMem(status_col); 
	} 
      }    
    }

    //
    // this is only updated if needed (change in display mode or address) 
    //

    if ( pc != lastPc || jump || oneStepOnly || force_refresh) {

      lastPc = pc;

      if (displayMode == DISP || displayMode == MEM || displayMode == DISP_LARGE ) {

	switch ( currentMode ) {

        case ENTERING_TIME : 

	  refreshLCD = true; 

	  displaySetCursor(0, 0);	
          display.print("SET CLK");
          break;

        case ENTERING_DATE : 

	  refreshLCD = true; 

	  displaySetCursor(0, 0);	
          display.print("SET DATE");
          break;

        case SHOWING_TIME :

	  refreshLCD = true; 

	  displaySetCursor(0, 0);	
          display.print("TIME");
          break;

        default:
	  break; 	  
	}

      } else if (displayMode == MEM_MNEM ) {

	refreshLCD = true; 

	displaySetCursor(0, 0);
	display.print("PC MNEMONICS");

	displaySetCursor(0, 1);
	if (pc < 16)
	  display.print(0, HEX);
	display.print(pc, HEX);
     
	getMnem(1);

	displaySetCursor(3, 1);     
	display.print(mnemonic);

	displaySetCursor(0, 2); 
	sep(); 

      } else if (displayMode == REGWR ) {

	refreshLCD = true; 

	displaySetCursor(0, 0);
	display.print("WR 0123456789");

	displaySetCursor(0, 1);
	display.print("   "); 
	for (int i = 0; i < 10; i++)
	  display.print(reg[i], HEX);

	displaySetCursor(0, 2);
	display.print("WR ABCDEF");

	displaySetCursor(0, 3);
	display.print("   "); 
	for (int i = 10; i < 16; i++)
	  display.print(reg[i], HEX);      

	displaySetCursor(0, 4);
	sep(); 
      
      } else if (displayMode == REGAR ) {

	refreshLCD = true; 

	displaySetCursor(0, 0);
	display.print("ER 0123456789");

	displaySetCursor(0, 1);
	display.print("   "); 
	for (int i = 0; i < 10; i++)
	  display.print(regEx[i], HEX);

	displaySetCursor(0, 2);
	display.print("ER ABCDEF");

	displaySetCursor(0, 3);
	display.print("   "); 
	for (int i = 10; i < 16; i++)
	  display.print(regEx[i], HEX);      

	displaySetCursor(0, 4);
	sep(); 
      
      }
    }

    //
    // process keypress feedback display 
    //

    if (displayCurFunKey != NO_KEY && ! ( oneStepOnly && lastInstructionWasDisp)) {

      refreshLCD = true; 

      if (displayMode == DISP_LARGE) 
	displaySetCursor(0, status_row+1);
      else 
	displaySetCursor(9, status_row);

      switch ( displayCurFunKey ) {
      case CCE  : display.print("C/CE"); break;
      case RUN  :  display.print("RUN"); break; 
      case UP  : 
      case DOWN  :       
	if (cpu_changed) {
	  //display.print(cpu_speed); 
	  display.print(cpu_delay); 
          display.print("ms "); 
	} 
	break;
      case BKP  : display.print("BKP"); break;
      case NEXT : display.print("NEXT"); break;
      case PGM  : display.print("PGM"); break;
      case HALT : display.print("HALT"); break;
      case STEP : display.print("STEP"); break;
      case REG  : display.print("REG"); break;
      default: break;
      }

      if (millis() - dispFunKeyTime > 500) {
	cpu_changed = false; 

	displayCurFunKey = NO_KEY;
	if (displayMode == DISP_LARGE) 
	  clearLine(status_row+1); 
	else 
	  clearLineFrom(9, status_row); 
      }
    }
  }

  //
  //
  //
 
  if (refreshLCD || lastCurrentMode != currentMode || lastDisplayMode != displayMode ) 
    display.display(); 

  lastDisplayMode = displayMode; 
  lastCurrentMode = currentMode; 

  //
  //
  // 


}

//
//
//

void LCDLogo() {

  display.clearDisplay();; 
  sep(); 
  displaySetCursor(0, 1);
  display.print("    Busch     ");
  displaySetCursor(0, 2);
  display.print("  Microtronic ");
  displaySetCursor(0, 3);
  display.print("2nd Generation");
  displaySetCursor(0, 4);
  sep(); 
  displaySetCursor(0, 5);
  display.print("  Version ");
  display.print(VERSION); 
  display.display(); 
  delay(2000);   

  display.clearDisplay();; 
  displaySetCursor(0, 0);
  display.print("(C) ");
  display.print(DATE); 
  displaySetCursor(0, 1);
  sep();
  displaySetCursor(0, 2);
  display.print("Frank DeJaeger");
  displaySetCursor(0, 3);
  display.print("Manfred Henf");
  displaySetCursor(0, 4);
  display.print("Michael Wessel");
  displaySetCursor(0, 5);
  display.print("& Lilly!");

  display.display(); 
  delay(1000);   

  //
  //
  //

#ifdef RTCPRESENT

  DS3231_get(&RTC);

  display.clearDisplay();;   
  displaySetCursor(0, 0);
  sep(); 
  displaySetCursor(0, 1);
  display.print("D: ");
  display.print(RTC.mon / 10, DEC);
  display.print(RTC.mon % 10, DEC);
  display.print("/");
  display.print(RTC.mday / 10, DEC);
  display.print(RTC.mday % 10, DEC);
  display.print("/");
  display.print(RTC.year);
  displaySetCursor(0, 2);
  sep(); 
  displaySetCursor(0, 3);
  display.print("T: ");
  display.print(RTC.hour / 10, DEC);
  display.print(RTC.hour % 10, DEC);
  display.print(":");
  display.print(RTC.min / 10, DEC);
  display.print(RTC.min % 10, DEC);
  display.print(":");
  display.print(RTC.sec / 10, DEC);
  display.print(RTC.sec % 10, DEC);
  displaySetCursor(0, 4);
  sep();
  display.display(); 
 
  delay(1500);

#endif 

}

//
//
//

void sep() {
  display.print("--------------");
}

void setTextSize(int n) {
  textSize = n; 
  display.setTextSize(n); 
}

void clearLine(int line) {
  display.fillRect(0, 8*textSize*line, 6*textSize*14, 8*textSize,WHITE);		
}

void clearLineFrom(int x, int line) {
  display.fillRect(x*6*textSize, 8*textSize*line, 6*textSize*14, 8*textSize,WHITE);		
}

void clearLines(int from, int to) {
  display.fillRect(0, 8*from*textSize, 6*14*textSize, ((to-from)+1)*8*textSize,WHITE);		
}

void displaySetCursor(int x, int y) {
  display.setCursor(x*6*textSize, 8*y*textSize); 
}

void advanceCursor(boolean yes) {
  if (yes) lcdCursor ++;
}

void sendChar(uint8_t pos, char c, boolean blink) {
   
  displaySetCursor(pos, status_row); 
  display.print(blink ? ' ' : c); 

}

void sendCharRow(uint8_t pos, uint8_t row, char c, boolean blink) {
   
  displaySetCursor(pos, row); 
  display.print(blink ? ' ' : c); 

}

void showStatus() {

  if ( last_status != status ) {
    sendCharRow(0, status_row, status, false);      
    last_status = status; 
    refreshLCD = true;
  }

}

void sendHex(uint8_t pos, uint8_t c, boolean blink) {
   
  displaySetCursor(pos, status_row); 
  
  if (blink) {
    display.setTextColor(WHITE, BLACK); 
  } else {
    display.setTextColor(BLACK, WHITE); 
  }
  display.print(c, HEX);	
  display.setTextColor(BLACK, WHITE); 

}

void sendHexCol(uint8_t pos, uint8_t col, uint8_t c, boolean blink) {
   
  displaySetCursor(pos, col); 

  if (blink) {
    display.setTextColor(WHITE, BLACK); 
  } else {
    display.setTextColor(BLACK, WHITE); 
  }
  display.print(c, HEX);	
  display.setTextColor(BLACK, WHITE); 

}

//
//
//

void clearMnemonicBuffer() {

  for (int i = 0; i < 14; i++)
    mnemonic[i] = ' ';

  mnemonic[14] = 0;

  lcdCursor = 0;

}

void inputMnem(String string) {

  for (int i = 0; i < string.length(); i++)
    mnemonic[i + lcdCursor] = string.charAt(i);

  lcdCursor += string.length();

}

void getMnem(boolean spaces) {

  clearMnemonicBuffer();

  byte op1 = op[pc];
  byte hi = arg1[pc];
  byte lo = arg2[pc];
  byte op2 = op1 * 16 + hi;
  unsigned int op3 = op1 * 256 + hi * 16 + lo;

  switch ( op[pc] ) {
  case OP_MOV  : inputMnem("MOV   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_MOVI : inputMnem("MOVI  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_AND  : inputMnem("AND   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_ANDI : inputMnem("ANDI  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_ADD  : inputMnem("ADD   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_ADDI : inputMnem("ADDI  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_SUB  : inputMnem("SUB   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_SUBI : inputMnem("SUBI  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_CMP  : inputMnem("CMP   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_CMPI : inputMnem("CMPI  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_OR   : inputMnem("OR    ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
  case OP_CALL : inputMnem("CALL  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); inputMnem( hexStringChar[lo] ); break;
  case OP_GOTO : inputMnem("GOTO  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); inputMnem( hexStringChar[lo] ); break;
  case OP_BRC  : inputMnem("BRC   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); inputMnem( hexStringChar[lo] ); break;
  case OP_BRZ  : inputMnem("BRZ   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); inputMnem( hexStringChar[lo] ); break;
  default : {
    switch (op2) {
    case OP_MAS  : inputMnem( "MAS   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_INV  : inputMnem( "INV   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_SHR  : inputMnem( "SHR   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_SHL  : inputMnem( "SHL   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_ADC  : inputMnem( "ADC   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_SUBC : inputMnem( "SUBC  ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_DIN  : inputMnem( "DIN   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_DOT  : inputMnem( "DOT   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_KIN  : inputMnem( "KIN   ");  advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    default : {
      switch (op3) {
      case OP_HALT   : inputMnem( "HALT  ");  break;
      case OP_NOP    : inputMnem( "NOP   ");  break;
      case OP_DISOUT : inputMnem( "DISOUT");  break;
      case OP_HXDZ   : inputMnem( "HXDZ  ");  break;
      case OP_DZHX   : inputMnem( "DZHX  ");  break;
      case OP_RND    : inputMnem( "RND   ");  break;
      case OP_TIME   : inputMnem( "TIME  ");  break;
      case OP_RET    : inputMnem( "RET   ");  break;
      case OP_CLEAR  : inputMnem( "CLEAR ");  break;
      case OP_STC    : inputMnem( "STC   ");  break; 
      case OP_RSC    : inputMnem( "RSC   ");  break;
      case OP_MULT   : inputMnem( "MULT  ");  break;
      case OP_DIV    : inputMnem( "DIV   ");  break;
      case OP_EXRL   : inputMnem( "EXRL  ");  break;
      case OP_EXRM   : inputMnem( "EXRM  ");  break;
      case OP_EXRA   : inputMnem( "EXRA  ");  break;
      default        : inputMnem( "DISP  ");  advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo]); break;
      }
    }
    }
  }
  }

}

int decodeHex(char c) {

  if (c >= 65 && c <= 70 )
    return c - 65 + 10;
  else if ( c >= 48 && c <= 67 )
    return c - 48;
  else return -1;

}

//
//
//

void clearStack() {
  sp = 0;
}

void reset(boolean quiet) {

  noNewTone(TONEPIN); // Turn off the tone.

  curHexKeyRaw = NO_KEY; 
  curFunKeyRaw = NO_KEY; 

  light_led = init_light_led; 
  keybeep = init_keybeep; 

  digitalWrite(LIGHT_LED, light_led);		

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  cpu_delay = init_cpu_delay; 

  if (! quiet) 
    announce(1, 1, "RESET"); 

  for (currentReg = 0; currentReg < 16; currentReg++) {
    reg[currentReg] = 0;
    regEx[currentReg] = 0;
    dispReg[currentReg] = 255; 
  }

  last_status = '*';
  status = ' '; 

  lastDisplayMode = UNDEFLCD;
  lastCurrentMode = UNDEFINED;
  lastShowingDisplayFromReg = 0;
  lastShowingDisplayDigits = 0; 
  lastDispOff = false;

  currentReg = 0;
  currentInputRegister = 0;

  carry = false;
  zero = false;
  error = false;
  pc = 0;
  oneStepOnly = false;

  clearStack();

  curInput = 0;
  inputs = 0;
  outputs = 0;  

  displayMode = init_displayMode;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  dispOff = false;
  lastInstructionWasDisp = false;

  autosave_ticker = 0; 

  displayStatus(true); 

}

void clearMem() {

  announce(0, 1, "RAM 000"); 

  currentMode = STOPPED;
  cursor = CURSOR_OFF;
  status_row = 1; 

  // prevent overflow, loop termination! pc is byte...
  for (pc = 0; pc < 255; pc++) {

    op[pc] = 0;
    arg1[pc] = 0;
    arg2[pc] = 0;

  }

  op[pc] = 0;
  arg1[pc] = 0;
  arg2[pc] = 0;

  reset(false);

}

void loadNOPs() {

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  announce(0, 1, "RAM F01"); 

  for (pc = 0; pc < 255; pc++) {
    op[pc] = 15;
    arg1[pc] = 0;
    arg2[pc] = 1;
  }

  op[pc] = 15;
  arg1[pc] = 0;
  arg2[pc] = 1;

  reset(false);

}

//
//
//

void start_running() {
    
  display.clearDisplay();
  display.display();
  
  currentMode = RUNNING;
  cursor = CURSOR_OFF;
  oneStepOnly = false;
  dispOff = false;
  ignoreBreakpointOnce = true;
  
  clearStack();
  jump = true; // don't increment PC !
  //step();

}

void check_for_autosave() {

  if (autosave_every_seconds && 
      autosave_ticker >= autosave_every_seconds ) {
    saveProgram1(true, false); 
    autosave_ticker = 0; 
    displayStatus(true); 
  }

}

void interpret() {


  if ( funKeyPressed(HALT) ) {

    display.clearDisplay();
    display.display();

    jump = true;
    currentMode = STOPPED;
    cursor = CURSOR_OFF;

    dispOff = false;
    lastInstructionWasDisp = false;

  } else if (funKeyPressed(RUN)) {

    start_running(); 

  } else if ( funKeyPressed(DOWN)) {

    display.clearDisplay();
    display.display();

    if (currentMode != RUNNING ) {
      pc--;
      //cursor = 0;
      //currentMode = ENTERING_ADDRESS_HIGH;
    }

    jump = true;

  } else if ( funKeyPressed(UP)) {

    display.clearDisplay();
    display.display();

    if (currentMode != RUNNING ) {
      pc++;
      //cursor = 0;
      //currentMode = ENTERING_ADDRESS_HIGH;
    }

    jump = true;

  } else if ( funKeyPressed(NEXT)) {

    display.clearDisplay();
    display.display();

    if (currentMode == STOPPED) {
      currentMode = ENTERING_ADDRESS_HIGH;
      cursor = 0;
    } else {
      pc++;
      cursor = 2;
      currentMode = ENTERING_OP;
    }

    jump = true;

  } else if ( funKeyPressed(REG)) {
 
    display.clearDisplay();
    display.display();

    if (currentMode != ENTERING_REG) {
      currentMode = ENTERING_REG;
      cursor = 0;
    } else {
      currentMode = INSPECTING;
      cursor = 1;
    }

  } else if ( funKeyPressed(STEP) ) {

    display.clearDisplay();
    display.display();

    currentMode = RUNNING;
    oneStepOnly = true;
    dispOff = false;
    jump = true; // don't increment PC !

  } else if ( funKeyPressed(BKP) ) {

    display.clearDisplay();
    display.display();

    if (currentMode != ENTERING_BREAKPOINT_LOW ) {
      currentMode = ENTERING_BREAKPOINT_HIGH;
      cursor = 0;
    } else {
      cursor = 1;
      currentMode = ENTERING_BREAKPOINT_LOW;
    }

  } else if ( funKeyPressed(CCE) ) {

    display.clearDisplay();
    display.display();

    if (currentMode == ENTERING_ADDRESS_HIGH || 
        currentMode == ENTERING_ADDRESS_LOW || 
	currentMode == STOPPED ) {
      cursor = 2;
      //op[pc] = 0;
      currentMode = ENTERING_OP;
    } else if (cursor == 2) {
      cursor = 4;
      //arg2[pc] = 0;
      currentMode = ENTERING_ARG2;
    } else if (cursor == 3) {
      cursor = 2;
      //op[pc] = 0;
      currentMode = ENTERING_OP;
    } else {
      cursor = 3;
      //arg1[pc] = 0;
      currentMode = ENTERING_ARG1;
    }

  } else if ( funKeyPressed(PGM) ) {

    display.clearDisplay();
    display.display();

    if ( currentMode != ENTERING_PROGRAM ) {
      cursor = 0;
      currentMode = ENTERING_PROGRAM;
    }

  }

  //
  //
  //

  if ( currentMode == RUNNING ) {

    run();

  } else if (currentMode == STOPPED ) {

    check_for_autosave(); 

    cursor = CURSOR_OFF;

  } else if (currentMode == ENTERING_ADDRESS_HIGH ) {

    check_for_autosave(); 

    if (someHexKeyPressed()) {
      cursor = 1;
      pc = getCurHexKey() * 16;
      currentMode = ENTERING_ADDRESS_LOW;
    }

  } else if ( currentMode == ENTERING_ADDRESS_LOW ) {

    check_for_autosave(); 

    if (someHexKeyPressed()) {
      cursor = 2;
      pc += getCurHexKey();
      currentMode = ENTERING_OP;
    }
      
  } else if ( currentMode == ENTERING_OP ) {

    check_for_autosave(); 

    if (someHexKeyPressed()) {
      cursor = 3;
      op[pc] = getCurHexKey();
      currentMode = ENTERING_ARG1;
    }

  } else if ( currentMode == ENTERING_ARG1 ) {

    check_for_autosave(); 

    if (someHexKeyPressed()) {
      cursor = 4;
      arg1[pc] = getCurHexKey();
      currentMode = ENTERING_ARG2;
    }

  } else if ( currentMode == ENTERING_ARG2 ) {

    check_for_autosave(); 

    if (someHexKeyPressed()) {
      cursor = 2;
      arg2[pc] = getCurHexKey();
      currentMode = ENTERING_OP;
    }

  } else if (currentMode == ENTERING_VALUE ) {

    if (someHexKeyPressed()) {
      curInput = getCurHexKey();
      reg[currentInputRegister] = curInput;
      carry = false;
      zero = reg[currentInputRegister] == 0;
      currentMode = RUNNING;
    }

  } else if (currentMode == ENTERING_TIME ) {

    if (someHexKeyPressed()) {

#ifdef RTCPRESENT 

      DS3231_get(&RTC);
      timeHours10 = RTC.hour / 10; 
      timeHours1 = RTC.hour % 10; 
      timeMinutes10 = RTC.min / 10; 
      timeMinutes1 = RTC.min % 10; 
      timeSeconds10 = RTC.sec / 10; 
      timeSeconds1 = RTC.sec % 10; 

#endif 

      curInput = getCurHexKey();

      switch (cursor) {
      case 0 : if (curInput < 3) {
	  timeHours10 = curInput;
	  cursor++;
	} break;
      case 1 : if (timeHours10 == 2 && curInput < 4 || timeHours10 < 2 && curInput < 10) {
	  timeHours1 = curInput;
	  cursor++;
	} break;
      case 2 : if (curInput < 6) {
	  timeMinutes10 = curInput;
	  cursor++;
	} break;
      case 3 : if (curInput < 10) {
	  timeMinutes1 = curInput;
	  cursor++;
	} break;
      case 4 : if (curInput < 6) {
	  timeSeconds10 = curInput;
	  cursor++;
	} break;
      case 5 : if (curInput < 10) {
	  timeSeconds1 = curInput;
	  cursor++;
	} break;
      default : break;
      }

      if (cursor > 5)
	cursor = 0;

      //
      //
      //

#ifdef RTCPRESENT 
      RTC.hour = timeHours10*10 + timeHours1; 
      RTC.min = timeMinutes10*10 + timeMinutes1;
      RTC.sec = timeSeconds10*10 + timeSeconds1;
      DS3231_set(RTC); 
#endif 
      	  
    }

  } else if (currentMode == ENTERING_DATE ) {

#ifdef RTCPRESENT 

    if (someHexKeyPressed()) {

      DS3231_get(&RTC);

      timeYears1000  = RTC.year / 1000; 
      timeYears1000R = RTC.year % 1000; 
      timeYears100  = timeYears1000R / 100; 
      timeYears100R = timeYears1000R % 100; 
      timeYears10 = timeYears100R / 10; 
      timeYears1  = timeYears100R % 10;   
      timeMonths10 = RTC.mon / 10; 
      timeMonths1  = RTC.mon % 10;  
      timeDays10 = RTC.mday / 10; 
      timeDays1  = RTC.mday % 10; 

      curInput = getCurHexKey();

      // MONTH DAYS YEARS

      switch (cursor) {
      case 0 : if (curInput < 2) {
	  timeMonths10 = curInput;
	  cursor++;
	} break;
      case 1 : if (timeMonths10 == 1 && curInput < 3 || timeMonths10 == 0 && curInput > 0 && curInput < 10) {
	  timeMonths1 = curInput;
	  cursor++;
	} break;
      case 2 : if (curInput < 4) {
	  timeDays10 = curInput;
	  cursor++;
	} break;
      case 3 : if (timeDays10 == 3 && curInput < 2 || timeDays10 < 3 && curInput >=0 && curInput < 10) {
	  timeDays1 = curInput;
	  cursor++;
	} break;

      case 4 : if (curInput >= 0 && curInput < 10) {
	  timeYears10 = curInput;
	  cursor++;
	} break;
      case 5 :  if (curInput >= 0 && curInput < 10) {
	  timeYears1 = curInput;
	  cursor++;
	} break;
      default : break;
      }

      if (cursor > 5)
	cursor = 0;

      //
      //
      //

      timeYears1000 = 2; 
      timeYears100 = 0; 

      RTC.year = timeYears10 * 10 + timeYears1 + timeYears1000 * 1000 + timeYears100 * 100 ; 
      RTC.mon = timeMonths10*10 + timeMonths1;
      RTC.mday = timeDays10*10 + timeDays1;

      DS3231_set(RTC); 
      	  
    }

#endif 

  } else if (currentMode == ENTERING_PROGRAM ) {

    if (someHexKeyPressed()) {

      program = getCurHexKey();
      currentMode = STOPPED;
      cursor = CURSOR_OFF;

      if ( program == 0 ) {
	currentMode = ENTERING_DATE;
	cursor = 0;
	  
      } else if ( program == 1 ) {

	loadProgram();

      } else if ( program == 2) {

	saveProgram();

      } else if (program == 3 ) {
	currentMode = ENTERING_TIME;
	cursor = 0;

      } else if ( program == 4 ) {
	currentMode = SHOWING_TIME;
	cursor = CURSOR_OFF;

      } else if ( program == 5) {
	clearMem();

      } else if ( program == 6) {
	loadNOPs();

      } else if (program - 7 < PROGRAMS ) {

	enterProgram(program - 7, 0);
	announce(0,1,"LOADED");

      } else {
	announce(0,1,"NO PROG");
      }

    }

  } else if ( currentMode == ENTERING_BREAKPOINT_HIGH ) {

    if (someHexKeyPressed()) {
      cursor = 1;
      breakAt = getCurHexKey() * 16;
      currentMode = ENTERING_BREAKPOINT_LOW;

      if (breakAt == 0) 
	announce(0,1,"NO BKP"); 
      else
	announce1(0,1,"BKP@ ", breakAt); 

    }

  } else if ( currentMode == ENTERING_BREAKPOINT_LOW ) {

    if (someHexKeyPressed()) {
      cursor = 0;
      breakAt += getCurHexKey();
      currentMode = ENTERING_BREAKPOINT_HIGH;
	
      if (breakAt == 0) 
	announce(0,1,"NO BKP"); 
      else
	announce1(0,1,"BKP@ ", breakAt); 

    }

  } else if ( currentMode == ENTERING_REG ) {

    if (someHexKeyPressed())
      currentReg = getCurHexKey();

  } else if ( currentMode == INSPECTING ) {

    if (someHexKeyPressed())
      reg[currentReg] = getCurHexKey();

  }

}

void run() {

  if (!jump ) {
    pc++;
    displayStatus(false);
  }
	
  if ( !oneStepOnly && breakAt == pc && breakAt > 0 && ! ignoreBreakpointOnce )  {
    // breakpoint hit! 
    announce1(0,1,"BKP@ ", breakAt); 
    currentMode = STOPPED;
    return;
  }

  lastInstructionWasDisp = false;
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

  //
  // case switch doesn't work for such long case statements!! 
  // 

  if (op1 == OP_MOV) {

    reg[d] = reg[s];
    zero = reg[d] == 0;

    if (d == s) {
      if (! d) {
        noNewTone(TONEPIN); // Turn off the tone.
      } else { 
	NewTone(TONEPIN, note_frequencies_mov[d-1]); 
      }
    }
    
  } else if (op1 == OP_MOVI) {

    reg[d] = n;
    zero = reg[d] == 0;

  } else if (op1 == OP_AND ) {

    reg[d] &= reg[s];
    carry = false;
    zero = reg[d] == 0;

  } else if (op1 == OP_ANDI ) {

    reg[d] &= n;
    carry = false;
    zero = reg[d] == 0;

  } else if (op1 == OP_ADD ) {

    reg[d] += reg[s];
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;

  } else if (op1 == OP_ADDI ) {

    reg[d] += n;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero =  reg[d] == 0;

    if (! n) 
      NewTone(TONEPIN, note_frequencies_addi[d]);       

  } else if (op1 == OP_SUB ) {

    reg[d] -= reg[s];
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero =  reg[d] == 0;

  } else if (op1 == OP_SUBI ) {

    reg[d] -= n;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero =  reg[d] == 0;

    if (! n) 
      NewTone(TONEPIN, note_frequencies_subi[d]);       
      
  } else if (op1 == OP_CMP ) {

    carry = reg[s] < reg[d];
    zero = reg[s] == reg[d];

  } else if (op1 == OP_CMPI ) {

    carry = n < reg[d];
    zero = reg[d] == n;

  } else if (op1 == OP_OR ) {

    reg[d] |= reg[s];
    carry = false;
    zero = reg[d] == 0;

  } else if (op1 == OP_CALL ) {

    if (sp < STACK_DEPTH - 1) {
      stack[sp] = pc;
      sp++;
      pc = adr;
      jump = true;

    } else {

      error = true;
      currentMode = STOPPED;

    }

  } else if (op1 == OP_GOTO ) {

    pc = adr;
    jump = true;
    
  } else if (op1 == OP_BRC ) {

    if (carry) {
      pc = adr;
      jump = true;
    }

  } else if (op1 == OP_BRZ ) {

    if (zero) {
      pc = adr;
      jump = true;
    }

  } else if ( op2 == OP_MAS ) {

    regEx[d] = reg[d];

  } else if  (op2 == OP_INV ) {
    
    reg[d] ^= 15;
    
  } else if (op2 == OP_SHR ) {
    
    carry = reg[d] & 1;
    reg[d] >>= 1;
    zero = reg[d] == 0;
    
  } else if (op2 == OP_SHL ) {
    
    reg[d] <<= 1;
    carry = reg[d] & 16;
    reg[d] &= 15;
    zero = reg[d] == 0;
    
  } else if (op2 == OP_ADC ) {
    
    if (carry) reg[d]++;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;
    
  } else if (op2 == OP_SUBC ) {
    
    if (carry) reg[d]--;
    carry = reg[d] > 15;
    reg[d] &= 15;
    zero = reg[d] == 0;
    
  } else if (op2 == OP_DIN ) {
    
    reg[d] = readDIN();
    carry = false;
    zero = reg[d] == 0;
    
  } else if (op2 == OP_DOT ) {
    
    outputs = reg[dot_s];
    carry = false;
    zero = reg[dot_s] == 0;
    
  } else if (op2 == OP_KIN ) {

    display.clearDisplay();
    display.display();
    
    currentMode = ENTERING_VALUE;
    currentInputRegister = d;
        	      
  } else if (op3 == OP_HALT ) {

    display.clearDisplay();
    display.display();

    currentMode = STOPPED;

  } else if (op3 == OP_NOP ) {

  } else if (op3 == OP_DISOUT ) {

    display.clearDisplay();
    display.display();

    dispOff = true;
    showingDisplayFromReg = 0;
    showingDisplayDigits = 0;

  } else if (op3 == OP_HXDZ ) {
    
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
    
  } else if (op3 == OP_DZHX ) {

    num =
      reg[0xD] +
      10 * reg[0xE] +
      100 * reg[0xF];
    
    carry = false;
    zero = false;
    
    reg[0xD] = num % 16;
    reg[0xE] = ( num / 16 ) % 16;
    reg[0xF] = ( num / 256 ) % 16;
    
  } else if (op3 == OP_RND ) {

    reg[0xD] = random(16);
    reg[0xE] = random(16);
    reg[0xF] = random(16);
    
  } else if (op3 == OP_TIME ) {

    reg[0xA] = timeSeconds1;
    reg[0xB] = timeSeconds10;
    reg[0xC] = timeMinutes1;
    reg[0xD] = timeMinutes10;
    reg[0xE] = timeHours1;
    reg[0xF] = timeHours10;
    
  } else if (op3 == OP_RET ) {

    pc = stack[--sp] + 1;
    jump = true;

  } else if (op3 == OP_CLEAR ) {

    for (byte i = 0; i < 16; i ++)
      reg[i] = 0;
    
    carry = false;
    zero = true;
    
  } else if (op3 == OP_STC ) {

    carry = true;

  } else if (op3 == OP_RSC ) {

    carry = false;

  } else if (op3 == OP_MULT ) {

    num =
      reg[0] + 10 * reg[1] + 100 * reg[2] +
      1000 * reg[3] + 10000 * reg[4] + 100000 * reg[5];
    
    num2 =
      regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
      1000 * regEx[3] + 10000 * regEx[4] + 100000 * regEx[5];
    
    num *= num2;
    
    carry = num > 999999;
    
    for (int i = 0; i < 6; i++) {
      carry |= ( reg[i] > 9 || regEx[i] > 9 );
    }
    
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
    
    for (int i = 0; i < 6; i++) // not documented in manual, but true!
      regEx[i] = 0;

  } else if (op3 == OP_DIV ) {

    num2 =
      reg[0] + 10 * reg[1] + 100 * reg[2] +
      1000 * reg[3];
    
    num =
      regEx[0] + 10 * regEx[1] + 100 * regEx[2] +
      1000 * regEx[3];
    
    carry = false;
    
    for (int i = 0; i < 6; i++) {
      carry |= ( reg[i] > 9 || regEx[i] > 9 );
    }
    
    if (num2 == 0 || carry ) {
      
      carry = true;
      zero = false; 
	
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
      
      num3 = num % num2;
      zero = num3 > 0;
      
      regEx[0] = num3 % 10;
      regEx[1] = ( num3 / 10 ) % 10;
      regEx[2] = ( num3 / 100 ) % 10;
      regEx[3] = ( num3 / 1000 ) % 10;
      
    }
    
  } else if (op3 == OP_EXRL ) {

    for (int i = 0; i < 8; i++) {
      byte aux = reg[i];
      reg[i] = regEx[i];
      regEx[i] = aux;
    }

  } else if (op3 == OP_EXRM ) {

    for (int i = 8; i < 16; i++) {
      byte aux = reg[i];
      reg[i] = regEx[i];
      regEx[i] = aux;
    }
    
  } else if (op3 == OP_EXRA ) {

    for (int i = 0; i < 8; i++) {
      byte aux = reg[i];
      reg[i]   = reg[i + 8];
      reg[i + 8] = aux;
    }
    
  } else { 

    // DISP!

    dispOff = false;
    showingDisplayDigits = disp_n;
    showingDisplayFromReg = disp_s;
    lastInstructionWasDisp = true;
    
    if (oneStepOnly) {
      display.clearDisplay();
      showDISP(status_col);
      display.display();

      // delay(2000); 
      curFunKey = NO_KEY;
      while ( curFunKey == NO_KEY) {
	readFunKeys();      
      }	

      display.clearDisplay();
      display.display();
    }
  }

  //
  //
  //

  if (oneStepOnly) {
    currentMode = STOPPED;
    if (! jump) {
      pc++;
    }
  } else {

    delay(cpu_delay); 

  }

}

void enterProgram(int pgm, int start) {

  char* pgm_string = (char *) pgm_read_word(&PGMROM[pgm]); 

  while (pgm_read_byte(pgm_string)) {

    op[start]   = decodeHex(pgm_read_byte(pgm_string++)); 
    arg1[start] = decodeHex(pgm_read_byte(pgm_string++)); 
    arg2[start] = decodeHex(pgm_read_byte(pgm_string++)); 

    if (start == 255) {
      exit(1);
    }

    pgm_string++; // skip over space 
    start++; 

  };

}

//
// Auxiliary Operations for 2095 Emulation 
// Both PGM1 and PGM2 have been tested
// 

int clock(int pin) {

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(pin, LOW);
#else 
  digitalWrite(pin, HIGH);
#endif 

  delay(READ_CLOCK_DELAY);

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(pin, HIGH);
#else 
  digitalWrite(pin, LOW);
#endif 

  delay(READ_CLOCK_DELAY);

#ifdef MICRO_SECOND_GEN_BOARD 
// these are INPUT_PULLUP - hence, inverted!
  int bit = ! digitalRead(BUSCH_OUT1) ;
#endif 

#ifndef MICRO_SECOND_GEN_BOARD 
  int bit = digitalRead(BUSCH_OUT1) ;
#endif 

  return ( bit ? 1 : 0);

}

void clockWrite(int pin, int bit) {

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(BUSCH_IN1, bit);
#else
  digitalWrite(BUSCH_IN1, ! bit);
#endif

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(pin, HIGH);
#else
  digitalWrite(pin, LOW);
#endif 

  delay(WRITE_CLOCK_DELAY);

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(pin, LOW);
#else 
  digitalWrite(pin, HIGH);
#endif 

  delay(WRITE_CLOCK_DELAY);

}

void storeNibble(byte nibble, boolean first) {

  if (first) {

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
    digitalWrite(BUSCH_IN1, nibble & 0b0001);
#else 
    digitalWrite(BUSCH_IN1, ! (nibble & 0b0001));
#endif 

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
    digitalWrite(BUSCH_IN3, LOW);
#else 
    digitalWrite(BUSCH_IN3, HIGH);
#endif 

    delay(WRITE_CLOCK_DELAY);

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
    digitalWrite(BUSCH_IN3, HIGH);
#else 
    digitalWrite(BUSCH_IN3, LOW);
#endif 

    delay(WRITE_CLOCK_DELAY);

  } else {

    clockWrite(BUSCH_IN2, nibble & 0b0001);

  }

  clockWrite(BUSCH_IN2, nibble & 0b0010);
  clockWrite(BUSCH_IN2, nibble & 0b0100);
  clockWrite(BUSCH_IN2, nibble & 0b1000);

}

void resetPins() {

#ifndef INVERTED_OUTPUTS_FOR_2095_EMU
  digitalWrite(BUSCH_IN4, LOW);
  digitalWrite(BUSCH_IN3, LOW);
  digitalWrite(BUSCH_IN2, LOW);
  digitalWrite(BUSCH_IN1, LOW);
#else 
  digitalWrite(BUSCH_IN4, HIGH);
  digitalWrite(BUSCH_IN3, HIGH);
  digitalWrite(BUSCH_IN2, HIGH);
  digitalWrite(BUSCH_IN1, HIGH);
#endif 

}

//
// Setup Arduino
//

void setup() {

  //
  // RTC Setup
  // 

#ifdef RTCPRESENT 

  Wire.begin();
  DS3231_init(DS3231_INTCN);

  DS3231_get(&RTC);

  timeHours10 = RTC.hour / 10; 
  timeHours1 = RTC.hour % 10; 
  timeMinutes10 = RTC.min / 10; 
  timeMinutes1 = RTC.min % 10; 
  timeSeconds10 = RTC.sec / 10; 
  timeSeconds1 = RTC.sec % 10; 

  timeYears1000  = RTC.year / 1000; 
  timeYears1000R = RTC.year % 1000; 
  timeYears100  = timeYears1000R / 100; 
  timeYears100R = timeYears1000R % 100; 
  timeYears10 = timeYears100R / 10; 
  timeYears1  = timeYears100R % 10;   
  timeMonths10 = RTC.mon / 10; 
  timeMonths1  = RTC.mon % 10;  
  timeDays10 = RTC.mday / 10; 
  timeDays1  = RTC.mday % 10; 

#endif 
  
  initializeClock(); 

  //
  //
  //

  boolean loaded_initfile = false; 

  strcpy((char*) "AUTO.MIC", autoloadsave_file); 

  // Serial.begin(9600);

  pinMode(CLOCK_1HZ_LED, OUTPUT);
  pinMode(CARRY_LED,     OUTPUT);
  pinMode(ZERO_LED,      OUTPUT);

  digitalWrite(CLOCK_1HZ_LED, HIGH); 
  digitalWrite(CARRY_LED, HIGH); 
  digitalWrite(ZERO_LED, HIGH); 

  //
  //
  // 

  pinMode(TONEPIN, OUTPUT); 
  NewTone(TONEPIN, 400, 100); 

  //
  //
  //	

  pinMode(LIGHT_LED, OUTPUT);
  digitalWrite(LIGHT_LED, light_led);

  //
  // init displays
  //

  display.begin(); 
  display.setContrast(63);

  display.clearDisplay();

  setTextSize(1);
  display.setTextColor(BLACK);
  displaySetCursor(0,0);

  LCDLogo();

  //
  // SD Card init
  //

  setTextSize(2);
  
  if (! SD.begin(SDCARD_CHIP_SELECT)) {
    display.clearDisplay();
    displaySetCursor(0,1);
    display.print("SD BAD!");
    display.display(); 
    delay(2000);
  } else {
    loaded_initfile = loadMicrotronicInitFile();     
  }

  //
  //
  //

  display.clearDisplay();
  setTextSize(1);

  //
  // Configure keypads 
  //

  for (int x = 0; x < HEX_KEYPAD_ROWS; x++) {
    pinMode(hex_keypad_row_pins[x],OUTPUT);          
    digitalWrite(hex_keypad_row_pins[x],HIGH);          
  }     						       
  for (int x = 0; x < HEX_KEYPAD_COLS; x++) {
    pinMode(hex_keypad_col_pins[x],INPUT_PULLUP);          
  }     						       
	
  for (int x = 0; x < FN_KEYPAD_ROWS; x++) {
    pinMode(fn_keypad_row_pins[x],OUTPUT);          
    digitalWrite(fn_keypad_row_pins[x],HIGH);          
  }     						       
  for (int x = 0; x < FN_KEYPAD_COLS; x++) {
    pinMode(fn_keypad_col_pins[x],INPUT_PULLUP);          
  }     						       
	
  //
  // Configure 
  //

  pinMode(RESET,  INPUT_PULLUP);

  //
  //
    
  pinMode(DOT_LED_1, OUTPUT);
  pinMode(DOT_LED_2, OUTPUT);
  pinMode(DOT_LED_3, OUTPUT);
  pinMode(DOT_LED_4, OUTPUT);

  // 
  // 2095 Emulation Outputs 
  // in case DOT_LED_x == BUSCH_INx, these are
  // declared twice, but that doesn't matter really:
  //   
    
  pinMode(BUSCH_IN1, OUTPUT);
  pinMode(BUSCH_IN2, OUTPUT);
  pinMode(BUSCH_IN3, OUTPUT);
  pinMode(BUSCH_IN4, OUTPUT);

  //
  //
  // 

#ifndef MICRO_SECOND_GEN_BOARD
  pinMode(CLOCK_OUT, OUTPUT);

  pinMode(DOT_1, OUTPUT);
  pinMode(DOT_2, OUTPUT);
  pinMode(DOT_3, OUTPUT);
  pinMode(DOT_4, OUTPUT);
#endif 

  //
  //
  // 

#ifndef MICRO_SECOND_GEN_BOARD 
  pinMode(DIN_1, INPUT);
  pinMode(DIN_2, INPUT);
  pinMode(DIN_3, INPUT);
  pinMode(DIN_4, INPUT);

  pinMode(BUSCH_OUT1, INPUT);
  pinMode(BUSCH_OUT3, INPUT);
#endif

#ifdef MICRO_SECOND_GEN_BOARD 
  pinMode(DIN_1, INPUT_PULLUP);
  pinMode(DIN_2, INPUT_PULLUP);
  pinMode(DIN_3, INPUT_PULLUP);
  pinMode(DIN_4, INPUT_PULLUP);

  pinMode(BUSCH_OUT1, INPUT_PULLUP);
  pinMode(BUSCH_OUT3, INPUT_PULLUP);
#endif
 
  //
  //
  //

  pinMode(RANDOM_ANALOG_PIN,       INPUT);
  randomSeed(analogRead(RANDOM_ANALOG_PIN));
    
  //
  //
  //

  // attempt to load autoload file: 
  // this also ALWAYS does a quiet reset... 
  loadProgram1(true, true); 

  // run if specified in init file: 
  if (loaded_initfile) {

    pc = init_autorun_address; 
    lastPc = pc + 1; 

  }

}

//
// main loop / emulator / shell
//

void loop() {


  readFunKeys();
  readHexKeys(); 

  if ((currentMode == RUNNING || currentMode == ENTERING_VALUE)) {

    if (funKeyPressed(UP)) {   
      cpu_changed = true; 
      cpu_delay += cpu_delay_delta; 
      forceFullRefresh = true; 
    } else if (funKeyPressed(DOWN)) {
      cpu_changed = true; 
      cpu_delay = cpu_delay < cpu_delay_delta ? 0 : cpu_delay - cpu_delay_delta; 
      forceFullRefresh = true; 
    }		      
  }

  //
  //
  //

  displayStatus(false);
  interpret();

  //
  //
  //

  if (!digitalRead(RESET)) {
    reset(false);
  }

}
