/*
  A Busch 2090 Microtronic Emulator for Arduino Mega 2560

  Version 0.99 (c) Michael Wessel, January 23 2016

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  With Contributions from Martin Sauter (PGM 7)
  See http://mobilesociety.typepad.com/

  Hardware requirements:
  - 4x4 hex keypad (HEX keypad for data and program entry)
  - TM1638 8 digit 7segment display with 8 LEDs and 8 buttons (function keys)
  - SDCard + Ethernet shield
  - LCD + Keypad shield

  The Busch Microtronic 2090 is (C) Busch GmbH
  See http://www.busch-model.com/online/?rubrik=82&=6&sprach_id=de

  Please run the PGM-EEPROM.ino sketch before running / loading this
  sketch into the Arduino. The emulator will not work properly
  otherwise. Note that PGM-EEPROM.ino stores example programs into the
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

#include <SPI.h>
#include <SD.h>
#include <TM1638.h>
#include <Keypad.h>
#include <TM16XXFonts.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

//
// set up hardware - wiring
//

//
// LCD panel pins
//

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

boolean forceRefresh = false;

//
// define key codes for LCD keypad panel
// these are analog read from the LCD and
// may require adjustments in order to fit
// your LCD Keypad shield
// Note that the values specified here should
// be lower bounds to distinguish them, i.e.,
// an analog value > 770 counts as SELECT etc.
//

#define NOTHING_KEY 1000
#define SELECT_KEY  770
#define LEFT_KEY    540
#define DOWN_KEY    360
#define UP_KEY      160

int keypadKeys = 0;
int lastKeypadKeys = 255;

//
//
//

#define NOTHING 0
#define SELECT 1
#define LEFT 2
#define DOWN 3
#define UP 4
#define RIGHT 5

//
// LCD Display mode
//

enum LCDmode {
  OFF,
  PCMEM,
  REG1,
  REG2
};

LCDmode displayMode = OFF;

//
// LED Module and keys
// Use pins 49, 47, 45 for TM1638 module
//

TM1638 module(49, 47, 45);

#define DISP_DELAY 200

//
// Keypad 4x4 matrix
// Use pins 30 - 37
//

#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = { // plus one because 0 = no key pressed!
  {0xD, 0xE, 0xF, 0x10},
  {0x9, 0xA, 0xB, 0xC},
  {0x5, 0x6, 0x7, 0x8},
  {0x1, 0x2, 0x3, 0x4}
};

byte rowPins[ROWS] = {37, 35, 33, 31}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {36, 34, 32, 30}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

unsigned long lastFuncKeyTime = 0;

#define FUNCTION_KEY_DEBOUNCE_TIME 50

//
// these are the digital pins used for DIN instructions
//

#define DIN_PIN_1 22
#define DIN_PIN_2 24
#define DIN_PIN_3 26
#define DIN_PIN_4 28

//
// reset Microtronic (not Arduino) by pulling this to GND
//

#define RESET_PIN 53

//
// connect a CPU throttle potentiometer here
//

#define CPU_THROTTLE_ANALOG_PIN 15 // connect a potentiometer here for CPU speed throttle controll 
#define CPU_THROTTLE_DIVISOR 10 // potentiometer dependent 
#define CPU_MIN_THRESHOLD 10 // if smaller than this, delay = 0

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
// Current PGM program
//

byte program;

//
//
//

int cpu_delay = 0;

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

boolean jump = false;
byte pc = 0;
byte lastPc = 1;
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
  ENTERING_PROGRAM,

  ENTERING_TIME,
  SHOWING_TIME

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

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Busch 2090");
  lcd.setCursor(0, 1);
  lcd.print("Microtronic");

  Serial.begin(9600);
  randomSeed(analogRead(1));

  pinMode(RESET_PIN, INPUT_PULLUP); // reset pin
  pinMode(DIN_PIN_1, INPUT_PULLUP); // DIN 1
  pinMode(DIN_PIN_2, INPUT_PULLUP); // DIN 2
  pinMode(DIN_PIN_3, INPUT_PULLUP); // DIN 3
  pinMode(DIN_PIN_4, INPUT_PULLUP); // DIN 4

  //
  // read EEPROM PGMs meta data
  //

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  init  ");

  int adr = 0;
  programs = EEPROM.read(adr++);
  module.setDisplayToHexNumber(programs, 0, true);
  delay(100);

  int start = 1;

  for (int n = 0; n < programs; n++) {
    startAddresses[n] = start + programs;
    programLengths[n] = EEPROM.read(adr++);
    start += programLengths[n];
    module.setDisplayToHexNumber( startAddresses[n], 0, true);
    delay(50);
  }

  //
  //
  //

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  ready ");

  lcd.clear();

  //
  //
  //

  boolean initialized = SD.begin(4); 

/*
  while (!Serial) {
  }

  if (! initialized) {
    Serial.println("SD initialization failed!");
    return;
  }
*/ 

}


