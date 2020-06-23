/*

  A Busch 2090 Microtronic Emulator for Arduino Mega 2560

  Version 4 (c) Michael Wessel, June 2020

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  The Busch Microtronic 2090 is (C) Busch GmbH
  See http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de

  Please run the PGM-EEPROM.ino sketch before running / loading this
  sketch into the Arduino. The emulator will not work properly
  otherwise. Note that PGM-EEPROM-v2.ino stores example programs into the
  EEPROM, and this sketch retrieve them from there.

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

#define VERSION "4"

// #define SDCARD // comment out if no SDCard shield present 

//
//
//

#if defined (SDCARD)
#include <SdFat.h>
#endif

#include <SPI.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <EEPROM.h>
//#include <SoftwareSerial.h> 

//
// SD Card
//

#if defined (SDCARD)
#define SDCARD_CHIP_SELECT 4

SdFat SD;
SdFile root;
#endif

//
// hardware configuration / wiring
//

#define RESET  49 // soft reset 

//
// status LEDs
//

#define CLOCK_1HZ_LED 48
#define CARRY_LED     50
#define ZERO_LED      52

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

int curPushButton    = NO_KEY;
int curPushButtonRaw = NO_KEY;

#define BACK   63
#define RIGHT  64
#define UP     65
#define DOWN   66
#define LEFT   67
#define CANCEL 68
#define ENTER  69



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
// Function 4x3 matrix keypad 
//

#define CCE  1 
#define PGM  2 

#define F1 3
#define F2 4

#define RUN  5
#define HALT 6  

#define F3 7
#define F4 8

#define BKP  9
#define STEP 10  

#define F5 11
#define F6 12

#define NEXT 13
#define REG  14 

#define F7 15
#define F8 16


#define FN_KEYPAD_ROWS 4
#define FN_KEYPAD_COLS 4 

byte fn_keys[FN_KEYPAD_ROWS][FN_KEYPAD_COLS] = { 
  {NEXT, REG,  F7, F8 },
  {BKP,  STEP, F5, F6 },
  {RUN,  HALT, F3, F4 },
  {CCE,  PGM,  F1, F2 }
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
  PCMEM,
  REGD
};

LCDmode displayMode = OFF;

typedef char LCDLine[21];

LCDLine mnemonic;
LCDLine file;
LCDLine speakFile;

int lcdCursor = 0;

//
// Hex keypad
//

#define DEBOUNCE_TIME 100

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
// EEPROM PGM read
// Please first use the sketch PGM-EEPROM
// to set up the PGM Microtronic ROM!
// otherwise, PGM 7 - PGM B cannot be loaded
// properly!
//

byte programs = 0;
int startAddresses[16];
int programLengths[16];

//
// current PGM program
//

byte program;

//
// display and status variables
//

unsigned long lastClockTime = 0;
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

int clock = 0;
boolean clock1hz = false;
boolean carry = false;
boolean zero = false;
boolean error = false;

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

void displaySetCursor(int x, int y) {
     display.setCursor(x*8, 8*y); 
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
  }

  return 0; 

}

//
// setup Arduino
//

void setup() {

  Serial.begin(9600);

  //
  // SD Card init
  //


#if defined (SDCARD)

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  if (! SD.begin(SDCARD_CHIP_SELECT, SPI_HALF_SPEED)) {
    display.clearDisplay();
    display.print("SD init failed!");
    delay(2000);
  }

#endif

  //
  // init displays
  //

  display.begin(); 
  display.setContrast(57);

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  displaySetCursor(0,0);

  LCDLogo();
  display.display(); 

  delay(2000);   

  //
  //
  //

  pinMode(CLOCK_1HZ_LED, OUTPUT);
  pinMode(CARRY_LED,     OUTPUT);
  pinMode(ZERO_LED,      OUTPUT);

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
  // read EEPROM PGMs meta data
  //

/*
  int adr = 0;
  programs = EEPROM.read(adr++);
  setDisplayToHexNumber(programs);
  delay(100);

  int start = 1;

  for (int n = 0; n < programs; n++) {
    startAddresses[n] = start + programs;
    programLengths[n] = EEPROM.read(adr++);
    start += programLengths[n];
    setDisplayToHexNumber( startAddresses[n]);
    delay(50);
  }

*/


}

//
//
//

boolean enterPending = false;


int readPushButtons() {

  curPushButton = NO_KEY;
  curPushButtonRaw = NO_KEY;

  return curPushButton;

}

int readFunctionKeys() {

  int button = fn_keypad_getKey(); 

  if ( button != curFuncKeyRaw ) {
    if (( millis() - funcKeyTime) > DEBOUNCE_TIME ) {
      curFuncKey = button;
      funcKeyTime = millis();
      if (button != NO_KEY)
        displayCurFuncKey = button;
        refreshLCD = true;
    }
  } else
    curFuncKey = NO_KEY;

  curFuncKeyRaw = button;

  return curFuncKey;

}


