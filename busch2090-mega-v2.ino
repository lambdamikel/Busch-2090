/*

  A Busch 2090 Microtronic Emulator for Arduino Mega 2560

  Version 2.2 (c) Michael Wessel, February 28 2016

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  With Contributions from Martin Sauter (PGM 7)
  See http://mobilesociety.typepad.com/

  Hardware requirements:
  - 4x4 hex keypad (HEX keypad for data and program entry), matrix encoded
  - 3x4 telephone keypad, NOT matrix encoded
  - 2 Adafruit 7Segment LED display backpacks
  - 4x20 LCD display, standard Hitachi HD44780
  - 8 LEDs (and 8 100 Ohm resistors, matching LEDs)
  - 9 momementary N.O. push buttons
  - 1 Power switch (optional)
  - 1 Arduino Power Supply
  - 1 200 Ohm potentiometer
  - 1 100 Ohm potentiometer (LCD contrast)
  - 1 SDCard + Ethernet shield
  - 1 Arduino Mega 2560
  - lots of wires and solder

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

#include <SPI.h>
#include <SD.h>
#include <Keypad.h>
#include <TM16XXFonts.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>

//
// hardware configuration / wiring
//

//
// momentary N.O. push buttons
//

#define RESET  47 // soft reset 

#define BACK   63
#define RIGHT  64
#define UP     65
#define DOWN   66
#define LEFT   67
#define CANCEL 68
#define ENTER  69

int pushButtons[] = { RESET, BACK, RIGHT, UP, DOWN, LEFT, CANCEL, ENTER };

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
// 1 Hz clock digital output
// 

#define CLOCK_OUT 6

//
// DOT digital output
//

#define DOT_1 5
#define DOT_2 3
#define DOT_3 2 // we need pin 4 for SD card!
#define DOT_4 1

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

int dinButtons[] = { DIN_BUTTON_1, DIN_BUTTON_2, DIN_BUTTON_3, DIN_BUTTON_4 };

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

int functionButtons[] = { CCE, RUN, BKP, NEXT, PGM, HALT, STEP, REG };

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

#define DISP_DELAY 200

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

//
// end of hardware configuration
//


//
// LCD display mode
//

enum LCDmode {
  OFF,
  PCMEM,
  REGD
};

LCDmode displayMode = OFF;

//
// keypad keys
//

boolean keypadPressed = false;

int curKeypadKey    = NO_KEY;
int curKeypadKeyRaw = NO_KEY;

//
// function keypad keys
//

#define FUNCKEY_DEBOUNCE_TIME 50

unsigned long funcKeyTime = 0;
int curFuncKey    = NO_KEY;
int curFuncKeyRaw = NO_KEY;

int displayCurFuncKey = NO_KEY; // for LCD display feedback only

//
// push buttons
//

#define PUSH_BUTTON_DEBOUNCE_TIME 50

unsigned long pushButtonTime = 0;
int curPushButton    = NO_KEY;
int curPushButtonRaw = NO_KEY;

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
// CPU speed / delay from throttle potentiometer
//

int cpu_delay = 0;
int last_cpu_delay = 0;

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
  STEPING,

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
//
//

String hexStringChar[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F" };

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

  //
  // init displays
  //

  right.begin(0x71);
  left.begin(0x70);

  lcd.begin(20, 4);
  LCDLogo();

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  init  ");

  //
  // SD Card init
  //

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  boolean initialized = SD.begin(4);
  if (! initialized) {
    lcd.clear();
    lcd.println("SD init failed!");
    delay(2000);
  }

  //
  // init pins
  //

  pinMode(RESET, INPUT_PULLUP);

  pinMode(BACK,   INPUT_PULLUP);
  pinMode(RIGHT,  INPUT_PULLUP);
  pinMode(UP,     INPUT_PULLUP);
  pinMode(DOWN,   INPUT_PULLUP);
  pinMode(LEFT,   INPUT_PULLUP);
  pinMode(CANCEL, INPUT_PULLUP);
  pinMode(ENTER,  INPUT_PULLUP);

  pinMode(CLOCK_LED,     OUTPUT);
  pinMode(CLOCK_1HZ_LED, OUTPUT);
  pinMode(CARRY_LED,     OUTPUT);
  pinMode(ZERO_LED,      OUTPUT);

  pinMode(DOT_LED_1, OUTPUT);
  pinMode(DOT_LED_2, OUTPUT);
  pinMode(DOT_LED_3, OUTPUT);
  pinMode(DOT_LED_4, OUTPUT);

  pinMode(CLOCK_OUT, OUTPUT); 
  
  pinMode(DOT_1, OUTPUT);
  pinMode(DOT_2, OUTPUT);
  pinMode(DOT_3, OUTPUT);
  pinMode(DOT_4, OUTPUT);

  pinMode(DIN_1, INPUT); // that should be changed to input with pulldown resistors!
  pinMode(DIN_2, INPUT);
  pinMode(DIN_3, INPUT);
  pinMode(DIN_4, INPUT);

  pinMode(DIN_BUTTON_1, INPUT_PULLUP);
  pinMode(DIN_BUTTON_2, INPUT_PULLUP);
  pinMode(DIN_BUTTON_3, INPUT_PULLUP);
  pinMode(DIN_BUTTON_4, INPUT_PULLUP);

  pinMode(CCE,  INPUT_PULLUP);
  pinMode(RUN,  INPUT_PULLUP);
  pinMode(BKP,  INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);
  pinMode(PGM,  INPUT_PULLUP);
  pinMode(HALT, INPUT_PULLUP);
  pinMode(STEP, INPUT_PULLUP);
  pinMode(REG,  INPUT_PULLUP);

  pinMode(CPU_THROTTLE_ANALOG_PIN, INPUT);
  pinMode(RANDOM_ANALOG_PIN,       INPUT);


  //
  // random generator init
  //

  randomSeed(analogRead(RANDOM_ANALOG_PIN));

  //
  // read EEPROM PGMs meta data
  //

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

  //
  //
  //

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  ready ");


  //
  // init SD card
  //


}

int readPushButtons() {

  int button = NO_KEY;

  for (byte i = 0; i < 8; i++) {
    if ( ! digitalRead(pushButtons[i]) ) {
      button = pushButtons[i];
      break;
    }
  }

  curPushButton = NO_KEY;

  if ( button != curPushButtonRaw ) {
    if ( ( millis() - pushButtonTime) > PUSH_BUTTON_DEBOUNCE_TIME ) {
      curPushButton = button;
      pushButtonTime = millis();
    }
  }

  curPushButtonRaw = button;

  return curPushButton;

}

int readFunctionKeys() {

  int button = NO_KEY;

  for (byte i = 0; i < 8; i++) {
    if ( ! digitalRead(functionButtons[i]) ) {
      button = functionButtons[i];
      break;
    }
  }

  if ( button != curFuncKeyRaw ) {
    if (( millis() - funcKeyTime) > FUNCKEY_DEBOUNCE_TIME ) {
      curFuncKey = button;
      funcKeyTime = millis();
      if (button != NO_KEY)
        displayCurFuncKey = button;
    }
  } else
    curFuncKey = NO_KEY;

  curFuncKeyRaw = button;

  return curFuncKey;

}


byte readDIN() {

  return ( !digitalRead(DIN_BUTTON_1)      |
           !digitalRead(DIN_BUTTON_2) << 1 |
           !digitalRead(DIN_BUTTON_3) << 2 |
           !digitalRead(DIN_BUTTON_4) << 3 |

           digitalRead(DIN_1)      |
           digitalRead(DIN_2) << 1 |
           digitalRead(DIN_3) << 2 |
           digitalRead(DIN_4) << 3   );

}

String selectFile() {

  lcd.clear();

  int count = 0;
  File root = SD.open("/");
  root.rewindDirectory();

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

  String* files = new String [count];
  count = 0;
  root.rewindDirectory();

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

  readPushButtons();

  while ( curPushButton != ENTER ) {

    lcd.setCursor(0, 0);
    lcd.print("Load Program ");
    lcd.print(cursor + 1);
    lcd.print(" / ");
    lcd.print(count);

    readPushButtons();

    if ( millis() - last > 100) {

      last = millis();

      lcd.setCursor(0, 1);

      blink = !blink;

      if (blink)
        lcd.print("                ");
      else
        lcd.print(files[cursor]);
    }

    switch ( curPushButton ) {
      case UP : cursor++; break;
      case DOWN : cursor--; break;
      case CANCEL : return ""; break;
      default : break;
    }

    if (cursor < 0)
      cursor = 0;
    else if (cursor > count - 1)
      cursor = count - 1;

  }

  return files[cursor];

}


String createName(String name) {

  int cursor = 0;

  unsigned long last = millis();
  boolean blink = false;

  readPushButtons();

  while ( curPushButton != ENTER ) {

    lcd.setCursor(0, 0);
    lcd.print("Save Program");

    readPushButtons();

    if ( millis() - last > 100) {

      last = millis();

      lcd.setCursor(0, 1);
      lcd.print(name);
      lcd.setCursor(cursor, 1);

      blink = !blink;

      if (blink)
        if (name[cursor] == ' ' )
          lcd.print("_");
        else
          lcd.print(name[cursor]);
      else
        lcd.print("_");
    }

    switch ( curPushButton ) {
      case UP : name[cursor]++; break;
      case DOWN : name[cursor]--; break;
      case LEFT : cursor--; break;
      case RIGHT : cursor++; break;
      case BACK : if (cursor == name.length() - 1 && cursor > 0) {
          name = name.substring(0, name.length() - 1);
          cursor--;
          lcd.clear();
        } break;
      case CANCEL : return "";
      default : break;
    }

    if (cursor < 0)
      cursor = 0;
    else if (cursor > 7)
      cursor = 7;

    if (cursor > name.length() - 1 )
      name += "0";

  }

  return name;

}

void saveProgram() {

  int oldPc = pc;

  lcd.clear();

  String name = String("P");

  String fn = createName(name);

  if ( fn.equals("") ) {
    lcd.clear();
    lcd.print("*** ABORT ***");
    delay(500);   
    reset();
    pc = oldPc;
    return;
  }

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

    int curX = 0;
    int curY = 1;

    for (int i = pc; i < 256; i++) {

      delay(10);

      lcd.setCursor(8, 0);
      lcd.print("@ ");
      if ( i < 16)
        lcd.print("0");
      lcd.print(i, HEX);

      myFile.print(op[i], HEX);
      myFile.print(arg1[i], HEX);
      myFile.print(arg2[i], HEX);
      myFile.println();
      if (i % 16 == 15)
        myFile.println();

      pc = i;
      showMem();
      /*
        lcd.setCursor(13, 0);
        lcd.print(op[i], HEX);
        lcd.print(arg1[i], HEX);
        lcd.print(arg2[i], HEX); */

      lcd.setCursor(curX, curY);

      lcd.print(op[i], HEX);
      lcd.print(arg1[i], HEX);
      lcd.print(arg2[i], HEX);
      lcd.print("-");

      curX += 4;
      if (curX == 20) {
        curX = 0;
        curY ++;
        if (curY == 4) {
          curY = 1;
          lcd.setCursor(0, 1);
          lcd.print("                    ");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
        }
      }
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
    reset();
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

    int curX = 0;
    int curY = 1;

    while (true) {

      lcd.setCursor(8, 0);
      lcd.print("@ ");
      if (pc < 16)
        lcd.print("0");
      lcd.print(pc, HEX);
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

        if ( readingOrigin ) {
          switch ( count ) {
            case 0 : pc = decoded * 16; count = 1; break;
            case 1 :
              pc += decoded;
              showMem();
              count = 0;
              readingOrigin = false;
              lcd.setCursor(curX, curY);
              lcd.write("@");
              if (pc < 16)
                lcd.print("0");
              lcd.print(pc, HEX);
              lcd.print("-");
              curX += 4;
              if (curX == 20) {
                curX = 0;
                curY ++;
                if (curY == 4) {
                  curY = 1;
                  lcd.setCursor(0, 1);
                  lcd.print("                    ");
                  lcd.setCursor(0, 2);
                  lcd.print("                    ");
                  lcd.setCursor(0, 3);
                  lcd.print("                    ");
                }
              }

              break;
            default : break;
          }

        } else {

          lcd.setCursor(curX, curY);
          lcd.write(b);

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

              lcd.setCursor(curX, curY);
              lcd.write("-");

              curX++;
              if (curX == 20) {
                curX = 0;
                curY++;
                if (curY == 4) {
                  curY = 1;
                  lcd.setCursor(0, 1);
                  lcd.print("                    ");
                  lcd.setCursor(0, 2);
                  lcd.print("                    ");
                  lcd.setCursor(0, 3);
                  lcd.print("                    ");
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

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loaded @ ");
    if (pc < 16)
      lcd.print("0");
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

  // lcd.clear();

  reset();
  pc = firstPc;

}


//
//
//

void writeDisplay() {

  left.writeDisplay();
  right.writeDisplay();

}



void sendString(String string) {

  for (int i = 0; i < 8; i++) {
    sendChar(i, FONT_DEFAULT[string[i] - 32], false);
  }

  writeDisplay();

  delay(DISP_DELAY);

}


void showMem() {

  if (cursor == 0)
    sendChar(2, blink ? NUMBER_FONT[pc / 16 ] : 0, true);
  else
    sendChar(2, NUMBER_FONT[pc / 16 ], false);

  if (cursor == 1)
    sendChar(3, blink ? NUMBER_FONT[pc % 16]  : 0, true);
  else
    sendChar(3, NUMBER_FONT[pc % 16], false);

  if (cursor == 2)
    sendChar(5, blink ? NUMBER_FONT[op[pc]] : 0, true);
  else
    sendChar(5, NUMBER_FONT[op[pc]], false);

  if (cursor == 3)
    sendChar(6, blink ? NUMBER_FONT[arg1[pc]] : 0, true);
  else
    sendChar(6, NUMBER_FONT[arg1[pc]], false);

  if (cursor == 4)
    sendChar(7, blink ? NUMBER_FONT[arg2[pc]] : 0, true);
  else
    sendChar(7, NUMBER_FONT[arg2[pc]], false);

  writeDisplay();

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

  if (cursor == 0)
    sendChar(2, blink ? NUMBER_FONT[ timeHours10 ] : 0, true);
  else
    sendChar(2, NUMBER_FONT[ timeHours10 ], false);

  if (cursor == 1)
    sendChar(3, blink ? NUMBER_FONT[ timeHours1 ] : 0, true);
  else
    sendChar(3, NUMBER_FONT[ timeHours1  ], false);

  if (cursor == 2)
    sendChar(4, blink ? NUMBER_FONT[ timeMinutes10 ] : 0, true);
  else
    sendChar(4, NUMBER_FONT[ timeMinutes10 ], false);

  if (cursor == 3)
    sendChar(5, blink ? NUMBER_FONT[ timeMinutes1 ] : 0, true);
  else
    sendChar(5, NUMBER_FONT[ timeMinutes1  ], false);

  if (cursor == 4)
    sendChar(6, blink ? NUMBER_FONT[ timeSeconds10 ] : 0, true);
  else
    sendChar(6, NUMBER_FONT[ timeSeconds10 ], false);

  if (cursor == 5)
    sendChar(7, blink ? NUMBER_FONT[ timeSeconds1 ] : 0, true);
  else
    sendChar(7, NUMBER_FONT[ timeSeconds1  ], false);

  writeDisplay();


}


void showReg() {

  if (cursor == 0)
    sendChar(3, blink ? NUMBER_FONT[currentReg] : 0, true);
  else
    sendChar(3, NUMBER_FONT[currentReg], false);

  if (cursor == 1)
    sendChar(7, blink ? NUMBER_FONT[reg[currentReg]]  : 0, true);
  else
    sendChar(7, NUMBER_FONT[reg[currentReg]], false);

  writeDisplay();

}

void showProgram() {

  sendChar(7, blink ? NUMBER_FONT[program] : 0, true);
  writeDisplay();

}

void showError() {

  if (blink)
    sendString("  Error  ");

  writeDisplay();

}


void showReset() {

  sendString("  reset ");
  writeDisplay();

}

void showDISP() {

  for (int i = 0; i < showingDisplayDigits; i++)
    sendChar(7 - i, NUMBER_FONT[reg[(i +  showingDisplayFromReg ) % 16]], false);

  writeDisplay();

}

void displayStatus() {

  unsigned long time = millis();
  unsigned long delta = time - lastDispTime;
  unsigned long delta2 = time - lastDispTime2;

  if (delta2 > 300) {
    blink = !blink;
    lastDispTime2 = time;
  }

  if (delta >= 1000) {
    clock1hz = !clock1hz;
    lastDispTime = time;
    advanceTime();
  }

  clock ++;
  clock %= 10;

  digitalWrite(CARRY_LED, carry);
  digitalWrite(ZERO_LED, carry);
  digitalWrite(CLOCK_LED, clock == 0);
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

  clearDisplay();
  sendChar(0, FONT_DEFAULT[status - 32], false);

  if ( currentMode == RUNNING || currentMode == ENTERING_VALUE )
    showDISP();
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

  //
  //
  //

  updateLCD();

}

void LCDLogo() {

  lcd.setCursor(0, 0);
  lcd.print("Busch 2090");
  lcd.setCursor(0, 1);
  lcd.print(" Microtronic");
  lcd.setCursor(0, 2);
  lcd.print("  Mega 2560 Emulator");
  lcd.setCursor(0, 3);
  lcd.print("   by Michael Wessel");

}

void updateLCD() {

  readPushButtons();

  if (curPushButton == ENTER ) {
    refreshLCD = true;
    switch ( displayMode  ) {
      case OFF    : displayMode = PCMEM; lcd.clear(); break;
      case PCMEM  : displayMode = REGD; lcd.clear(); break;
      default     : displayMode = OFF; lcd.clear(); break;
    }
  }

  if ( pc != lastPc || refreshLCD ) {

    lastPc = pc;

    if (displayMode == OFF && refreshLCD ) {

      LCDLogo();

    } else if (displayMode == PCMEM ) {

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
        case OP_MOV  : mnem = "MOV   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_MOVI : mnem = "MOVI  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_AND  : mnem = "AND   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_ANDI : mnem = "ANDI  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_ADD  : mnem = "ADD   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_ADDI : mnem = "ADDI  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_SUB  : mnem = "SUB   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_SUBI : mnem = "SUBI  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_CMP  : mnem = "CMP   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_CMPI : mnem = "CMPI  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_OR   : mnem = "OR    " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_CALL : mnem = "CALL  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_GOTO : mnem = "GOTO  " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_BRC  : mnem = "BRC   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        case OP_BRZ  : mnem = "BRZ   " + hexStringChar[hi] + hexStringChar[lo] ; break;
        default : {
            switch (op2) {
              case OP_MAS  : mnem = "MAS   " + hexStringChar[lo] + " "; break;
              case OP_INV  : mnem = "INV   " + hexStringChar[lo] + " "; break;
              case OP_SHR  : mnem = "SHR   " + hexStringChar[lo] + " "; break;
              case OP_SHL  : mnem = "SHL   " + hexStringChar[lo] + " "; break;
              case OP_ADC  : mnem = "ADC   " + hexStringChar[lo] + " "; break;
              case OP_SUBC : mnem = "SUBC  " + hexStringChar[lo] + " "; break;
              case OP_DIN  : mnem = "DIN   " + hexStringChar[lo] + " "; break;
              case OP_DOT  : mnem = "DOT   " + hexStringChar[lo] + " "; break;
              case OP_KIN  : mnem = "KIN   " + hexStringChar[lo] + " "; break;
              default : {
                  switch (op3) {
                    case OP_HALT   : mnem = "HALT    " ; break;
                    case OP_NOP    : mnem = "NOP     "; break;
                    case OP_DISOUT : mnem = "DISOUT  "; break;
                    case OP_HXDZ   : mnem = "HXDZ    "; break;
                    case OP_DZHX   : mnem = "DZHX    "; break;
                    case OP_RND    : mnem = "RND     "; break;
                    case OP_TIME   : mnem = "TIME    "; break;
                    case OP_RET    : mnem = "RET     "; break;
                    case OP_CLEAR  : mnem = "CLEAR   "; break;
                    case OP_STC    : mnem = "STC     "; break;
                    case OP_RSC    : mnem = "RSC     "; break;
                    case OP_MULT   : mnem = "MULT    "; break;
                    case OP_DIV    : mnem = "DIV     "; break;
                    case OP_EXRL   : mnem = "EXRL    "; break;
                    case OP_EXRM   : mnem = "EXRM    "; break;
                    case OP_EXRA   : mnem = "EXRA    "; break;
                    default        : mnem = "DISP  " + hexStringChar[hi] + hexStringChar[lo]; break;
                  }
                }
            }
          }
      }

      lcd.print(mnem);
      lcd.setCursor(0, 2);
      lcd.print("WR ");
      for (int i = 0; i < 16; i++)
        lcd.print(reg[i], HEX);
      lcd.setCursor(0, 3);
      lcd.print("SR ");
      for (int i = 0; i < 16; i++)
        lcd.print(regEx[i], HEX);

    } else if (displayMode == REGD ) {

      lcd.setCursor(0, 0);
      lcd.print("WR 0123456789ABCDEF");
      lcd.setCursor(3, 1);
      for (int i = 0; i < 16; i++)
        lcd.print(reg[i], HEX);

      lcd.setCursor(0, 2);
      lcd.print("SR 0123456789ABCDEF");
      lcd.setCursor(3, 3);
      for (int i = 0; i < 16; i++)
        lcd.print(regEx[i], HEX);
    }
  }

  if (displayMode != REGD && displayCurFuncKey != NO_KEY) {

    lcd.setCursor(16, 0);

    switch ( displayCurFuncKey ) {
      case CCE  : lcd.print("C/CE"); break;
      case RUN  : lcd.print(" RUN"); break;
      case BKP  : lcd.print(" BKP"); break;
      case NEXT : lcd.print("NEXT"); break;
      case PGM  : lcd.print(" PGM"); break;
      case HALT : lcd.print("HALT"); break;
      case STEP : lcd.print("STEP"); break;
      case REG  : lcd.print(" REG"); break;
      default: break;
    }

    if (millis() - funcKeyTime > 500) {
      displayCurFuncKey = NO_KEY;
      lcd.setCursor(16, 0);
      lcd.print("    ");
    }
  }

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

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading PGM ");
  lcd.print(pgm + 7);

  int curX = 0;
  int curY = 1;

  while (adr < end) {

    lcd.setCursor(15, 0);
    lcd.print("@ ");
    if (pc < 16)
      lcd.print("0");
    lcd.print(pc, HEX);

    delay(10);

    op[start] = EEPROM.read(adr++);
    arg1[start] = EEPROM.read(adr++);
    arg2[start] = EEPROM.read(adr++);

    lcd.setCursor(curX, curY);
    lcd.print(op[start], HEX);
    lcd.print(arg1[start], HEX);
    lcd.print(arg2[start], HEX);
    lcd.print("-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
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

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loaded @ ");
  if (pc < 16)
    lcd.print("0");
  lcd.print(pc, HEX);

  showLoaded();
  delay(500);

  reset();

}

void sendChar(uint8_t pos, uint8_t bits, boolean dot) {

  if (pos < 4) {

    if (pos > 1 )
      pos++;

    left.writeDigitRaw(pos, bits);

  } else {

    if (pos > 5 )
      pos++;

    pos -= 4;

    right.writeDigitRaw(pos, bits);

  }


}

void clearDisplay() {

  left.clear();
  right.clear();
}

void displayOff() {

  clearDisplay();

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

}


void setDisplayToHexNumber(uint32_t number) {

  left.clear();
  right.clear();

  uint32_t r = number % 65536;
  uint32_t l = number / 65536;


  right.printNumber(r, HEX);

  if (l > 0)
    left.printNumber(l, HEX);

  writeDisplay();

}

void showLoaded() {

  lcd.clear(); 
  lcd.write("Loaded @ "); 
  lcd.print(pc, HEX); 
  
  sendString(" loaded ");   
  sendChar(4, NUMBER_FONT[program], false);
  writeDisplay();
  delay(DISP_DELAY);

  sendString("   at   ");

  sendChar(3, NUMBER_FONT[pc / 16], false);
  sendChar(4, NUMBER_FONT[pc % 16], false);
  writeDisplay();
  
  delay(DISP_DELAY);
  

}

void clearStack() {

  sp = 0;

}

void reset() {

  lcd.clear();

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

  curInput = 0;
  inputs = 0;
  outputs = 0;

  displayMode = OFF;
  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  refreshLCD = true;

}

void clearMem() {

  cursor = CURSOR_OFF;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PGM 5 LOAD 000");

  int curX = 0;
  int curY = 1;

  for (pc = 0; pc < 255; pc++) {

    lcd.setCursor(15, 0);
    lcd.print("@ ");
    if (pc < 16)
      lcd.print("0");
    lcd.print(pc, HEX);

    op[pc] = 0;
    arg1[pc] = 0;
    arg2[pc] = 0;

    lcd.setCursor(curX, curY);
    lcd.print("000-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
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

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PGM 6 LOAD F01");

  int curX = 0;
  int curY = 1;

  for (pc = 0; pc < 255; pc++) {

    lcd.setCursor(15, 0);
    lcd.print("@ ");
    if (pc < 16)
      lcd.print("0");
    lcd.print(pc, HEX);

    op[pc] = 15;
    arg1[pc] = 0;
    arg2[pc] = 1;

    lcd.setCursor(curX, curY);
    lcd.print("F01-");

    curX += 4;
    if (curX == 20) {
      curX = 0;
      curY ++;
      if (curY == 4) {
        curY = 1;
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
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

      currentMode = STOPPED;
      cursor = CURSOR_OFF;
      break;

    case RUN :
      currentMode = RUNNING;
      displayOff();
      clearStack();
      jump = true; // don't increment PC !
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
        curInput = curKeypadKey;
        reg[currentInputRegister] = curInput;
        carry = false;
        zero = reg[currentInputRegister] == 0;
        currentMode = RUNNING;
      }

      break;

    case ENTERING_TIME :

      if (keypadPressed ) {

        curInput = curKeypadKey;

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

        program = curKeypadKey;
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
            lcd.clear();
            lcd.print("PGM 3 ENTER TIME");
            cursor = 0;
            break;

          case 4 :

            currentMode = SHOWING_TIME;
            lcd.clear();
            lcd.print("PGM 4 SHOW TIME");
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
            } else
              error = true;
        }

      }

      break;

    case ENTERING_ADDRESS_HIGH :

      if (keypadPressed) {
        cursor = 1;
        pc = curKeypadKey * 16;
        currentMode = ENTERING_ADDRESS_LOW;
      }

      break;

    case ENTERING_ADDRESS_LOW :

      if (keypadPressed) {
        cursor = 2;
        pc += curKeypadKey;
        currentMode = ENTERING_OP;
      }

      break;

    case ENTERING_OP :

      if (keypadPressed) {
        cursor = 3;
        op[pc] = curKeypadKey;
        currentMode = ENTERING_ARG1;
      }

      break;

    case ENTERING_ARG1 :

      if (keypadPressed) {
        cursor = 4;
        arg1[pc] = curKeypadKey;
        currentMode = ENTERING_ARG2;
      }

      break;

    case ENTERING_ARG2 :

      if (keypadPressed) {
        cursor = 2;
        arg2[pc] = curKeypadKey;
        currentMode = ENTERING_OP;
      }

      break;

    case RUNNING:
      run();
      break;

    case ENTERING_REG:

      if (keypadPressed)
        currentReg = curKeypadKey;

      break;

    case INSPECTING :

      if (keypadPressed)
        reg[currentReg] = curKeypadKey;

      break;

  }

}

void run() {

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

  readFunctionKeys();

  int keypadKeyRaw = keypad.getKey();

  if (keypadKeyRaw != NO_KEY) {
    curKeypadKey = keypadKeyRaw - 1;
    keypadPressed = true;
    refreshLCD = true;
  } else {
    keypadPressed = false;
    curKeypadKey = NO_KEY;
  }

  //
  //
  //

  displayStatus();
  refreshLCD = false;
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

  cpu_delay = analogRead(CPU_THROTTLE_ANALOG_PIN) / CPU_THROTTLE_DIVISOR;
  boolean update = false;

  if (cpu_delay < CPU_MIN_THRESHOLD) {
    cpu_delay = 0;
    update = last_cpu_delay != 0;
  } else if (cpu_delay > CPU_MAX_THRESHOLD) {
    cpu_delay = 100;
    update = last_cpu_delay != 100;
  }

  update = update || abs(cpu_delay - last_cpu_delay) > CPU_DELTA_DISP;

  if ( update )  {
    lcd.setCursor(16, 0);

    int percent = 100 - cpu_delay;
    if (percent < 100)
      lcd.print(0);
    if (percent < 10)
      lcd.print(0);

    lcd.print(percent);
    lcd.print("%");

    last_cpu_delay = cpu_delay;
  }

  if (cpu_delay > 0)
    delay(cpu_delay);

}