String selectFile() {

  int count = 0;

  lcd.clear();

  File root = SD.open("/");

  while (true) {

    File entry =  root.openNextFile();
    if (! entry) {
      entry.close();
      break;
    } else if (! entry.isDirectory()) {
      count++;
      entry.close();
    }
  }
  root.close();

  String* files = new String [count];

  count = 0;

  root = SD.open("/");

  while (true) {

    File entry =  root.openNextFile();
    if (! entry) {
      entry.close();
      break;
    } else if (! entry.isDirectory()) {
      files[count++] = String(entry.name());
      entry.close();
    }
  }
  root.close();

  int cursor = 0;

  unsigned long last = millis();
  boolean blink = false;

  int decoded = NOTHING;

  lcd.setCursor(0, 0);
  lcd.print("Load Program");
  lcd.setCursor(0, 1);
  lcd.print(files[0]);

  while ( decoded != SELECT ) {

    keypadKeys  = analogRead(0);

    if ( millis() - last > 100) {

      last = millis();

      lcd.setCursor(0, 1);

      blink = !blink;

      if (blink)
        lcd.print("                ");
      else
        lcd.print(files[cursor]);

      if ( abs(keypadKeys - lastKeypadKeys ) > 40) {

        delay(10);
        keypadKeys  = analogRead(0);

        if (keypadKeys > NOTHING_KEY)
          decoded = NOTHING;
        else if (keypadKeys > SELECT_KEY)
          decoded = SELECT;
        else if (keypadKeys > LEFT_KEY)
          decoded = LEFT;
        else if (keypadKeys > DOWN_KEY)
          decoded = DOWN;
        else if (keypadKeys > UP_KEY)
          decoded = UP;
        else
          decoded = RIGHT;

        lastKeypadKeys = keypadKeys;

        switch ( decoded ) {
          case UP : cursor++; break;
          case DOWN : cursor--; break;
          case LEFT : return ""; break;
          default : break;
        }

        if (cursor < 0)
          cursor = 0;
        else if (cursor > count - 1)
          cursor = count - 1;

      }
    }
  }

  return files[cursor];

}


String createName(String name) {

  int cursor = 0;

  unsigned long last = millis();
  boolean blink = false;

  int decoded = NOTHING;

  lcd.setCursor(0, 0);
  lcd.print("Save Program");
  lcd.setCursor(0, 1);
  lcd.print(name);

  while ( decoded != SELECT ) {

    keypadKeys  = analogRead(0);

    if ( millis() - last > 100) {

      last = millis();

      lcd.setCursor(cursor, 1);

      blink = !blink;

      if (blink)
        if (name[cursor] == ' ' )
          lcd.print("_");
        else
          lcd.print(name[cursor]);
      else
        lcd.print("_");

      if ( abs(keypadKeys - lastKeypadKeys ) > 40) {

        delay(10);
        keypadKeys  = analogRead(0);

        if (keypadKeys > NOTHING_KEY)
          decoded = NOTHING;
        else if (keypadKeys > SELECT_KEY)
          decoded = SELECT;
        else if (keypadKeys > LEFT_KEY)
          decoded = LEFT;
        else if (keypadKeys > DOWN_KEY)
          decoded = DOWN;
        else if (keypadKeys > UP_KEY)
          decoded = UP;
        else
          decoded = RIGHT;

        lastKeypadKeys = keypadKeys;

        switch ( decoded ) {
          case UP : name[cursor]++; break;
          case DOWN : name[cursor]--; break;
          case LEFT : cursor--; break;
          case RIGHT : cursor++; break;
          default : break;
        }

        if (cursor < 0)
          cursor = 0;
        else if (cursor > 7)
          cursor = 7;

        if (cursor > name.length() - 1 )
          name += "0";

        lcd.setCursor(0, 1);
        lcd.print(name);

      }
    }
  }

  return name;

}