int readHexKeys() {

  int button = hex_keypad_getKey(); 
  keypadPressed = false; 

  if ( button != curHexKeyRaw ) {
    if (( millis() - hexKeyTime) > DEBOUNCE_TIME ) {
      hexKeyTime = millis();
      if (button != NO_KEY) {
        curHexKey = button-1;
        keypadPressed = true; 
       }
    }
  } else {
    curHexKey = NO_KEY;
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

int selectFileNo(int no) {

#if defined (SDCARD)
  int count = 0;

  clearFileBuffer();

  SD.vwd()->rewind();
  while (root.openNext(SD.vwd(), O_READ)) {
    if (! root.isDir()) {
      count++;
      root.getName(file, 20);
      if (count == no ) {
        root.close();
        return count;
      }
    }
    root.close();
  }
#endif
  return 0;

}


int countFiles() {

#if defined (SDCARD)
  int count = 0;

  SD.vwd()->rewind();
  while (root.openNext(SD.vwd(), O_READ)) {
    if (! root.isDir())
      count++;
    root.close();
  }

  return count;
#endif

}

int selectFile() {

  display.clearDisplay();

  int no = 1;
  int count = countFiles();
  selectFileNo(no);

  unsigned long last = millis();
  boolean blink = false;

  readPushButtons();

  while ( curPushButton != ENTER ) {

    displaySetCursor(0, 0);
    display.print("Load Program ");
    display.print(no);
    display.print(" / ");
    display.print(count);
    display.print("      "); // erase previoulsy left characters if number got smaller

    readPushButtons();

    if ( millis() - last > 100) {

      last = millis();
      displaySetCursor(0, 1);
      blink = !blink;

      if (blink)
        display.print("                ");
      else
        display.print(file);
    }

    switch ( curPushButton ) {
      case UP : if (no < count) no = selectFileNo(no + 1); else no = selectFileNo(1);  break;
      case DOWN : if (no > 1) no = selectFileNo(no - 1); else no = selectFileNo(count); break;
      case CANCEL : return -1; break;
      default : break;
    }
  }

  return 0;

}


int createName() {

  display.clearDisplay();

  clearFileBuffer();
  file[0] = 'P';

  int cursor = 0;
  int length = 1;

  unsigned long last = millis();
  boolean blink = false;

  readPushButtons();

  while ( curPushButton != ENTER ) {

    displaySetCursor(0, 0);
    display.print("Save Program");

    readPushButtons();

    if ( millis() - last > 100) {

      last = millis();

      displaySetCursor(0, 1);
      display.print(file);
      displaySetCursor(cursor, 1);

      blink = !blink;

      if (blink)
        if (file[cursor] == ' ' )
          display.print("_");
        else
          display.print(file[cursor]);
      else
        display.print("_");
    }

    switch ( curPushButton ) {
      case UP : file[cursor]++; break;
      case DOWN : file[cursor]--; break;
      case LEFT : cursor--; break;
      case RIGHT : cursor++; break;
      case BACK : if (cursor == length - 1 && cursor > 0) {
          file[cursor] = 0;
          length--;
          cursor--;
          display.clearDisplay();
        } break;
      case CANCEL : return -1;
      default : break;
    }

    if (cursor < 0)
      cursor = 0;
    else if (cursor > 7)
      cursor = 7;

    if (cursor > length - 1 ) {
      file[cursor] = '0';
      length++;
    }
  }

  cursor++;
  file[cursor++] = '.';
  file[cursor++] = 'M';
  file[cursor++] = 'I';
  file[cursor++] = 'C';
  file[cursor++] = 0;

  return 0;

}

void saveProgram() {

#if defined (SDCARD)

  int oldPc = pc;

  int aborted = createName();

  if ( aborted == -1 ) {
    display.clearDisplay();
    display.print("*** ABORT ***");
    delay(500);
    reset();
    pc = oldPc;
    return;
  }

  if (SD.exists(file)) {
    display.clearDisplay();
    displaySetCursor(0, 0);
    display.print("Overwriting");
    displaySetCursor(0, 1);
    display.print(file);
    SD.remove(file);
    delay(500);
  }

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("Saving");
  displaySetCursor(0, 1);
  display.print(file);


  File myFile = SD.open( file , FILE_WRITE);

  cursor = CURSOR_OFF;

  if (myFile) {

    myFile.print("# PC = ");
    myFile.print(pc);
    myFile.println();

    int curX = 0;
    int curY = 1;

    for (int i = pc; i < 256; i++) {

      delay(10);

      displaySetCursor(8, 0);
      display.print("@ ");
      if ( i < 16)
        display.print("0");
      display.print(i, HEX);

      myFile.print(op[i], HEX);
      myFile.print(arg1[i], HEX);
      myFile.print(arg2[i], HEX);
      myFile.println();
      if (i % 16 == 15)
        myFile.println();

      pc = i;
      showMem();

      displaySetCursor(curX, curY);

      display.print(op[i], HEX);
      display.print(arg1[i], HEX);
      display.print(arg2[i], HEX);
      display.print("-");

      curX += 4;
      if (curX == 20) {
        curX = 0;
        curY ++;
        if (curY == 4) {
          curY = 1;
          displaySetCursor(0, 1);
          display.print("                    ");
          displaySetCursor(0, 2);
          display.print("                    ");
          displaySetCursor(0, 3);
          display.print("                    ");
        }
      }
    }

    display.clearDisplay();
    displaySetCursor(0, 0);
    display.print("Saved");
    displaySetCursor(0, 1);
    display.print(file);

  } else {
    display.clearDisplay();
    display.print("*** ERROR ! ***");
  }

  display.clearDisplay();
  myFile.close();

  reset();
  pc = oldPc;

#endif

}


void loadProgram() {

#if defined (SDCARD)
  display.clearDisplay();

  int aborted = selectFile();

  if ( aborted == -1 ) {
    display.clearDisplay();
    display.print("*** ABORT ***");

    reset();
    return;
  }

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("Loading");
  displaySetCursor(0, 1);
  display.print(file);


  cursor = CURSOR_OFF;

  SdFile myFile;

  myFile.open( file, FILE_READ);

  int count = 0;
  int firstPc = -1;
  int oldPc = pc;

  if ( myFile.isOpen()) {

    boolean readingComment = false;
    boolean readingOrigin = false;

    int curX = 0;
    int curY = 1;

    while (true) {

      displaySetCursor(8, 0);
      display.print("@ ");
      if (pc < 16)
        display.print("0");
      display.print(pc, HEX);
      delay(10);

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
          display.print("*** ERROR ! ***");
          displaySetCursor(0, 1);
          display.print("Byte ");
          display.print(count);
          display.print(" ");
          display.print((char) b);
          delay(2000);
          display.clearDisplay();
          break;
        }

        if ( readingOrigin ) {
          switch ( count ) {
            case 0 : pc = decoded * 16; count = 1; break;
            case 1 :
              pc += decoded;
              showMem();
              count = 0;
              readingOrigin = false;
              displaySetCursor(curX, curY);
              display.write("@");
              if (pc < 16)
                display.print("0");
              display.print(pc, HEX);
              display.print("-");
              curX += 4;
              if (curX == 20) {
                curX = 0;
                curY ++;
                if (curY == 4) {
                  curY = 1;
                  displaySetCursor(0, 1);
                  display.print("                    ");
                  displaySetCursor(0, 2);
                  display.print("                    ");
                  displaySetCursor(0, 3);
                  display.print("                    ");
                }
              }

              break;
            default : break;
          }

        } else {

          displaySetCursor(curX, curY);
          display.write(b);

          curX++;

          switch ( count ) {
            case 0 : op[pc] = decoded; count = 1;  break;
            case 1 : arg1[pc] = decoded; count = 2; break;
            case 2 :
              arg2[pc] = decoded;
              showMem();
              count = 0;
              if (firstPc == -1) firstPc = pc;
              pc++;

              displaySetCursor(curX, curY);
              display.write("-");

              curX++;
              if (curX == 20) {
                curX = 0;
                curY++;
                if (curY == 4) {
                  curY = 1;
                  displaySetCursor(0, 1);
                  display.print("                    ");
                  displaySetCursor(0, 2);
                  display.print("                    ");
                  displaySetCursor(0, 3);
                  display.print("                    ");
                }
              }

              break;

            default : break;
          }

        }
      }
    }

    myFile.close();

    pc = firstPc;

    display.clearDisplay();
    displaySetCursor(0, 0);
    display.print("Loaded @ ");
    if (pc < 16)
      display.print("0");
    display.print(pc, HEX);
    displaySetCursor(0, 1);
    display.print(file);

    showLoaded(false);


  } else {
    display.clearDisplay();
    display.print("*** ERROR ! ***");

  }

  // display.clearDisplay();

  reset();
  pc = firstPc;
#endif

}


