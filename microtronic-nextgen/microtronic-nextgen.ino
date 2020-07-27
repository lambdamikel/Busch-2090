/*

  A Busch 2090 Microtronic Emulator for Arduino Mega 2560

  Version 4 (c) Michael Wessel, June 2020

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

#define VERSION "5"

//
//
//

#include <SdFat.h>

#include <SPI.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
// #include <EEPROM.h>
//#include <SoftwareSerial.h> 

//
// SD Card
//

#define SDCARD_CHIP_SELECT 53

SdFat SD;
SdFile root;

//
// hardware configuration / wiring
//

#define RESET  49 // soft reset 

//
// status LEDs
//
 
#define CARRY_LED     38
#define ZERO_LED      36
#define CLOCK_1HZ_LED 34

//
// LCD Backlight
// 

#define LIGHT_LED 35

//
// 1 Hz clock digital output
//

#define CLOCK_OUT 7 

//
// DOT digital output
//

#define DOT_LED_1 40
#define DOT_LED_2 42
#define DOT_LED_3 44
#define DOT_LED_4 46


#define DOT_1 25
#define DOT_2 27
#define DOT_3 29 
#define DOT_4 31 

//
// DIN digital input
//

#define DIN_1 24
#define DIN_2 26
#define DIN_3 28
#define DIN_4 30

//
// for initialization of random generator
//

#define RANDOM_ANALOG_PIN A5

//
// Nokia display 
//

Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 3, 2);

#define DISP_DELAY 200

//
// Function Push Buttons (not yet)
//

#define NO_KEY 0 

//
// HEX 4x4 matrix keypad 
//


#define HEX_KEYPAD_ROWS 4
#define HEX_KEYPAD_COLS 4

byte hex_keys[HEX_KEYPAD_ROWS][HEX_KEYPAD_COLS] = { // plus one because 0 = no key pressed!
  {13, 14, 15, 16},
  {9, 10, 11, 12},
  {5, 6, 7, 8},
  {1, 2, 3, 4}
};

byte hex_keypad_col_pins[HEX_KEYPAD_COLS] = {14, 12, 10, 8}; // columns
byte hex_keypad_row_pins[HEX_KEYPAD_ROWS] = {15, 13, 11, 9}; // rows

//
// Function 4x4 matrix keypad 
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

int curPushButton = NO_KEY;

#define FN_KEYPAD_ROWS 4
#define FN_KEYPAD_COLS 4 
 
byte fn_keys[FN_KEYPAD_ROWS][FN_KEYPAD_COLS] = { 
  {NEXT, REG,  LEFT,  RIGHT },
  {BKP,  STEP, UP,    DOWN },
  {RUN,  HALT, ENTER, CANCEL },
  {CCE,  PGM,  BACK,  LIGHT }
};

byte fn_keypad_row_pins[FN_KEYPAD_ROWS] = {23, 21, 19, 17}; 
byte fn_keypad_col_pins[FN_KEYPAD_COLS] = {22, 20, 18, 16}; 

//
// end of hardware configuration
//

//
// Display mode
//

enum LCDmode {
  OFF,
  OFF1, 
  OFF3, 
  PCMEM,
  REGWR, 
  REGAR
};

LCDmode displayMode = OFF3;

typedef char LCDLine[15]; // +1 for End of String!

LCDLine mnemonic;
LCDLine file;

int lcdCursor = 0;

int textSize = 1; 

uint8_t status_row = 0; 
uint8_t status_col = 0; 


//
// Hex keypad
//

#define DEBOUNCE_TIME 50

boolean keypadPressed = false;

unsigned long hexKeyTime = 0;

int curHexKey    = NO_KEY;
int curHexKeyRaw = NO_KEY;

//
// Function keypad 
//


unsigned long funcKeyTime = 0;
int curFuncKey    = NO_KEY;
int curFuncKeyRaw = NO_KEY;

int displayCurFuncKey = NO_KEY; // for LCD display feedback only


//
// current PGM program
//

byte program = 7;

//
// display and status variables
//

unsigned long lastClockTime = 0;
unsigned long last1HzTime = 0;
unsigned long lastDispTime = 0;

#define CURSOR_OFF 8
byte cursor = CURSOR_OFF;
boolean blink = true;

boolean dispOff = false;
boolean lastInstructionWasDisp = false;
byte showingDisplayFromReg = 0;
byte showingDisplayDigits = 0;

byte currentReg = 0;
byte currentInputRegister = 0;

boolean clock1hz = false;
boolean carry = false;
boolean zero = false;
boolean error = false;

boolean light_led = true;

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

//
// auxilary helper variables
//

unsigned long num = 0;
unsigned long num2 = 0;
unsigned long num3 = 0;

//
// PGM ROM Programs 
//

#define PROGRAMS 6

String PGMROM[PROGRAMS] = {

  // PGM 7 - NIM GAME 
  "F08 FE0 F41 FF2 FF1 FF4 045 046 516 FF4 854 D19 904 E19 \
B3F F03 0D1 0E2 911 E15 C1A 902 D1A 1F0 FE0 F00 F02 064 10C 714 B3F \
11A 10B C24 46A FBB 8AD E27 C29 8BE E2F 51C E2C C22 914 E2F C1C F03 \
0D1 0E2 F41 902 D09 911 E38 C09 1E2 1E3 1F5 FE5 105 FE5 C3A 01D 02E \
F04 64D FCE F07 ",

  // PGM 8 - crazy counter
  "F60 510 521 532 543 554 565 FE0 C00 ",

  // PGM 9 - electronic die 
  "F05 90D E00 96D D00 F1D FF0 C00 ",

  // PGM A - three digit counter with carry
  "F30 510 FB1 FB2 FE1 FE1 C00 ",

  // PGM B - scrolling LED light
  "110 F10 FE0 FA0 FB0 C02 ", 

  // PGM C - DIN digital input test
  "F10 FD0 FE0 C00 ", 
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

  ENTERING_REG,
  INSPECTING,

  ENTERING_VALUE,
  ENTERING_PROGRAM,

  ENTERING_TIME,
  SHOWING_TIME

};

mode currentMode = STOPPED;

boolean refreshLCD = false;

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

  return 0; 

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

  return 0; 

}

//
// setup Arduino
//

void setup() {

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

  pinMode(LIGHT_LED, OUTPUT);
  digitalWrite(LIGHT_LED, light_led);

  //
  // init displays
  //

  display.begin(); 
  display.setContrast(57);

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
    display.clearDisplay();
    displaySetCursor(0,1);
    display.print("SD OK!");
    display.display(); 
    delay(2000);
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
  //
  // 

  pinMode(CLOCK_OUT, OUTPUT);

  pinMode(DOT_1, OUTPUT);
  pinMode(DOT_2, OUTPUT);
  pinMode(DOT_3, OUTPUT);
  pinMode(DOT_4, OUTPUT);

  pinMode(DIN_1, INPUT);
  pinMode(DIN_2, INPUT);
  pinMode(DIN_3, INPUT);
  pinMode(DIN_4, INPUT);

  //
  //
  //

  pinMode(RANDOM_ANALOG_PIN,       INPUT);
  randomSeed(analogRead(RANDOM_ANALOG_PIN));

  //
  // 
  //

  lastClockTime = millis();
  last1HzTime = millis();

}

//
//
//

boolean enterPending = false;

int readPushButtons() {
    return readFunctionKeys(); 
}

int readFunctionKeys() {

  int button = fn_keypad_getKey(); 

  if ( button != curFuncKeyRaw ) {
    if (( millis() - funcKeyTime) > DEBOUNCE_TIME ) {
      funcKeyTime = millis();
      if (button != NO_KEY)
        displayCurFuncKey = button;
	curFuncKey = button;
	curPushButton = button; 
    }
  } else {
    curFuncKey = NO_KEY;
    curPushButton = NO_KEY; 
  }

  curFuncKeyRaw = button;

  return curFuncKey;

}


int readHexKeys() {

  int button = hex_keypad_getKey(); 

  keypadPressed = false; 
      
  // button change? check if pressed long enough -> change butotn state 
  if ( button != curHexKeyRaw ) {
    if (( millis() - hexKeyTime) > DEBOUNCE_TIME ) {
      hexKeyTime = millis();
      if (button != NO_KEY) {
        curHexKey = button-1;
        keypadPressed = true; 
      } else {
        curHexKey = NO_KEY; 
      }
    }
  }

  curHexKeyRaw = button; 

  return curHexKey;

}


byte readDIN() {

  return ( digitalRead(DIN_1)      |
           digitalRead(DIN_2) << 1 |
           digitalRead(DIN_3) << 2 |
           digitalRead(DIN_4) << 3   );

}

//
//
//

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

  readPushButtons();

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

    }

    readPushButtons();

    if (keypadPressed) {
    if ( curPushButton == UP) {
      if (no < count) 
	no = selectFileNo(no + 1); 
      else 
	no = selectFileNo(1); 
    } else if (curPushButton == DOWN) {
      if (no > 1) 
	no = selectFileNo(no - 1); 
      else 
	no = selectFileNo(count); 
    } else if (curPushButton == CANCEL) {
      announce(1,1,"CANCEL"); 
      return -1; 
    } else if (curPushButton == ENTER) {
      announce(0,1,"LOADING"); 
      return no; 
    } 
  }

  lastNo = no; 

  }

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

    change = false; 

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

    }

    readPushButtons();

    if (keypadPressed) {
      switch ( curPushButton ) {
      case UP : 
	file[cursor]++; 
	change = true;  
	break;
      case DOWN : 
	file[cursor]--; 
	change = true; 
	break;
      case LEFT : 
	cursor--; 
	change = true; 
	break;
      case RIGHT : 
	cursor++; 
	change = true; 
	break;
      case BACK : 
	if (cursor == length - 1 && cursor > 0) {
          file[cursor] = 0;
          length--;
          cursor--;
	  change = true; 
        } 
	break;
      case CANCEL : 
	announce(1,1,"CANCEL"); 
	return -1;
	break; 
      case ENTER : 	
	cursor++;
	file[cursor++] = '.';
	file[cursor++] = 'M';
	file[cursor++] = 'I';
	file[cursor++] = 'C';
	file[cursor++] = 0;	
	announce(0,1,"SAVING"); 

	return 0;
	break; 
      default : break;
      }
      
      if (cursor < 0)
	cursor = 0;
      else if (cursor > 7)
	cursor = 7;

      if (cursor > length - 1 && length < 9 ) {
	file[cursor] = '0';
	length++;	
      }
    }
  }
}

void saveProgram() {

  int oldPc = pc;

  int aborted = createName();

  if ( aborted == -1 ) {
    reset();
    pc = oldPc;
    return;
  }

  if (SD.exists(file)) {
    announce(0,1,"REPLACE");
  }

  File myFile = SD.open( file , FILE_WRITE);

  cursor = CURSOR_OFF;

  if (myFile) {

    myFile.print("# PC = ");
    myFile.print(pc);
    myFile.println();

    for (int i = pc; i < 256; i++) {

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

      myFile.print(op[i], HEX);
      myFile.print(arg1[i], HEX);
      myFile.print(arg2[i], HEX);
      myFile.println();
      if (i % 16 == 15)
        myFile.println();

      pc = i;

      setTextSize(2); 
      status_row = 2; 
      showMem(0); 
      display.display(); 

    }

    announce(0, 1, "SAVED");

  } else {

    announce(0, 1, "ERROR");
    
  }

  myFile.close();

  reset();
  pc = oldPc;

}


void loadProgram() {

  int aborted = selectFile();

  if ( aborted == -1 ) {
    reset();
    return;
  }

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

  SdFile myFile;

  myFile.open( file, FILE_READ);

  int count = 0;
  int firstPc = -1;
  int oldPc = pc;

  if ( myFile.isOpen()) {

    boolean readingComment = false;
    boolean readingOrigin = false;

    while (true) {

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
	  reset();
	  pc = firstPc;
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

          switch ( count ) {
            case 0 : op[pc] = decoded; count = 1;  break;
            case 1 : arg1[pc] = decoded; count = 2; break;
            case 2 :
              arg2[pc] = decoded;
              count = 0;
              if (firstPc == -1) firstPc = pc;
              pc++;
              break;
            default : break;
          }
        }

	setTextSize(2); 
	status_row = 2; 
	showMem(0); 
	display.display(); 

      }


    }

    myFile.close();
    pc = firstPc;
    announce(0, 1, "LOADED");

  } else {

    announce(0, 1, "ERROR"); 

  }
  
  reset();
  pc = firstPc;

}


//
//
//


void showMem(uint8_t col) {

  int adr = pc;

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

  int adr = pc;
  int adr0 = pc == 0 ? 255 : pc-1; 
  int adr1 = pc == 255 ? 0 : pc+1; 

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

void advanceTime() {

  if (currentMode != ENTERING_TIME) {

    unsigned long time = millis();
    unsigned long delta = time - lastClockTime;
    unsigned long delta2 = time - last1HzTime; 

    while (delta2 >= 500) {
      clock1hz = !clock1hz;
      delta2 -= 500;
      last1HzTime = time;
    }

    while (delta >= 1000) {

      delta -= 1000;
      
      timeSeconds1++;
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
    
    lastClockTime = time; 

    } 
  }
}


void showTime(uint8_t col) {

  sendHex(col++, timeHours10, blink & (cursor == 0));
  sendHex(col++, timeHours1, blink & (cursor == 1));
  sendHex(col++, timeMinutes10, blink & (cursor == 2));
  sendHex(col++, timeMinutes1, blink & (cursor == 3));
  sendHex(col++, timeSeconds10, blink & (cursor == 4));
  sendHex(col++, timeSeconds1, blink & (cursor == 5));

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

void showDISP(uint8_t col) {
  for (int i = 0; i < showingDisplayDigits; i++)
    sendHex(5 - i + col, reg[(i +  showingDisplayFromReg ) % 16], false);
}

//
//
//

void displayStatus() {

  refreshLCD = pc != lastPc || jump || oneStepOnly;

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;

  if (delta > 300) {
    blink = !blink;
    lastDispTime = time;
  }

  advanceTime();	

  //
  //
  //

  digitalWrite(CARRY_LED, carry);
  digitalWrite(ZERO_LED, zero);
  digitalWrite(CLOCK_1HZ_LED, clock1hz);

  digitalWrite(DOT_LED_1, outputs & 1);
  digitalWrite(DOT_LED_2, outputs & 2);
  digitalWrite(DOT_LED_3, outputs & 4);
  digitalWrite(DOT_LED_4, outputs & 8);

  //
  //
  //

  digitalWrite(CLOCK_OUT, clock1hz);

  digitalWrite(DOT_1, outputs & 1);
  digitalWrite(DOT_2, outputs & 2);
  digitalWrite(DOT_3, outputs & 4);
  digitalWrite(DOT_4, outputs & 8);

  //
  //
  //

  if (curPushButton == ENTER ) {
    refreshLCD = true;
    switch ( displayMode  ) {
    case OFF    : displayMode = OFF1; display.clearDisplay(); break;
    case OFF1   : displayMode = OFF3; display.clearDisplay(); break;
    case OFF3   : displayMode = PCMEM; display.clearDisplay(); break;
    case PCMEM  : displayMode = REGWR; display.clearDisplay(); break;
    case REGWR  : displayMode = REGAR; display.clearDisplay(); break;
    default     : displayMode = OFF; display.clearDisplay(); break;
    }
  }

  //
  //
  //

  if (curPushButton == LIGHT ) {
    light_led = ! light_led; 
    digitalWrite(LIGHT_LED, light_led); 
  } 

  //
  //
  //

  if ( currentMode == RUNNING && dispOff && 
       displayMode != REGWR && displayMode != REGAR && displayMode != PCMEM ) { 

    // no refresh / display updates when RUNNING and DISOUT and neither register nor mnemonics display mode
    // in these cases, we want to see what's going on, even with DISOUT 

  } else {

    char status = ' ';

    if ( currentMode == STOPPED && ! error)
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
    else if (currentMode == RUNNING)
      status = 'R';
    else if (currentMode == ENTERING_REG ||
	     currentMode == INSPECTING )
      status = 'I';
    else if (currentMode == ENTERING_VALUE )
      status = '?';
    else if (currentMode == ENTERING_TIME )
      status = 'T';
    else if (currentMode == SHOWING_TIME )
      status = 'C';
    else if (currentMode == ENTERING_PROGRAM )
      status = 'X';
    else status = ' ' ;

    //
    //	
    //	

    switch ( displayMode ) {
    case OFF3 :
    case PCMEM : setTextSize(1); status_row = 4; status_col = 2; clearLines(3,5); refreshLCD = true; break; 
    case OFF1 : setTextSize(2); status_row = 1; status_col = 1; clearLine(3); break; 
    default : setTextSize(1); status_row = 5; status_col = 2; clearLine(5); 
    }

    //
    // this is updated with every call to displayStatus() 
    // hence, we only clear parts of the display as necessary 
    // (1 or 3 lines) 
    // 

    if (dispOff) {
      // we are in MNEMONICS or REGISTER DISPLAY mode -> show memory! 
      sendCharRow(0, status_row, status, false);  
      if (displayMode == OFF3 || displayMode == PCMEM ) { 
	showMemMore(status_col); 
      } else {
	showMem(status_col); 
      }       
    } else {
      // display is on 
      
      if ( currentMode == RUNNING && showingDisplayDigits > 0 || currentMode == ENTERING_VALUE ) {
	sendCharRow(0, status_row, status, false);      
	showDISP(status_col);
      } else if ( currentMode == ENTERING_REG || currentMode == INSPECTING ) {
	sendCharRow(0, status_row, status, false);      
	showReg(status_col);
      } else if ( currentMode == ENTERING_PROGRAM ) {
	sendCharRow(0, status_row, status, false);      
	showProgram(status_col);
      } else if ( currentMode == ENTERING_TIME || currentMode == SHOWING_TIME ) {
	sendCharRow(0, status_row, status, false);      
	showTime(status_col);
      } else if ( error ) {
	sendCharRow(0, status_row, status, false);      
	showError(status_col);
      } else if ( ! lastInstructionWasDisp || oneStepOnly ) {     
	sendCharRow(0, status_row, status, false);  
	if (displayMode == OFF3 || displayMode == PCMEM ) { 
	  showMemMore(status_col); 
	} else {
	  showMem(status_col); 
	} 
      }    
    }

    //
    // this is only updated if needed (change in display mode or address) 
    //

    if ( refreshLCD ) {

      lastPc = pc;

      if (displayMode == OFF || displayMode == OFF3 || displayMode == OFF1 ) {

	switch ( currentMode ) {

        case ENTERING_TIME :
	  displaySetCursor(0, 0);	
          display.print("SET CLK");
          break;

        case SHOWING_TIME :
	  displaySetCursor(0, 0);	
          display.print("TIME");
          break;

        default:
	  break; 	  
	}

      } else if (displayMode == PCMEM ) {

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

	//showMemMore(status_col); 

      } else if (displayMode == REGWR ) {

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
    //
    //

    if (displayCurFuncKey != NO_KEY && ! ( oneStepOnly && lastInstructionWasDisp) ) {

      if (displayMode == OFF1) 
	displaySetCursor(0, status_row+1);
      else 
	displaySetCursor(9, status_row);

      switch ( displayCurFuncKey ) {
      case CCE  : display.print("C/CE"); break;
      case RUN  : display.print("RUN"); break;
      case BKP  : display.print("BKP"); break;
      case NEXT : display.print("NEXT"); break;
      case PGM  : display.print("PGM"); break;
      case HALT : display.print("HALT"); break;
      case STEP : display.print("STEP"); break;
      case REG  : display.print("REG"); break;
      default: break;
      }

      if (millis() - funcKeyTime > 500) {
	displayCurFuncKey = NO_KEY;
	if (displayMode == OFF1) 
	  clearLine(status_row+1); 
	else 
	  clearLine(status_row); 
      }

    }

    display.display(); 

  }


  refreshLCD = false;

}

//
//
//

void LCDLogo() {

  display.clearDisplay();; 

  sep(); 
  displaySetCursor(0, 1);
  display.print("Busch 2090    ");
  displaySetCursor(0, 2);
  display.print(" Microtronic  ");
  displaySetCursor(0, 3);
  display.print(" 2560 Emulator");
  displaySetCursor(0, 4);
  sep(); 
  displaySetCursor(0, 5);
  display.print("Baukastenforum");

  
  display.display(); 

  delay(2000);   

  display.clearDisplay();; 
  display.display(); 

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


void clearFileBuffer() {

  for (int i = 0; i < 14; i++)
    file[i] = ' ';

  file[14] = 0;

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

void reset() {

  currentMode = STOPPED;
  cursor = CURSOR_OFF;

  announce(1, 1, "RESET"); 

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
  oneStepOnly = false;

  clearStack();

  curInput = 0;
  inputs = 0;
  outputs = 0;  

  displayMode = OFF3;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  dispOff = false;
  lastInstructionWasDisp = false;

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

    /*
    clearLine(3);
    showMem(0);
    display.display(); 
    */
  }

  op[pc] = 0;
  arg1[pc] = 0;
  arg2[pc] = 0;

  /*
  clearLine(3);
  showMem(0);
  display.display(); 
  */

  reset();

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

  reset();

}