void saveProgram() {

  int oldPc = pc;

  lcd.clear();

  String name = String("P");

  String fn = createName(name);
  fn += ".mic";

  if (SD.exists(fn)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Overwriting");
    lcd.setCursor(0, 1);
    lcd.print(fn);
    SD.remove(fn);
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving");
  lcd.setCursor(0, 1);
  lcd.print(fn);

  File myFile = SD.open( fn , FILE_WRITE);

  cursor = CURSOR_OFF;

  if (myFile) {

    myFile.print("# PC = ");
    myFile.print(pc);
    myFile.println();

    for (int i = pc; i < 256; i++) {

      lcd.setCursor(8, 0);
      lcd.print("@ ");
      lcd.print(i, HEX);

      myFile.print(op[i], HEX);
      myFile.print(arg1[i], HEX);
      myFile.print(arg2[i], HEX);
      myFile.println();
      if (i % 16 == 15)
        myFile.println();

      pc = i;
      showMem();

      lcd.setCursor(13, 0);
      lcd.print(op[i], HEX);
      lcd.print(arg1[i], HEX);
      lcd.print(arg2[i], HEX);

    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Saved");
    lcd.setCursor(0, 1);
    lcd.print(fn);
    delay(500);

  } else {
    lcd.clear();
    lcd.print("*** ERROR ! ***");
    delay(500);

  }

  lcd.clear();

  myFile.close();

  reset();
  pc = oldPc;

}


void loadProgram() {

  lcd.clear();

  String fn = selectFile();

  if ( fn.equals("") ) {
    lcd.clear();
    lcd.print("*** ABORT ***");
    delay(500);
    lcd.clear();
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading");
  lcd.setCursor(0, 1);
  lcd.print(fn);

  cursor = CURSOR_OFF;

  File myFile = SD.open( fn);

  int count = 0;
  int firstPc = -1;
  int oldPc = pc;

  if (myFile) {

    boolean readingComment = false;
    boolean readingOrigin = false;

    while (true) {

      lcd.setCursor(8, 0);
      lcd.print("@ ");
      lcd.print(pc, HEX);

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
          lcd.clear();
          lcd.print("*** ERROR ! ***");
          lcd.setCursor(0, 1);
          lcd.print("Byte ");
          lcd.print(count);
          lcd.print(" ");
          lcd.print((char) b);
          delay(2000);
          lcd.clear();
          break;
        }

        lcd.setCursor(13 + count, 0);
        lcd.print(b);

        if ( readingOrigin ) {
          switch ( count ) {
            case 0 : pc = decoded * 16; count = 1; break;
            case 1 : pc += decoded; showMem(); count = 0; readingOrigin = false; break;
            default : break;
          }       

        } else {
          switch ( count ) {
            case 0 : op[pc] = decoded; count = 1;  break;
            case 1 : arg1[pc] = decoded; count = 2; break;
            case 2 : arg2[pc] = decoded; showMem(); count = 0; if (firstPc == -1) firstPc = pc; pc++; break;
            default : break;
          }
       
        }
      }
    }

    myFile.close();

    pc = firstPc;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loaded @ ");
    lcd.print(pc, HEX);
    lcd.setCursor(0, 1);
    lcd.print(fn);

    showLoaded();
    delay(500);

  } else {
    lcd.clear();
    lcd.print("*** ERROR ! ***");
    delay(500);

  }

  lcd.clear();

  reset();
  pc = firstPc;

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

void advanceTime() {

  if (currentMode != ENTERING_TIME) {

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
  }
}

void showTime() {

  module.sendChar(1, 0, false);

  if (cursor == 0)
    module.sendChar(2, blink ? NUMBER_FONT[ timeHours10 ] : 0, true);
  else
    module.sendChar(2, NUMBER_FONT[ timeHours10 ], false);

  if (cursor == 1)
    module.sendChar(3, blink ? NUMBER_FONT[ timeHours1 ] : 0, true);
  else
    module.sendChar(3, NUMBER_FONT[ timeHours1  ], false);

  if (cursor == 2)
    module.sendChar(4, blink ? NUMBER_FONT[ timeMinutes10 ] : 0, true);
  else
    module.sendChar(4, NUMBER_FONT[ timeMinutes10 ], false);

  if (cursor == 3)
    module.sendChar(5, blink ? NUMBER_FONT[ timeMinutes1 ] : 0, true);
  else
    module.sendChar(5, NUMBER_FONT[ timeMinutes1  ], false);

  if (cursor == 4)
    module.sendChar(6, blink ? NUMBER_FONT[ timeSeconds10 ] : 0, true);
  else
    module.sendChar(6, NUMBER_FONT[ timeSeconds10 ], false);

  if (cursor == 5)
    module.sendChar(7, blink ? NUMBER_FONT[ timeSeconds1 ] : 0, true);
  else
    module.sendChar(7, NUMBER_FONT[ timeSeconds1  ], false);

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

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  for (int i = 0; i < 8; i++)
    module.sendChar(i, 0, false);

}

void showDisplay() {

  for (int i = 0; i < showingDisplayDigits; i++)
    module.sendChar(7 - i, NUMBER_FONT[reg[(i +  showingDisplayFromReg ) % 16]], false);

}

void displayStatus() {

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;
  unsigned long delta2 = time - lastDispTime2;

  moduleLEDs = 0;

  if (delta >= 1000) {
    clock = !clock;
    lastDispTime = time;
    advanceTime();
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
  else if (currentMode == ENTERING_TIME )
    status = 't';
  else if (currentMode == SHOWING_TIME )
    status = 'C';
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
  else if ( currentMode == ENTERING_TIME || currentMode == SHOWING_TIME )
    showTime();
  else if ( error )
    showError();
  else
    showMem();

  updateLCD();


}

void updateLCD() {

  scanLCDKeypad();

  if ( displayMode != OFF) {

    if ( pc != lastPc || forceRefresh ) {

      forceRefresh = false;

      if (displayMode == PCMEM ) {

        lastPc = pc;

        lcd.setCursor(0, 0);
        lcd.print("PC     OP  MNEM");
        lcd.setCursor(0, 1);

        if (pc < 16)
          lcd.print(0);
        lcd.print(pc, HEX);

        lcd.setCursor(3, 1);

        if (pc < 100)
          lcd.print(0);
        if (pc < 10)
          lcd.print(0);
        lcd.print(pc);

        lcd.setCursor(7, 1);
        lcd.print(op[pc], HEX);
        lcd.print(arg1[pc], HEX);
        lcd.print(arg2[pc], HEX);

        lcd.setCursor(11, 1);

        String mnem = "";

        byte op1 = op[pc];
        byte hi = arg1[pc];
        byte lo = arg2[pc];
        byte op2 = op1 * 16 + hi;
        unsigned int op3 = op1 * 256 + hi * 16 + lo;

        switch ( op[pc] ) {
          case OP_MOV  : mnem = "MOV   " ; break;
          case OP_MOVI : mnem = "MOVI  " ; break;
          case OP_AND  : mnem = "AND   " ; break;
          case OP_ANDI : mnem = "ANDI  "; break;
          case OP_ADD  : mnem = "ADD   "; break;
          case OP_ADDI : mnem = "ADDI  "; break;
          case OP_SUB  : mnem = "SUB   "; break;
          case OP_SUBI : mnem = "SUBI  "; break;
          case OP_CMP  : mnem = "CMP   "; break;
          case OP_CMPI : mnem = "CMPI  "; break;
          case OP_OR   : mnem = "OR    "; break;
          case OP_CALL : mnem = "CALL  "; break;
          case OP_GOTO : mnem = "GOTO  "; break;
          case OP_BRC  : mnem = "BRC   "; break;
          case OP_BRZ  : mnem = "BRZ   "; break;
          default : {
              switch (op2) {
                case OP_MAS  : mnem = "MAS   "; break;
                case OP_INV  : mnem = "INV   "; break;
                case OP_SHR  : mnem = "SHR   "; break;
                case OP_SHL  : mnem = "SHL   "; break;
                case OP_ADC  : mnem = "ADC   "; break;
                case OP_SUBC : mnem = "SUBC  "; break;
                case OP_DIN  : mnem = "DIN   "; break;
                case OP_DOT  : mnem = "DOT   "; break;
                case OP_KIN  : mnem = "KIN   "; break;
                default : {
                    switch (op3) {
                      case OP_HALT   : mnem = "HALT  "; break;
                      case OP_NOP    : mnem = "NOP   "; break;
                      case OP_DISOUT : mnem = "DISOUT"; break;
                      case OP_HXDZ   : mnem = "HXDZ  "; break;
                      case OP_DZHX   : mnem = "DZHX  "; break;
                      case OP_RND    : mnem = "RND   "; break;
                      case OP_TIME   : mnem = "TIME  "; break;
                      case OP_RET    : mnem = "RET   "; break;
                      case OP_CLEAR  : mnem = "CLEAR "; break;
                      case OP_STC    : mnem = "STC   "; break;
                      case OP_RSC    : mnem = "RSC   "; break;
                      case OP_MULT   : mnem = "MULT  "; break;
                      case OP_DIV    : mnem = "DIV   "; break;
                      case OP_EXRL   : mnem = "EXRL  "; break;
                      case OP_EXRM   : mnem = "EXRM  "; break;
                      case OP_EXRA   : mnem = "EXRA  "; break;
                      default        : mnem = "DISP  "; break;
                    }
                  }
              }
            }
        }

        lcd.print(mnem);

      } else if (displayMode == REG1 ) {

        lcd.setCursor(0, 0);
        lcd.print("0123456789ABCDEF");
        lcd.setCursor(0, 1);
        for (int i = 0; i < 16; i++)
          lcd.print(reg[i], HEX);

      } else if (displayMode == REG2 ) {

        lcd.setCursor(0, 0);
        lcd.print("0123456789abcdef");
        lcd.setCursor(0, 1);
        for (int i = 0; i < 16; i++)
          lcd.print(regEx[i], HEX);
      }
    }
  }
}

void scanLCDKeypad() {

  keypadKeys  = analogRead(0);

  if ( abs(keypadKeys - lastKeypadKeys ) > 40) {

    delay(30);
    keypadKeys  = analogRead(0);
    int decoded;

    if (keypadKeys > NOTHING_KEY)
      decoded = NOTHING;
    else if (keypadKeys > SELECT_KEY)
      decoded = SELECT;
    else if (keypadKeys > LEFT_KEY)
      decoded = LEFT;
    else if (keypadKeys > DOWN_KEY)
      decoded = DOWN;
    else if (keypadKeys > UP_KEY)
      decoded = UP;
    else
      decoded = RIGHT;

    lastKeypadKeys = keypadKeys;

    if (decoded == SELECT ) {
      forceRefresh = true;
      switch ( displayMode  ) {
        case OFF    : displayMode = PCMEM; break;
        case PCMEM  : displayMode = REG1; break;
        case REG1   : displayMode = REG2; break;
        default     : displayMode = OFF; lcd.clear(); break;
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

void enterProgram(byte pgm, byte start) {

  cursor = CURSOR_OFF;
  int origin = start;
  int adr  = startAddresses[pgm];
  int end = adr + programLengths[pgm];

  while (adr < end) {

    op[start] = EEPROM.read(adr++);
    arg1[start] = EEPROM.read(adr++);
    arg2[start] = EEPROM.read(adr++);

    pc = start;
    start++;
    currentMode = STOPPED;
    outputs = pc;
    displayOff();
    displayStatus();
    delay(10);
  };

  pc = origin;
  currentMode = STOPPED;

  showLoaded();
  lcd.clear();

}

void showLoaded() {

  sendString(" loaded ");
  displayOff();
  module.sendChar(4, NUMBER_FONT[program], false);
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

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  forceRefresh = true;

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

    case ENTERING_TIME :

      if (keypadPressed ) {

        input = keypadKey;
        switch (cursor) {
          case 0 : if (input < 3) {
              timeHours10 = input;
              cursor++;
            } break;
          case 1 : if (timeHours10 == 2 && input < 4 || timeHours10 < 2 && input < 10) {
              timeHours1 = input;
              cursor++;
            } break;
          case 2 : if (input < 6) {
              timeMinutes10 = input;
              cursor++;
            } break;
          case 3 : if (input < 10) {
              timeMinutes1 = input;
              cursor++;
            } break;
          case 4 : if (input < 6) {
              timeSeconds10 = input;
              cursor++;
            } break;
          case 5 : if (input < 10) {
              timeSeconds1 = input;
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

        program = keypadKey;
        currentMode = STOPPED;
        cursor = CURSOR_OFF;

        switch ( program ) {

          case 0 :
            error = true;
            break;

          case 1 :
            loadProgram();
            break;

          case 2 :

            saveProgram();
            break;

          case 3 :

            currentMode = ENTERING_TIME;
            cursor = 0;
            break;

          case 4 :

            currentMode = SHOWING_TIME;
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
              enterProgram(program - 7, 0);
            } else
              error = true;
        }

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

  if (!jump)
    pc++;
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
      ;

      break;

    case OP_MOVI :

      reg[d] = n;
      zero = reg[d] == 0;
      ;

      break;

    case OP_AND :

      reg[d] &= reg[s];
      carry = false;
      zero = reg[d] == 0;
      ;

      break;

    case OP_ANDI :

      reg[d] &= n;
      carry = false;
      zero = reg[d] == 0;
      ;

      break;

    case OP_ADD :

      reg[d] += reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero = reg[d] == 0;
      ;

      break;

    case OP_ADDI :

      reg[d] += n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      ;

      break;

    case OP_SUB :

      reg[d] -= reg[s];
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      ;

      break;

    case OP_SUBI :

      reg[d] -= n;
      carry = reg[d] > 15;
      reg[d] &= 15;
      zero =  reg[d] == 0;
      ;

      break;

    case OP_CMP :

      carry = reg[s] < reg[d];
      zero = reg[s] == reg[d];
      ;

      break;

    case OP_CMPI :

      carry = n < reg[d];
      zero = reg[d] == n;
      ;

      break;

    case OP_OR :

      reg[d] |= reg[s];
      carry = false;
      zero = reg[d] == 0;
      ;

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

            reg[d] = !digitalRead(DIN_PIN_1) | !digitalRead(DIN_PIN_2) << 1 | !digitalRead(DIN_PIN_3) << 2 | !digitalRead(DIN_PIN_4) << 3;
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

                  showingDisplayDigits = 0;
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

                  for (int i = 0; i < 7; i++) // not documented in manual, but true!
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

                  displayOff();
                  showingDisplayDigits = disp_n;
                  showingDisplayFromReg = disp_s;


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
  } else if (millis() - lastFuncKeyTime > FUNCTION_KEY_DEBOUNCE_TIME ) { // debounce
    previousFunctionKey = functionKey;
    error = false;
    lastFuncKeyTime = millis();
  } else
    functionKey = NO_KEY;

  keypadKey = keypad.getKey();

  if (keypadKey == previousKeypadKey) { // button held down pressed
    keypadKey = NO_KEY;
  } else {
    previousKeypadKey = keypadKey;
    forceRefresh = true;
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

  if (!digitalRead(RESET_PIN)) {
    reset();
  }

  cpu_delay = analogRead(CPU_THROTTLE_ANALOG_PIN) / CPU_THROTTLE_DIVISOR;
  if (cpu_delay < CPU_MIN_THRESHOLD)
    cpu_delay = 0;

}