//
//
//


void sendString(String string) {

  clearStatus(); 

  for (int i = 0; i < string.length(); i++) {
    sendChar(i, string[i], false);
  }

  display.display();

  delay(DISP_DELAY);

}



void showMem() {

  int adr = pc;

  if ( currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW )
    adr = breakAt;

  if (cursor == 0)
    sendHex(2, adr / 16, blink); 
  else
    sendHex(2, adr / 16 , false);

  if (cursor == 1)
    sendHex(3, adr % 16, blink);
  else
    sendHex(3, adr % 16, false);


  if (cursor == 2)
    sendHex(5, op[adr], blink);
  else
    sendHex(5, op[adr], false);

  if (cursor == 3)
    sendHex(6, arg1[adr], blink);
  else
    sendHex(6, arg1[adr], false);

  if (cursor == 4)
    sendHex(7, arg2[adr], blink);
  else
    sendHex(7, arg2[adr], false);

  display.display();

}

void advanceTime() {

  if (currentMode != ENTERING_TIME) {

    unsigned long time = millis();
    long delta = time - lastClockTime;

    while (delta >= 1000) {

      clock1hz = !clock1hz;
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


void showTime() {

  if (cursor == 0)
    sendHex(2, timeHours10, blink);
  else
    sendHex(2, timeHours10, false);

  if (cursor == 1)
    sendHex(3, timeHours1, blink);
  else
    sendHex(3, timeHours1, false);

  if (cursor == 2)
    sendHex(4, timeMinutes10, blink);
  else
    sendHex(4, timeMinutes10, false);

  if (cursor == 3)
    sendHex(5, timeMinutes1, blink);
  else
    sendHex(5, timeMinutes1, false);

  if (cursor == 4)
    sendHex(6, timeSeconds10, blink);
  else
    sendHex(6, timeSeconds10, false);

  if (cursor == 5)
    sendHex(7, timeSeconds1, blink);
  else
    sendHex(7, timeSeconds1 , false);

  display.display();


}


void showReg() {

  if (cursor == 0)
    sendHex(3, currentReg, blink);
  else
    sendHex(3, currentReg, false);

  if (cursor == 1)
    sendHex(7, reg[currentReg], blink);
  else
    sendHex(7, reg[currentReg], false);

  display.display();

}

void showProgram() {

  sendHex(7, program, blink);
  display.display();

}

void showError() {

  if (blink)
    sendString("  Error  ");

  display.display();

}


void showReset() {

  sendString("  reset ");
  display.display();

}

void showDISP() {

  for (int i = 0; i < showingDisplayDigits; i++)
    sendHex(7 - i, reg[(i +  showingDisplayFromReg ) % 16], false);

  display.display();

}

void displayStatus() {

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;

  if (delta > 300) {

    blink = !blink;
    lastDispTime = time;

    advanceTime();

  }

  clock ++;
  clock %= 10;

  digitalWrite(CARRY_LED, carry);
  digitalWrite(ZERO_LED, carry);
  digitalWrite(CLOCK_1HZ_LED, clock1hz);

  digitalWrite(CLOCK_OUT, clock1hz);

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
    status = 'b';
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
  else if (currentMode == ENTERING_TIME )
    status = 't';
  else if (currentMode == SHOWING_TIME )
    status = 'C';
  else status = ' ' ;

  digitalWrite(DOT_LED_1, outputs & 1);
  digitalWrite(DOT_LED_2, outputs & 2);
  digitalWrite(DOT_LED_3, outputs & 4);
  digitalWrite(DOT_LED_4, outputs & 8);

  digitalWrite(DOT_1, outputs & 1);
  digitalWrite(DOT_2, outputs & 2);
  digitalWrite(DOT_3, outputs & 4);
  digitalWrite(DOT_4, outputs & 8);

  //
  //
  //

  clearStatus(); 
 
  sendChar(0, status, false); 

  if ( currentMode == RUNNING && ! dispOff && showingDisplayDigits > 0 || currentMode == ENTERING_VALUE )
    showDISP();
  else if  ( currentMode == RUNNING && dispOff ) {
    // nothing
  } else if ( currentMode == ENTERING_REG || currentMode == INSPECTING )
    showReg();
  else if ( currentMode == ENTERING_PROGRAM )
    showProgram();
  else if ( currentMode == ENTERING_TIME || currentMode == SHOWING_TIME )
    showTime();
  else if ( error )
    showError();
  else if ( ! dispOff && ! lastInstructionWasDisp )
    showMem();

  //
  //
  //

  updateLCD();

}

void LCDLogo() {

  clearLCD(); 

  displaySetCursor(0, 0);
  display.print("Busch 2090");
  displaySetCursor(0, 1);
  display.print(" Microtronic");
  displaySetCursor(0, 2);
  display.print("  Mega 2560");

}

void clearLine(int line) {
     display.fillRect(0, 8*line, 8*12, 8,WHITE);		
}

void clearStatus() {
     clearLine(5); 
}

void clearLCD() {
     clearLine(0); 
     clearLine(1); 
     clearLine(2); 
     clearLine(3); 
     clearLine(4); 
}


void clearMnemonicBuffer() {

  for (int i = 0; i < 20; i++)
    mnemonic[i] = ' ';

  mnemonic[20] = 0;

  lcdCursor = 0;

}


void clearFileBuffer() {

  for (int i = 0; i < 20; i++)
    file[i] = ' ';

  file[20] = 0;

  lcdCursor = 0;

}


void advanceCursor(boolean yes) {

  if (yes) lcdCursor ++;

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
    case OP_CALL : inputMnem("CALL  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_GOTO : inputMnem("GOTO  ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_BRC  : inputMnem("BRC   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
    case OP_BRZ  : inputMnem("BRZ   ") ; advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); inputMnem( hexStringChar[lo] ); break;
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
                case OP_HALT   : inputMnem( "HALT    ");  break;
                case OP_NOP    : inputMnem( "NOP     ");  break;
                case OP_DISOUT : inputMnem( "DISOUT  ");  break;
                case OP_HXDZ   : inputMnem( "HXDZ    ");  break;
                case OP_DZHX   : inputMnem( "DZHX    ");  break;
                case OP_RND    : inputMnem( "RND     ");  break;
                case OP_TIME   : inputMnem( "TIME    ");  break;
                case OP_RET    : inputMnem( "RET     ");  break;
                case OP_CLEAR  : inputMnem( "CLEAR   ");  break;
                case OP_STC    : inputMnem( "STC     ");  break;
                case OP_RSC    : inputMnem( "RSC     ");  break;
                case OP_MULT   : inputMnem( "MULT    ");  break;
                case OP_DIV    : inputMnem( "DIV     ");  break;
                case OP_EXRL   : inputMnem( "EXRL    ");  break;
                case OP_EXRM   : inputMnem( "EXRM    ");  break;
                case OP_EXRA   : inputMnem( "EXRA    ");  break;
                default        : inputMnem( "DISP  ");  advanceCursor(spaces); inputMnem( hexStringChar[hi] ); advanceCursor(spaces); hexStringChar[lo]; break;
              }
            }
        }
      }
  }

}


