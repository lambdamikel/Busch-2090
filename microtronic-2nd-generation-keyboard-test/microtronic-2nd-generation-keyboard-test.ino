#include "hardware.h" 

//
// Keyboard Test Program for the Microtronic 2nd Generation
// (C) 2021 Michael Wessel
// 

#include <Adafruit_PCD8544.h>

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
// Hex keypad
//

#define REPEAT_TIME 200
#define DEBOUNCE_TIME 20

unsigned long funKeyTime  = 0;
unsigned long funKeyTime0 = 0;
unsigned long dispFunKeyTime = 0;

unsigned long hexKeyTime  = 0;

int lastcurHexKey    = 255;
int lastcurFunKey = 255; 

int curHexKey    = NO_KEY;
int curHexKeyRaw = NO_KEY;

//
// Function keypad 
//

int curFunKey    = NO_KEY;
int curFunKeyRaw = NO_KEY;

int displayCurFunKey = NO_KEY; // for LCD display feedback only


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
      ; 

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
// Setup Arduino
//

void setup() {

  //
  // init displays
  //

  digitalWrite(LIGHT_LED, true); 

  display.begin(); 
  display.clearDisplay();
  display.setContrast(57);
  display.setCursor(0,0); 
  display.setTextSize(2); 
  display.println("KEYPAD"); 
  display.println("TEST"); 
  display.println("PROG"); 
  display.display();

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


}


//
// main loop / emulator / shell
//

void loop() {

  readFunKeys();
  readHexKeys(); 
  
  char* keyf = ""; 
  switch ( curFunKey ) {
    case CCE : keyf = "CCE"; break; 
    case PGM : keyf = "PGM"; break; 
    case RUN : keyf = "RUN"; break;  
    case HALT : keyf = "HALT"; break;  
    case BKP :  keyf = "BKP"; break; 
    case STEP : keyf = "STEP"; break; 
    case NEXT :	keyf = "NEXT"; break; 
    case REG : 	keyf = "REG"; break; 
    case LEFT : keyf = "LEFT"; break; 
    case RIGHT : keyf = "RIGHT"; break; 
    case UP  : keyf = "UP"; break;
    case DOWN : keyf = "DOWN"; break; 
    case ENTER : keyf = "ENTER"; break; 
    case CANCEL : keyf = "CANCEL"; break; 
    case BACK : keyf = "BACK"; break; 
    case LIGHT :  keyf = "LIGHT" ; break; 
    default : keyf = "NONE";
  } 

  char* keyh = ""; 
  switch ( curHexKey ) {
    case 0 : keyh = "0"; break; 
    case 1 : keyh = "1"; break; 
    case 2 : keyh = "2"; break;  
    case 3 : keyh = "3"; break;  
    case 4 : keyh = "4"; break; 
    case 5 : keyh = "5"; break; 
    case 6 : keyh = "6"; break; 
    case 7 : keyh = "7"; break; 
    case 8 : keyh = "8"; break; 
    case 9 : keyh = "9"; break; 
    case 10 : keyh = "A"; break;
    case 11 : keyh = "B"; break; 
    case 12 : keyh = "C"; break; 
    case 13 : keyh = "D"; break; 
    case 14 : keyh = "E"; break; 
    case 15 : keyh = "F" ; break; 
    default : keyh = "NONE"; 
  }

  if (( lastcurHexKey != curHexKey) || 
      ( lastcurFunKey != curFunKey)) {

    display.clearDisplay();
    display.setTextSize(2); 
    display.setCursor(0, 0); 
    display.print(keyf);
    display.setCursor(0, 20); 
    display.print(keyh);
    display.display(); 

    lastcurHexKey = curHexKey; 
    lastcurFunKey = curFunKey; 
  
  } 	      

}