//
//
//

void interpret() {

  if ( curFuncKey == HALT ) {

      display.clearDisplay();
      display.display();

      jump = true;
      currentMode = STOPPED;
      cursor = CURSOR_OFF;

      dispOff = false;
      lastInstructionWasDisp = false;

  } else if  (curFuncKey == RUN ) {

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

  } else if ( curFuncKey == DOWN ) {

      display.clearDisplay();
      display.display();

      if (currentMode != RUNNING ) {
        pc--;
        cursor = 0;
        currentMode = ENTERING_ADDRESS_HIGH;
      }

      jump = true;

  } else if ( curFuncKey == UP ) {

      display.clearDisplay();
      display.display();

      if (currentMode != RUNNING ) {
        pc++;
        cursor = 0;
        currentMode = ENTERING_ADDRESS_HIGH;
      }

      jump = true;

  } else if ( curFuncKey == NEXT ) {

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

  } else if ( curFuncKey == REG ) {
 
      display.clearDisplay();
      display.display();

      if (currentMode != ENTERING_REG) {
        currentMode = ENTERING_REG;
        cursor = 0;
      } else {
        currentMode = INSPECTING;
        cursor = 1;
      }

  } else if ( curFuncKey == STEP ) {

      display.clearDisplay();
      display.display();

      currentMode = RUNNING;
      oneStepOnly = true;
      dispOff = false;
      jump = true; // don't increment PC !

  } else if ( curFuncKey == BKP ) {

      display.clearDisplay();
      display.display();

      if (currentMode != ENTERING_BREAKPOINT_LOW ) {
        currentMode = ENTERING_BREAKPOINT_HIGH;
        cursor = 0;
      } else {
        cursor = 1;
        currentMode = ENTERING_BREAKPOINT_LOW;
      }

  } else if ( curFuncKey == CCE ) {

      display.clearDisplay();
      display.display();

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

  } else if ( curFuncKey == PGM ) {

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

      cursor = CURSOR_OFF;

  } else if (currentMode == ENTERING_ADDRESS_HIGH ) {

      if (keypadPressed) {
        cursor = 1;
        pc = curHexKey * 16;
        currentMode = ENTERING_ADDRESS_LOW;
      }

  } else if ( currentMode == ENTERING_ADDRESS_LOW ) {

      if (keypadPressed) {
        cursor = 2;
        pc += curHexKey;
        currentMode = ENTERING_OP;
      }
      
  } else if ( currentMode == ENTERING_OP ) {

      if (keypadPressed) {
        cursor = 3;
        op[pc] = curHexKey;
        currentMode = ENTERING_ARG1;
      }

  } else if ( currentMode == ENTERING_ARG1 ) {

      if (keypadPressed) {
        cursor = 4;
        arg1[pc] = curHexKey;
        currentMode = ENTERING_ARG2;
      }

  } else if ( currentMode == ENTERING_ARG2 ) {

      if (keypadPressed) {
        cursor = 2;
        arg2[pc] = curHexKey;
        currentMode = ENTERING_OP;
      }

  } else if (currentMode == ENTERING_VALUE ) {

      if (keypadPressed) {

        curInput = curHexKey;
        reg[currentInputRegister] = curInput;
        carry = false;
        zero = reg[currentInputRegister] == 0;
        currentMode = RUNNING;
      }

  } else if (currentMode == ENTERING_TIME ) {

      if (keypadPressed ) {

        curInput = curHexKey;

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

      }

  } else if (currentMode == ENTERING_PROGRAM ) {

      if (keypadPressed) {

        program = curHexKey;
        currentMode = STOPPED;
        cursor = CURSOR_OFF;

	if ( program == 0 ) {

	  announce(0,1,"NO TEST");
	  
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

	  enterProgram(PGMROM[program - 7], 0);
	  announce(0,1,"LOADED");

	} else {
	  announce(0,1,"NO PROG");
        }

      }

  } else if ( currentMode == ENTERING_BREAKPOINT_HIGH ) {

      if (keypadPressed) {
        cursor = 1;
        breakAt = curHexKey * 16;
        currentMode = ENTERING_BREAKPOINT_LOW;

	if (breakAt == 0) 
	  announce(0,1,"NO BKP"); 
	else
	  announce1(0,1,"BKP@ ", breakAt); 

      }

  } else if ( currentMode == ENTERING_BREAKPOINT_LOW ) {

      if (keypadPressed) {
        cursor = 0;
        breakAt += curHexKey;
        currentMode = ENTERING_BREAKPOINT_HIGH;
	
	if (breakAt == 0) 
	  announce(0,1,"NO BKP"); 
	else
	  announce1(0,1,"BKP@ ", breakAt); 

      }

  } else if ( currentMode == ENTERING_REG ) {

      if (keypadPressed)
        currentReg = curHexKey;

  } else if ( currentMode == INSPECTING ) {

      if (keypadPressed)
        reg[currentReg] = curHexKey;

  }

}

void run() {

  if (!jump ) {
    pc++;
    displayStatus();
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
    
    reg[d] >>= 1;
    carry = reg[d] & 16;
    reg[d] &= 15;
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
      delay(2000); 
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
  }

}

void enterProgram(String string, int start) {

  int i = 0; 

  while ( string[i] ) {

    op[start]   = decodeHex(string[i++]);
    arg1[start] = decodeHex(string[i++]);
    arg2[start] = decodeHex(string[i++]);

    if (start == 255) {
      exit(1);
    }

    i++; // skip over space 
    start++; 

  };

}


//
// main loop / emulator / shell
//

void loop() {

  readFunctionKeys();
  readHexKeys(); 

  //
  //
  //

  displayStatus();
  interpret();

  //
  //
  //

  if (!digitalRead(RESET)) {
    reset();
  }

  //
  //
  //

  //  cpu_delay = (analogRead(CPU_THROTTLE_ANALOG_PIN) / CPU_THROTTLE_DIVISOR) / 10  * 10;

  int  cpu_delay = 0; 

  if (cpu_delay > 0)
    delay(cpu_delay);

}