void updateLCD() {

  readPushButtons();

  if (curPushButton == ENTER ) {
    refreshLCD = true;
    switch ( displayMode  ) {
      case OFF    : displayMode = PCMEM; clearLCD(); break;
      case PCMEM  : displayMode = REGD; clearLCD(); break;
      default     : displayMode = OFF; clearLCD(); break;
    }
  }

  if ( pc != lastPc || refreshLCD ) {

    lastPc = pc;

    if (displayMode == OFF && refreshLCD ) {

      switch ( currentMode ) {

        case ENTERING_TIME :
          clearLCD(); 
          display.print("PGM 3 ENTER TIME");
          break;

        case SHOWING_TIME :
	  clearLCD(); 
          display.print("PGM 4 SHOW TIME");
          break;

        default:

          // LCDLogo();
	  break; 
	  
      }

    } else if (displayMode == PCMEM ) {

      displaySetCursor(0, 0);
      display.print("PC BKP OP  MNEM");
      displaySetCursor(0, 1);

      if (pc < 16)
        display.print(0);
      display.print(pc, HEX);

      displaySetCursor(4, 1);

      if (breakAt < 16)
        display.print(0);
      display.print(breakAt, HEX);

      displaySetCursor(7, 1);
      display.print(op[pc], HEX);
      display.print(arg1[pc], HEX);
      display.print(arg2[pc], HEX);

      displaySetCursor(11, 1);

      getMnem(false);

      display.print(mnemonic);
      displaySetCursor(0, 2);
      display.print("WR ");
      for (int i = 0; i < 16; i++)
        display.print(reg[i], HEX);
      displaySetCursor(0, 3);
      display.print("SR ");
      for (int i = 0; i < 16; i++)
        display.print(regEx[i], HEX);

    } else if (displayMode == REGD ) {

      displaySetCursor(0, 0);
      display.print("WR 0123456789ABCDEF");
      displaySetCursor(3, 1);
      for (int i = 0; i < 16; i++)
        display.print(reg[i], HEX);

      displaySetCursor(0, 2);
      display.print("SR 0123456789ABCDEF");
      displaySetCursor(3, 3);
      for (int i = 0; i < 16; i++)
        display.print(regEx[i], HEX);
    }
  }

  if (displayMode != REGD && displayCurFuncKey != NO_KEY) {

    displaySetCursor(7, 4);

    switch ( displayCurFuncKey ) {
      case CCE  : display.print("C/CE"); break;
      case RUN  : display.print(" RUN"); break;
      case BKP  : display.print(" BKP"); break;
      case NEXT : display.print("NEXT"); break;
      case PGM  : display.print(" PGM"); break;
      case HALT : display.print("HALT"); break;
      case STEP : display.print("STEP"); break;
      case REG  : display.print(" REG"); break;
      default: break;
    }

    if (millis() - funcKeyTime > 500) {
      displayCurFuncKey = NO_KEY;
      clearLine(4); 
    }
  }


  display.display(); 
  refreshLCD = false;

}


