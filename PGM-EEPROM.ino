/*

  A Busch 2090 Microtronic Emulator for Arduino Uno / R3
  This program stores the PGM example programs into the EEPROM. 
  It needs to be run before the busch2090.ino sketch, otherwise
  the emulator will fail to initialize and will not work properly. 

  Version 1.0 (c) Michael Wessel, January 18 2016 
  
  michael_wessel@gmx.de
  miacwess@gmail.com
  http://www.michael-wessel.info

  With Contributions from Martin Sauter (PGM 7) 
  See http://mobilesociety.typepad.com/

  Hardware requirements:
  - 4x4 hex keypad (HEX keypad for data and program entry)
  - TM1638 8 digit 7segment display with 8 LEDs and 8 buttons (function keys)

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

#include <TM1638.h>
#include <TM16XXFonts.h>
#include <EEPROM.h>

#define DISP_DELAY 200

// use pins 14, 15, 16 for TM1638 module
TM1638 module(14, 15, 16);

#define PROGRAMS 5
String programs [PROGRAMS] = {

  // PGM 7 - many thanks to Martin Sauter for retrieving this from original Microtronic and entering it here!!
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

};

//
//
//

void setup() {
  Serial.begin(9600);

  sendString("  BUSCH ");
  sendString("  2090  ");
  sendString("  eeprog");

}


void sendString(String string) {

  for (int i = 0; i < 8; i++) {

    module.sendChar(i, FONT_DEFAULT[string[i] - 32], false);

  }

  delay(DISP_DELAY);

}


byte decodeHex(char c) {

  if (c >= 65)
    return c - 65 + 10;
  else
    return c - 48;

}

void enterProgram(String string, int start, byte num) {

  int i = 0;

  while ( string[i] ) {

    EEPROM.write(start++, decodeHex(string[i++]));
    module.setLEDs( i % 16);
    EEPROM.write(start++, decodeHex(string[i++]));
    module.setLEDs( i % 16);
    EEPROM.write(start++, decodeHex(string[i++]));
    module.setLEDs( i % 16);
    i++; // skip over "-"

    delay(5);

    if (start > 1023) {
      module.setDisplayToHexNumber( 0xEEEEEEEE, 0, true);
      delay(2000);
      exit(1);
    }

  };

  sendString(" loaded ");
  displayOff();
  module.sendChar(7, NUMBER_FONT[num], false);
  delay(DISP_DELAY);

}

void displayOff() {

  for (int i = 0; i < 8; i++)
    module.sendChar(i, 0, false);

}

void loop() {

  int i = 0;

  byte numPrograms = PROGRAMS;

  int adr = 0;

  EEPROM.write(adr++, numPrograms);

  for (int n = 0; n < numPrograms; n++) {

    String program = programs[n];
    int length = (program.length() / 4) * 3;
    EEPROM.write(adr++, length);

  }

  for (int n = 0; n < numPrograms; n++) {

    String program = programs[n];
    int length = (program.length() / 4) * 3;

    enterProgram(program, adr, n);

    adr += length;


  }

  module.setDisplayToHexNumber(adr, 0, true);
  delay(DISP_DELAY);

  //
  // test read back
  //

  adr = 0;

  int n1 = EEPROM.read(adr++);
  module.setDisplayToHexNumber(n1, 0, true);
  delay(DISP_DELAY);

  int startAddresses[16];

  int start = 1;

  for (int n = 0; n < n1; n++) {

    startAddresses[n] = start;
    start += EEPROM.read(adr++) * 3;
    module.setDisplayToHexNumber( startAddresses[n], 0, true);
    delay(DISP_DELAY);
  }

  module.setDisplayToHexNumber( start, 0, true);
  delay(DISP_DELAY);

  sendString("  exit  ");

  exit(0);

}
