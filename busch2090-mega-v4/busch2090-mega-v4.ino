/*
  A Busch 2090 Microtronic Emulator for Arduino Mega 2560
  Optional support for
  - Emic 2 TTS Speech Module
  - SDCard + Ehternet shield

  See #define SDCARD
  See #define SPEECH

  Version 4 (c) Michael Wessel, March 16, 2022 

  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  With Contributions from Lilly 

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

  - optional EMic 2 speech module

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

#define VERSION "4"

#define SPEECH // comment out if no Emic 2 present
#define SDCARD // comment out if no SDCard shield present 

//
//
//

#include <avr/pgmspace.h>

#if defined (SDCARD)
#include <SdFat.h>
#endif

#include <SPI.h>
#include <Keypad.h>
#include <TM16XXFonts.h>
#include <LiquidCrystal.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <SoftwareSerial.h>

//
// SD Card
//

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

// PGM C - Speaking Counter 
const char PGMC[] PROGMEM = "F20 100 131 3FA 700 3FF 3FB 510 FB1 9A0 E01 C03 ";

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

//
//
//

#if defined (SDCARD)
#define SDCARD_CHIP_SELECT 4

SdFat SD;
SdFile root;
#endif

//
// hardware configuration / wiring
//

//
// serial interface to Emic 2 TTS module
// note that only certain pins can be used
// for RX_SPEECH (see software serial library doc)
//

#if defined (SPEECH)
#define RX_SPEECH A8
#define TX_SPEECH 53
#endif

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

const int pushButtons[] = { RESET, BACK, RIGHT, UP, DOWN, LEFT, CANCEL, ENTER };

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
#define DOT_4 18 // PIN 1 didn't work for hardware experiments for some reason

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

const int dinButtons[] = { DIN_BUTTON_1, DIN_BUTTON_2, DIN_BUTTON_3, DIN_BUTTON_4 };

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

const int functionButtons[] = { CCE, RUN, BKP, NEXT, PGM, HALT, STEP, REG };

//
// CPU speed throttle potentiometer
//

#define CPU_THROTTLE_ANALOG_PIN A0

//
// for initialization of random generator
//

#define RANDOM_ANALOG_PIN A5

//
// LCD panel pins
//

LiquidCrystal lcd(13, 12, 11, 7, 9, 8); // we need pin 10 for SD card!

typedef char TwoDigits[3];
typedef char LCDLine[21];

LCDLine mnemonic;
LCDLine file;
LCDLine speakFile;

int lcdCursor = 0;

//
// 2 Adafruit 7Segment LED backpacks
//

Adafruit_7segment dispRight = Adafruit_7segment();
Adafruit_7segment dispLeft  = Adafruit_7segment();

#define DISP_DELAY 200

//
// HEX 4x4 matrix keypad
//

#define ROWS 4
#define COLS 4

const char keys[ROWS][COLS] = { // plus one because 0 = no key pressed!
  {0xD, 0xE, 0xF, 0x10},
  {0x9, 0xA, 0xB, 0xC},
  {0x5, 0x6, 0x7, 0x8},
  {0x1, 0x2, 0x3, 0x4}
};

byte colPins[COLS] = {37, 35, 33, 31}; // columns
byte rowPins[ROWS] = {29, 27, 25, 23}; // rows

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//
// Emic 2 TTS
//

#if defined (SPEECH)

SoftwareSerial emicSerial =  SoftwareSerial(RX_SPEECH, TX_SPEECH);

boolean speechOn = false; 

#endif

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
// current nibble for Emic 2
//

byte nibbleLo = 255;
byte nibbleHi = 255;

//
// current PGM program
//

byte program;

//
// CPU speed / delay from throttle potentiometer
//

int cpu_delay = 0;

unsigned long lastRunTime = 0; 

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

boolean clock = false;
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
// setup Arduino
//

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

  clock1hz = !clock1hz;

  if (clock1hz)
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

void setup() {

  Serial.begin(9600);

  //
  // SD Card init
  //


#if defined (SDCARD)

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  if (! SD.begin(SDCARD_CHIP_SELECT, SPI_HALF_SPEED)) {
    lcd.clear();
    lcd.print("SD init failed!");
    delay(2000);
  }

#endif

  //
  // setup Emic 2 TSS
  //

#if defined (SPEECH)

  pinMode(RX_SPEECH, INPUT);
  pinMode(TX_SPEECH, OUTPUT);

  emicSerial.begin(9600);

#endif

  analogReference(EXTERNAL);

  //
  // init displays
  //

  dispRight.begin(0x71);
  dispLeft.begin(0x70);

  lcd.begin(20, 4);
  LCDLogo();

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  init  ");

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

  pinMode(DIN_1, INPUT);
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
  //
  //

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  ready ");

  //
  // Emic 2 init
  //

  emicInit();

  initializeClock();

}

void emicStop() {

#if defined (SPEECH)

  emicSerial.flush();
  emicSerial.print('X');
  emicSerial.print('\r');

#endif

}

void emicInit() {

  emicStop();

#if defined (SPEECH)

  emicSerial.print('\n');
  emicSerial.print("N0");
  emicSerial.print('\n');

  emicSerial.print("V3");
  emicSerial.print('\n');

  emicSerial.print("W300");
  emicSerial.print('\n');

  delay(100);

  speakAndWait("Microtronic Computer System ready.");

#endif

}

//
//
//

boolean enterPending = false;

void speakInit() {

#if defined (SPEECH)

  if (speechOn) {
    speakEnter();
    speakWait(); 
    emicSerial.print('S');
    enterPending = true;
  }
  
#endif

}

void speakEnter() {

#if defined (SPEECH)

  if (speechOn) {
    emicSerial.print('\n');
    // enterPending = false;
    delay(50);
  }
  
#endif

}

void speakSend(String message) {

#if defined (SPEECH)
  if (speechOn) 
    emicSerial.print(message);
#endif

}

void speakSendChar(char c) {

#if defined (SPEECH)
  if (speechOn)   
    emicSerial.print(c);
#endif

}

void speakAndWait(String message) {

#if defined (SPEECH)
  if (speechOn) {
    speakInit();
    speakSend(message);
    speakEnter();
    speakWait();
  }
#endif

}

void speakWait() {

#if defined (SPEECH)
  if (speechOn) {
    while (emicSerial.read() != ':');   
    delay(10);                         
    emicSerial.flush();                
  }
#endif

}

//
//
//

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

  speakAndWait("Choose file.");

  lcd.clear();

  int no = 1;
  int count = countFiles();
  selectFileNo(no);

  unsigned long last = millis();
  boolean blink = false;

  readPushButtons();

  while ( curPushButton != ENTER ) {

    lcd.setCursor(0, 0);
    lcd.print("Load Program ");
    lcd.print(no);
    lcd.print(" / ");
    lcd.print(count);
    lcd.print("      "); // erase previoulsy left characters if number got smaller

    readPushButtons();

    if ( millis() - last > 100) {

      last = millis();
      lcd.setCursor(0, 1);
      blink = !blink;

      if (blink)
        lcd.print("                ");
      else
        lcd.print(file);
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

  lcd.clear();

  speakAndWait("Create file.");
  clearFileBuffer();
  file[0] = 'P';

  int cursor = 0;
  int length = 1;

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
      lcd.print(file);
      lcd.setCursor(cursor, 1);

      blink = !blink;

      if (blink)
        if (file[cursor] == ' ' )
          lcd.print("_");
        else
          lcd.print(file[cursor]);
      else
        lcd.print("_");
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
	lcd.clear();
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
    lcd.clear();
    lcd.print("*** ABORT ***");
    speakAndWait("Saving aborted.");
    delay(500);
    reset();
    pc = oldPc;
    return;
  }

  if (SD.exists(file)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Overwriting");
    lcd.setCursor(0, 1);
    lcd.print(file);
    SD.remove(file);
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving");
  lcd.setCursor(0, 1);
  lcd.print(file);

  int i = 0;
  while ( file[i] != 0 && file[i] != '.') {
    speakFile[i] = file[i];
    i++;
  }

  speakInit();
  speakSend("Now saving ");
  speakSend(speakFile);
  speakEnter();

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
    lcd.print(file);
    speakInit();
    speakSend("Saved ");
    speakSend(speakFile);
    speakEnter();

  } else {
    lcd.clear();
    lcd.print("*** ERROR ! ***");
    speakAndWait("Error during saving occured.");
  }

  lcd.clear();
  myFile.close();

  reset();
  pc = oldPc;

#endif

}


void loadProgram() {

#if defined (SDCARD)
  lcd.clear();

  int aborted = selectFile();

  if ( aborted == -1 ) {
    lcd.clear();
    lcd.print("*** ABORT ***");
    speakAndWait("Loading aborted.");

    reset();
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading");
  lcd.setCursor(0, 1);
  lcd.print(file);

  int i = 0;
  while ( file[i] != 0 && file[i] != '.') {
    speakFile[i] = file[i];
    i++;
  }

  speakInit();
  speakSend("Now loading ");
  speakSend(speakFile);
  speakEnter();

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
    lcd.print(file);

    speakInit();
    speakSend("Loaded ");
    speakSend(speakFile);
    speakEnter();

    showLoaded(false);


  } else {
    lcd.clear();
    lcd.print("*** ERROR ! ***");
    speakAndWait("Error during loading occured.");

  }

  // lcd.clear();

  reset();
  pc = firstPc;
#endif

}


//
//
//

void writeDisplay() {

  dispLeft.writeDisplay();
  dispRight.writeDisplay();

}



void sendString(String string) {

  for (int i = 0; i < 8; i++) {
    sendChar(i, FONT_DEFAULT[string[i] - 32], false);
  }

  writeDisplay();

  delay(DISP_DELAY);

}


void showMem() {

  int adr = pc;

  if ( currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW )
    adr = breakAt;

  if (cursor == 0)
    sendChar(2, blink ? NUMBER_FONT[adr / 16 ] : 0, true);
  else
    sendChar(2, NUMBER_FONT[adr / 16 ], false);

  if (cursor == 1)
    sendChar(3, blink ? NUMBER_FONT[adr % 16]  : 0, true);
  else
    sendChar(3, NUMBER_FONT[adr % 16], false);


  if (cursor == 2)
    sendChar(5, blink ? NUMBER_FONT[op[adr]] : 0, true);
  else
    sendChar(5, NUMBER_FONT[op[adr]], false);

  if (cursor == 3)
    sendChar(6, blink ? NUMBER_FONT[arg1[adr]] : 0, true);
  else
    sendChar(6, NUMBER_FONT[arg1[adr]], false);

  if (cursor == 4)
    sendChar(7, blink ? NUMBER_FONT[arg2[adr]] : 0, true);
  else
    sendChar(7, NUMBER_FONT[arg2[adr]], false);

  writeDisplay();

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

TwoDigits twoDigits;

void convertTwoDigits(byte a, byte b) {

  if (a > 0)  {
    twoDigits[0] = '0' + a;
  } else
    twoDigits[0] = ' ';

  twoDigits[1] = '0' + b;

  twoDigits[2] = 0;

}

void speakTime() {

  speakSend(" It is ");
  convertTwoDigits(timeHours10, timeHours1);
  speakSend(twoDigits);
  speakSend(" hours, ");
  convertTwoDigits(timeMinutes10, timeMinutes1);
  speakSend(twoDigits);
  speakSend(" minutes and ");
  convertTwoDigits(timeSeconds10, timeSeconds1);
  speakSend(twoDigits);
  speakSend(" seconds.");

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

  if (delta > 300) {

    blink = !blink;

    lastDispTime = time;
    
    lcd.setCursor(16, 0);

    int percent = 100 - cpu_delay;
    if (percent < 100)
      lcd.print(0);
    if (percent < 10)
      lcd.print(0);

    lcd.print(percent);
    lcd.print("%");

  }

  digitalWrite(CARRY_LED, carry);
  digitalWrite(ZERO_LED, carry);
  digitalWrite(CLOCK_LED, clock);

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

  clearDisplay();
  sendChar(0, FONT_DEFAULT[status - 32], false);

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

  lcd.setCursor(0, 0);
  lcd.print("Busch 2090");
  lcd.setCursor(0, 1);
  lcd.print(" Microtronic");
  lcd.setCursor(0, 2);
  lcd.print("  Mega 2560 Emulator");
  lcd.setCursor(0, 3);
  lcd.print("   by Michael Wessel");

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
    case OFF    : displayMode = PCMEM; lcd.clear(); break;
    case PCMEM  : displayMode = REGD; lcd.clear(); break;
    default     : displayMode = OFF; lcd.clear(); break;
    }
  }

  if ( pc != lastPc || refreshLCD ) {

    lastPc = pc;

    if (displayMode == OFF && refreshLCD ) {

      switch ( currentMode ) {

      case ENTERING_TIME :
	lcd.clear();
	lcd.print("PGM 3 ENTER TIME");
	break;

      case SHOWING_TIME :
	lcd.clear();
	lcd.print("PGM 4 SHOW TIME");
	break;

      default:

	LCDLogo();
      }

    } else if (displayMode == PCMEM ) {

      lcd.setCursor(0, 0);
      lcd.print("PC BKP OP  MNEM");
      lcd.setCursor(0, 1);

      if (pc < 16)
        lcd.print(0);
      lcd.print(pc, HEX);

      lcd.setCursor(4, 1);

      if (breakAt < 16)
        lcd.print(0);
      lcd.print(breakAt, HEX);

      lcd.setCursor(7, 1);
      lcd.print(op[pc], HEX);
      lcd.print(arg1[pc], HEX);
      lcd.print(arg2[pc], HEX);

      lcd.setCursor(11, 1);

      getMnem(false);

      lcd.print(mnemonic);
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


  if (curPushButton == LEFT ) {
    speakInfo();
  } else if (curPushButton == RIGHT) {
    speakLEDDisplay();
  } else if (curPushButton == UP ) {
    speakAndWait("I am a Microtronic Computer System Emulator running on an Arduino. I was developed by Doctor Wessel in Paalo Ahlto, California, U S A, in January 2022.");
  } else if ( curPushButton == DOWN ) {
    speakInit();
    speakSend("Microtronic Computer System Emulator Version ");
    speakSend(VERSION);
    speakEnter();
  } else if ( curPushButton == BACK ) {
#if defined (SPEECH)
    speechOn = true;     
    speakAndWait( "Speech on." );
#endif
  } else if ( curPushButton == CANCEL) {
#if defined (SPEECH)
    speakAndWait( "Speech off." );
    speechOn = false;     
#endif
  }

  refreshLCD = false;

}

void speakInfo() {

  speakInit();

  speakSend(" Current system status is ");

  if ( error )
    speakSend("error. Please reset to continue");
  else if ( currentMode == STOPPED )
    speakSend("halted");
  else if (currentMode == ENTERING_ADDRESS_HIGH )
    speakSend("entering high nibble of address");
  else if ( currentMode == ENTERING_ADDRESS_LOW )
    speakSend("entering low nibble of address");
  else if (currentMode == ENTERING_BREAKPOINT_HIGH )
    speakSend("entering high nibble of breakpoint");
  else if ( currentMode == ENTERING_BREAKPOINT_LOW )
    speakSend("entering low nibble of breakpoint");
  else if (currentMode == ENTERING_OP ||
           currentMode == ENTERING_ARG1 ||
           currentMode == ENTERING_ARG2 ) {
    speakSend("entering instruction at ");
    speakSend(hexStringChar[ int(pc / 16) ]);
    speakSend(" , ");
    speakSend(hexStringChar[ pc % 16 ]);
  } else if (currentMode == RUNNING)
    speakSend("running program");
  else if (currentMode == ENTERING_REG)
    speakSend("entering register");
  else if ( currentMode == INSPECTING )
    speakSend("entering register content");
  else if (currentMode == ENTERING_VALUE ) {
    speakSend("entering value into register ");
    speakSend( hexStringChar[ currentInputRegister ]);
  } else if (currentMode == ENTERING_TIME )
    speakSend("entering time");
  else if (currentMode == SHOWING_TIME )
    speakSend("showing time");
  else if (currentMode == ENTERING_PROGRAM )
    speakSend("entering program number");
  else
    speakSend("unknown");

  speakSend(".");
  speakEnter();

}


void speakLEDDisplay() {

  speakInit();

  speakSend("The display shows ");

  if ( currentMode == RUNNING || currentMode == ENTERING_VALUE ) {

    for (int i = showingDisplayDigits - 1; i >= 0; i--) {
      speakSend( hexStringChar[ reg[(i +  showingDisplayFromReg ) % 16 ] ] );
      if (i > 0)
        speakSend(" , ");
    }

    if (showingDisplayDigits < 1)  {
      speakSend("nothing");
    }

  } else if ( currentMode == ENTERING_REG ) {

    speakSend("current register number ");
    speakSend( hexStringChar[currentReg]);

  } else if ( currentMode == INSPECTING ) {

    speakSend("current register content ");
    speakSend(hexStringChar[reg[currentReg]]);

  } else if ( currentMode == ENTERING_PROGRAM ) {

    speakSend("current program number ");
    speakSend(hexStringChar[program]);

  } else if ( currentMode == ENTERING_TIME ) {

    speakSend("current time to set. ");
    speakTime();

  } else if ( currentMode == SHOWING_TIME ) {

    speakSend("current time. ");
    speakTime();

  } else if ( error ) {

    speakSend("error. Please reset.");

  } else {

    if ( currentMode == ENTERING_BREAKPOINT_HIGH || currentMode == ENTERING_BREAKPOINT_LOW )
      speakSend(" breakpoint at address ");
    else
      speakSend(" address ");
    speakSend(hexStringChar[ int(pc / 16) ]);
    speakSend(" , ");
    speakSend(hexStringChar[ pc % 16 ]);
    speakSend(" , with instruction ");
    getMnem(true);
    speakSend(mnemonic);
    speakSend(". Code ");
    speakSend(hexStringChar[ op[pc] ]);
    speakSend(" , ");
    speakSend(hexStringChar[ arg1[pc] ]);
    speakSend(" , ");
    speakSend(hexStringChar[ arg2[pc] ]);
    speakSend(".");

  }

  speakSend(" The current output value is ");
  speakSend( hexStringChar[ outputs ]);
  speakSend(" .");

  speakEnter();


}

int decodeHex(char c) {

  if (c >= 65 && c <= 70 )
    return c - 65 + 10;
  else if ( c >= 48 && c <= 67 )
    return c - 48;
  else return -1;

}



void loadPGMProgram(byte pgm, byte start) {

  cursor = CURSOR_OFF;
  char *pgm_string = (char *)pgm_read_word(&PGMROM[pgm]);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading PGM ");
  lcd.print(pgm + 7);
  int curX = 0;
  int curY = 1;
  int origin = start; 

  while (pgm_read_byte(pgm_string))
    {

      op[start] = decodeHex(pgm_read_byte(pgm_string++));
      arg1[start] = decodeHex(pgm_read_byte(pgm_string++));
      arg2[start] = decodeHex(pgm_read_byte(pgm_string++));

      lcd.setCursor(15, 0);
      lcd.print("@ ");
      if (pc < 16)
	lcd.print("0");
      lcd.print(pc, HEX);

      delay(10);

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

      if (start == 256)
	{
	  exit(1);
	}

      pgm_string++; // skip over space

    }

  pc = origin;
  currentMode = STOPPED;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loaded @ ");
  if (pc < 16)
    lcd.print("0");
  lcd.print(pc, HEX);

  showLoaded(false);
  delay(500);

  reset();

}

void sendChar(uint8_t pos, uint8_t bits, boolean dot) {

  if (pos < 4) {

    if (pos > 1 )
      pos++;

    dispLeft.writeDigitRaw(pos, bits);

  } else {

    if (pos > 5 )
      pos++;

    pos -= 4;

    dispRight.writeDigitRaw(pos, bits);

  }


}

void clearDisplay() {

  dispLeft.clear();
  dispRight.clear();

}

void displayOff() {

  clearDisplay();
  writeDisplay();

  dispOff = true;
  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

}


void setDisplayToHexNumber(uint32_t number) {

  dispLeft.clear();
  dispRight.clear();

  uint32_t r = number % 65536;
  uint32_t l = number / 65536;


  dispRight.printNumber(r, HEX);

  if (l > 0)
    dispLeft.printNumber(l, HEX);

  writeDisplay();

}

void showLoaded(boolean showLCD) {

  speakAndWait("Loaded");

  if (showLCD) {
    lcd.clear();
    lcd.write("Loaded @ ");
    lcd.print(pc, HEX);
  }

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

  emicStop();

  speakAndWait("Reset");

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

  nibbleLo = 255;
  nibbleHi = 255;

  displayMode = OFF;

  showingDisplayFromReg = 0;
  showingDisplayDigits = 0;

  dispOff = false;
  lastInstructionWasDisp = false;

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

    jump = true;
    currentMode = STOPPED;
    cursor = CURSOR_OFF;

    dispOff = false;
    lastInstructionWasDisp = false;

    speakAndWait("Halt");
    lcd.clear();
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

    speakAndWait("Run");
    lcd.clear();
    refreshLCD = true;

    lastRunTime = millis(); 

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

    speakAndWait("Next");
    lcd.clear();
    refreshLCD = true;
    jump = true;

    break;

  case REG :

    if (currentMode != ENTERING_REG) {
      currentMode = ENTERING_REG;
      cursor = 0;
      speakAndWait("Register");
    } else {
      currentMode = INSPECTING;
      cursor = 1;
      speakAndWait("Content");
    }
    lcd.clear();
    refreshLCD = true;

    break;

  case STEP :

    currentMode = RUNNING;
    oneStepOnly = true;
    dispOff = false;
    jump = true; // don't increment PC !

    speakAndWait("Step");
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

    speakAndWait("Breakpoint");
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

    speakAndWait("Clear");
    lcd.clear();
    refreshLCD = true;

    break;

  case PGM :

    if ( currentMode != ENTERING_PROGRAM ) {
      cursor = 0;
      currentMode = ENTERING_PROGRAM;
    }

    speakAndWait("Program");
    lcd.clear();
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
	speakInit();
	speakSend("Bad program");
	speakEnter();
	break;

      case 1 :
	loadProgram();
	break;

      case 2 :

	saveProgram();
	break;

      case 3 :
	speakInit();
	speakSend("Set time");
	speakEnter();
	currentMode = ENTERING_TIME;
	lcd.clear();
	lcd.print("PGM 3 ENTER TIME");
	cursor = 0;
	break;

      case 4 :
	speakInit();
	speakSend("Show time");
	speakEnter();
	currentMode = SHOWING_TIME;
	lcd.clear();
	lcd.print("PGM 4 SHOW TIME");
	cursor = CURSOR_OFF;
	break;

      case 5 : // clear mem
	speakInit();
	speakSend("Clear memory");
	speakEnter();
	clearMem();
	break;

      case 6 : // load NOPs
	speakInit();
	speakSend("Load NOPPs");
	speakEnter();
	loadNOPs();
	break;

      default : // load other

	if (program - 7 < programs ) {
	  speakInit();
	  speakSend("Load ");
	  speakSend( hexStringChar [ program ]);
	  speakEnter();
	  loadPGMProgram(program - 7, 0);
	} else {
	  speakInit();
	  speakSend("Bad program");
	  speakEnter();
	}
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

  case ENTERING_BREAKPOINT_HIGH :

    if (keypadPressed) {
      cursor = 1;
      breakAt = curKeypadKey * 16;
      currentMode = ENTERING_BREAKPOINT_LOW;
    }

    refreshLCD = true;

    break;

  case ENTERING_BREAKPOINT_LOW :

    if (keypadPressed) {
      cursor = 0;
      breakAt += curKeypadKey;
      currentMode = ENTERING_BREAKPOINT_HIGH;
    }

    refreshLCD = true;

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

  if (! oneStepOnly ) {
    unsigned long time = millis();
    if ( (time - lastRunTime) < cpu_delay) {
      return; 
    } else {
      lastRunTime = time;
      clock = !clock;
    }
  }

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
  
  byte speechOn0 = speechOn; 

  switch (op1) {
  case OP_MOV :

    reg[d] = reg[s];
    zero = reg[d] == 0;

    if (d == s) {
      speechOn = true; 
      if ( d == 10) {
	speakInit(); 
	nibbleHi = 255;
	nibbleLo = 255;
      } else if ( d == 11 ) {
	speakWait(); 
	nibbleHi = 255;
	nibbleLo = 255;
      } else if ( d == 15) {
	speakEnter();    
	nibbleHi = 255;
	nibbleLo = 255;
      } else {     
	if ( nibbleHi == 255)
	  nibbleHi = d;
	else {
	  nibbleLo = d;
	  char c = char(nibbleHi * 10 + nibbleLo);
	  if (c == 13 || c >= 32 && c <= 125 ) 
	    speakSendChar(c);
	  nibbleHi = 255;
	}
      }
      speechOn = speechOn0; 
    }

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

    if (n == 15) {
      speechOn = true;  
      if ( d == 10) {
	speakInit(); 
	nibbleHi = 255;
	nibbleLo = 255;
      } else if ( d == 11 ) {
	speakWait(); 
	nibbleHi = 255;
	nibbleLo = 255;
      } else if ( d == 15) {
	speakEnter();    
	nibbleHi = 255;
	nibbleLo = 255;
      }
      speechOn = speechOn0; 
    }
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

    if (n == 0) {
      // decimal encoded 
      speechOn = true; 
      nibbleLo = reg[d];
      nibbleHi = reg[(d+1) % 16];
      char c = char(nibbleHi * 10 + nibbleLo);
      if (c == 13 || c >= 32 && c <= 125 ) 
         speakSendChar(c);      
      speechOn = speechOn0; 
    }

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

     if (n == 0) {
      // HEX encoded 
      speechOn = true; 
      nibbleLo = reg[d];
      nibbleHi = reg[(d+1) % 16];
      char c = char(nibbleHi * 16 + nibbleLo);
      if (c == 13 || c >= 32 && c <= 125 ) 
         speakSendChar(c);      
      speechOn = speechOn0; 
    }

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

      carry = reg[d] & 1;
      reg[d] >>= 1;
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
      //speakAndWait("input");

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
	  clearDisplay();
	  writeDisplay();
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

  int keypadKeyRaw = keypad.getKey();

  if (keypadKeyRaw != NO_KEY) {
    curKeypadKey = keypadKeyRaw - 1;
    keypadPressed = true;
    refreshLCD = true;
    speakAndWait(hexStringChar[curKeypadKey]);
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

  cpu_delay = min(analogRead(CPU_THROTTLE_ANALOG_PIN) / 10, 100);

  if (cpu_delay < 3) 
    cpu_delay = 0;

}