int decodeHex(char c) {

  if (c >= 65 && c <= 70 )
    return c - 65 + 10;
  else if ( c >= 48 && c <= 67 )
    return c - 48;
  else return -1;

}

void loadEEPromProgram(byte pgm, byte start) {

  cursor = CURSOR_OFF;
  int origin = start;
  int adr  = startAddresses[pgm];
  int end = adr + programLengths[pgm];

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("Loading PGM ");
  display.print(pgm + 7);
  int curX = 0;
  int curY = 1;

  while (adr < end) {

    displaySetCursor(15, 0);
    display.print("@ ");
    if (pc < 16)
      display.print("0");
    display.print(pc, HEX);

    delay(10);

    op[start] = EEPROM.read(adr++);
    arg1[start] = EEPROM.read(adr++);
    arg2[start] = EEPROM.read(adr++);

    displaySetCursor(curX, curY);
    display.print(op[start], HEX);
    display.print(arg1[start], HEX);
    display.print(arg2[start], HEX);
    display.print("-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        displaySetCursor(0, 1);
        display.print("                    ");
        displaySetCursor(0, 2);
        display.print("                    ");
        displaySetCursor(0, 3);
        display.print("                    ");
      }
    }

    pc = start;
    start++;
    currentMode = STOPPED;
    outputs = pc;
    showMem();

  }

  pc = origin;
  currentMode = STOPPED;

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("Loaded @ ");
  if (pc < 16)
    display.print("0");
  display.print(pc, HEX);

  showLoaded(false);
  delay(500);

  reset();

}


void sendChar(uint8_t pos, char c, boolean blink) {
   
  displaySetCursor(pos, 5); 
  display.print(blink ? ' ' : c); 

}


void sendHex(uint8_t pos, uint8_t c, boolean blink) {
   
  displaySetCursor(pos, 5); 
  if (blink) {
    display.print(' '); 
  } else {
    display.print(c, HEX);	
  }

}


void displayOff() {

  display.clearDisplay();
  display.display();

  dispOff = true;
  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

}


void setDisplayToHexNumber(uint32_t number) {

/*
  dispLeft.clear();
  dispRight.clear();

  uint32_t r = number % 65536;
  uint32_t l = number / 65536;


  dispRight.printNumber(r, HEX);

  if (l > 0)
    dispLeft.printNumber(l, HEX);

  display.display();
*/ 

  displaySetCursor(0,5); 
  display.print(number, HEX); 

}

void showLoaded(boolean showLCD) {

  if (showLCD) {
    display.clearDisplay();
    display.write("Loaded @ ");
    display.print(pc, HEX);
  }

  sendString(" loaded ");
  sendHex(4, program, false);
  display.display();
  delay(DISP_DELAY);

  sendString("   at   ");

  sendHex(3, pc / 16, false);
  sendHex(4, pc % 16, false);
  display.display();

  delay(DISP_DELAY);


}

void clearStack() {

  sp = 0;

}

void reset() {

  display.clearDisplay();

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
  oneStepOnly = false;

  clearStack();

  curInput = 0;
  inputs = 0;
  outputs = 0;

  displayMode = OFF;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  dispOff = false;
  lastInstructionWasDisp = false;

  refreshLCD = true;

}

void clearMem() {

  cursor = CURSOR_OFF;

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("PGM 5 LOAD 000");

  int curX = 0;
  int curY = 1;

  for (pc = 0; pc < 255; pc++) {

    displaySetCursor(15, 0);
    display.print("@ ");
    if (pc < 16)
      display.print("0");
    display.print(pc, HEX);

    op[pc] = 0;
    arg1[pc] = 0;
    arg2[pc] = 0;

    displaySetCursor(curX, curY);
    display.print("000-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        displaySetCursor(0, 1);
        display.print("                    ");
        displaySetCursor(0, 2);
        display.print("                    ");
        displaySetCursor(0, 3);
        display.print("                    ");
      }
    }

    showMem();

  }

  op[255] = 0;
  arg1[255] = 0;
  arg2[255] = 0;

  reset();

}

void loadNOPs() {

  cursor = CURSOR_OFF;

  display.clearDisplay();
  displaySetCursor(0, 0);
  display.print("PGM 6 LOAD F01");

  int curX = 0;
  int curY = 1;

  for (pc = 0; pc < 255; pc++) {

    displaySetCursor(15, 0);
    display.print("@ ");
    if (pc < 16)
      display.print("0");
    display.print(pc, HEX);

    op[pc] = 15;
    arg1[pc] = 0;
    arg2[pc] = 1;

    displaySetCursor(curX, curY);
    display.print("F01-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        displaySetCursor(0, 1);
        display.print("                    ");
        displaySetCursor(0, 2);
        display.print("                    ");
        displaySetCursor(0, 3);
        display.print("                    ");
      }
    }

    showMem();

  }

  op[255] = 15;
  arg1[255] = 0;
  arg2[255] = 1;

  reset();

}

void interpret() {

  switch ( curFuncKey ) {

    case HALT :

      jump = true;
      currentMode = STOPPED;
      cursor = CURSOR_OFF;

      dispOff = false;
      lastInstructionWasDisp = false;

      display.clearDisplay();
      refreshLCD = true;

      break;

    case RUN :

      currentMode = RUNNING;
      cursor = CURSOR_OFF;
      oneStepOnly = false;
      dispOff = false;
      ignoreBreakpointOnce = true;

      clearStack();
      jump = true; // don't increment PC !
      //step();

      display.clearDisplay();
      refreshLCD = true;

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

      display.clearDisplay();
      refreshLCD = true;
      jump = true;

      break;

    case REG :

      if (currentMode != ENTERING_REG) {
        currentMode = ENTERING_REG;
        cursor = 0;
      } else {
        currentMode = INSPECTING;
        cursor = 1;
      }
      display.clearDisplay();
      refreshLCD = true;

      break;

    case STEP :

      currentMode = RUNNING;
      oneStepOnly = true;
      dispOff = false;
      jump = true; // don't increment PC !

      refreshLCD = true;

      displayStatus();

      break;

    case BKP :

      if (currentMode != ENTERING_BREAKPOINT_LOW ) {
        currentMode = ENTERING_BREAKPOINT_HIGH;
        cursor = 0;
      } else {
        cursor = 1;
        currentMode = ENTERING_BREAKPOINT_LOW;
      }

      refreshLCD = true;

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

      display.clearDisplay();
      refreshLCD = true;

      break;

    case PGM :

      if ( currentMode != ENTERING_PROGRAM ) {
        cursor = 0;
        currentMode = ENTERING_PROGRAM;
      }

      display.clearDisplay();
      refreshLCD = true;

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
        curInput = curHexKey;
        reg[currentInputRegister] = curInput;
        carry = false;
        zero = reg[currentInputRegister] == 0;
        currentMode = RUNNING;
      }

      break;

    case ENTERING_TIME :

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

      break;

    case ENTERING_PROGRAM :

      if (keypadPressed) {

        program = curHexKey;
        currentMode = STOPPED;
        cursor = CURSOR_OFF;

        switch ( program ) {

          case 0 :
            break;

          case 1 :
            loadProgram();
            break;

          case 2 :

            saveProgram();
            break;

          case 3 :
            currentMode = ENTERING_TIME;
            display.clearDisplay();
            display.print("PGM 3 ENTER TIME");
            cursor = 0;
            break;

          case 4 :
            currentMode = SHOWING_TIME;
            display.clearDisplay();
            display.print("PGM 4 SHOW TIME");
            cursor = CURSOR_OFF;
            break;

          case 5 : // clear mem
            clearMem();
            break;

          case 6 : // load NOPs
            loadNOPs();
            break;

          default : // load other

            if (program - 7 < programs ) {
              loadEEPromProgram(program - 7, 0);
            } else {
            }
        }

      }

      break;

    case ENTERING_ADDRESS_HIGH :

      if (keypadPressed) {
        cursor = 1;
        pc = curHexKey * 16;
        currentMode = ENTERING_ADDRESS_LOW;
      }

      break;

    case ENTERING_ADDRESS_LOW :

      if (keypadPressed) {
        cursor = 2;
        pc += curHexKey;
        currentMode = ENTERING_OP;
      }

      break;

    case ENTERING_BREAKPOINT_HIGH :

      if (keypadPressed) {
        cursor = 1;
        breakAt = curHexKey * 16;
        currentMode = ENTERING_BREAKPOINT_LOW;
      }

      refreshLCD = true;

      break;

    case ENTERING_BREAKPOINT_LOW :

      if (keypadPressed) {
        cursor = 0;
        breakAt += curHexKey;
        currentMode = ENTERING_BREAKPOINT_HIGH;
      }

      refreshLCD = true;

      break;

    case ENTERING_OP :

      if (keypadPressed) {
        cursor = 3;
        op[pc] = curHexKey;
        currentMode = ENTERING_ARG1;
      }

      break;

    case ENTERING_ARG1 :

      if (keypadPressed) {
        cursor = 4;
        arg1[pc] = curHexKey;
        currentMode = ENTERING_ARG2;
      }

      break;

    case ENTERING_ARG2 :

      if (keypadPressed) {
        cursor = 2;
        arg2[pc] = curHexKey;
        currentMode = ENTERING_OP;
      }

      break;

    case RUNNING:

      run();

      break;

    case ENTERING_REG:

      if (keypadPressed)
        currentReg = curHexKey;

      break;

    case INSPECTING :

      if (keypadPressed)
        reg[currentReg] = curHexKey;

      break;

  }

}

void run() {

  if (!jump )
    pc++;

  if ( !oneStepOnly && breakAt == pc && breakAt > 0 && ! ignoreBreakpointOnce )  {
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

  switch (op1) {
    case OP_MOV :

      reg[d] = reg[s];
      zero = reg[d] == 0;

      break;

    case OP_MOVI :

      reg[d] = n;
      zero = reg[d] == 0;

      break;

    case OP_AND :

      reg[d] &= reg[s];
      carry = false;
      zero = reg[d] == 0;

      break;

    case OP_ANDI :

      reg[d] &= n;
      carry = false;
      zero = reg[d] == 0;

      break;

    case OP_ADD :

      reg[d] += reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;

      break;

    case OP_ADDI :

      reg[d] += n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;

      break;

    case OP_SUB :

      reg[d] -= reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;

      break;

    case OP_SUBI :

      reg[d] -= n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;

      break;

    case OP_CMP :

      carry = reg[s] < reg[d];
      zero = reg[s] == reg[d];

      break;

    case OP_CMPI :

      carry = n < reg[d];
      zero = reg[d] == n;

      break;

    case OP_OR :

      reg[d] |= reg[s];
      carry = false;
      zero = reg[d] == 0;

      break;

    //
    //
    //


    case OP_CALL :

      if (sp < STACK_DEPTH - 1) {
        stack[sp] = pc;
        sp++;
        pc = adr;
        jump = true;

      } else {

        error = true;
        currentMode = STOPPED;

      }
      break;

    case OP_GOTO :

      pc = adr;
      jump = true;

      break;

    case OP_BRC :

      if (carry) {
        pc = adr;
        jump = true;
      }

      break;

    case OP_BRZ :

      if (zero) {
        pc = adr;
        jump = true;
      }

      break;

    //
    //
    //

    default : {

        switch (op2) {

          case OP_MAS :

            regEx[d] = reg[d];

            break;

          case OP_INV :

            reg[d] ^= 15;

            break;

          case OP_SHR :

            reg[d] >>= 1;
            carry = reg[d] & 16;
            reg[d] &= 15;
            zero = reg[d] == 0;

            break;

          case OP_SHL :

            reg[d] <<= 1;
            carry = reg[d] & 16;
            reg[d] &= 15;
            zero = reg[d] == 0;

            break;

          case OP_ADC :

            if (carry) reg[d]++;
            carry = reg[d] > 15;
            reg[d] &= 15;
            zero = reg[d] == 0;

            break;

          case OP_SUBC :

            if (carry) reg[d]--;
            carry = reg[d] > 15;
            reg[d] &= 15;
            zero = reg[d] == 0;

            break;

          //
          //
          //

          case OP_DIN :

            reg[d] = readDIN();
            carry = false;
            zero = reg[d] == 0;

            break;

          case OP_DOT :

            outputs = reg[dot_s];
            carry = false;
            zero = reg[dot_s] == 0;

            break;

          case OP_KIN :

            currentMode = ENTERING_VALUE;
            currentInputRegister = d;

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

                  break;

                case OP_DISOUT :

                  displayOff();

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

                  break;

                case OP_RND :

                  reg[0xD] = random(16);
                  reg[0xE] = random(16);
                  reg[0xF] = random(16);

                  break;

                case OP_TIME :

                  reg[0xA] = timeSeconds1;
                  reg[0xB] = timeSeconds10;
                  reg[0xC] = timeMinutes1;
                  reg[0xD] = timeMinutes10;
                  reg[0xE] = timeHours1;
                  reg[0xF] = timeHours10;

                  break;

                case OP_RET :

                  pc = stack[--sp] + 1;
                  jump = true;
                  break;

                case OP_CLEAR :

                  for (byte i = 0; i < 16; i ++)
                    reg[i] = 0;

                  carry = false;
                  zero = true;

                  break;

                case OP_STC :

                  carry = true;

                  break;

                case OP_RSC :

                  carry = false;

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

                  break;

                case OP_DIV :

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
                    zero = false,

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

                  break;

                case OP_EXRL :

                  for (int i = 0; i < 8; i++) {
                    byte aux = reg[i];
                    reg[i] = regEx[i];
                    regEx[i] = aux;
                  }

                  break;

                case OP_EXRM :

                  for (int i = 8; i < 16; i++) {
                    byte aux = reg[i];
                    reg[i] = regEx[i];
                    regEx[i] = aux;
                  }

                  break;

                case OP_EXRA :

                  for (int i = 0; i < 8; i++) {
                    byte aux = reg[i];
                    reg[i]   = reg[i + 8];
                    reg[i + 8] = aux;
                  }

                  break;

                default : // DISP!

                  dispOff = false;
                  showingDisplayDigits = disp_n;
                  showingDisplayFromReg = disp_s;
                  lastInstructionWasDisp = true;

                  if (oneStepOnly) {
                    display.clearDisplay();
                    display.display();
                    showDISP();
                  }

                  break;

                  //
                  //
                  //

              }
            }
        }
      }
  }

  if (oneStepOnly) {
    currentMode = STOPPED;
    if (! jump) {
      pc++;
    }
  }

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
