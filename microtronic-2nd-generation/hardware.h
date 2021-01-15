// For Microtronic 2nd Generation: 

#define MICRO_SECOND_GEN_BOARD 

// Uncomment this if you are using inverted outputs for the 2095 
// BUSCH_INx output ports (these connect to the 2090's INPUTs) 

#define INVERTED_OUTPUTS_FOR_2095_EMU

// Uncomment if you have a realtime clock at pins 20, 21 (SDA, SCL) I2C
// 

// #define RTCPRESENT

//
// Keypad Tones
// 

#define TONEPIN A0 
#define HEXKEYTONE 440 
#define KEYTONELENGTH 50
#define FUNKEYTONE 880
#define FUNTONELENGTH 50 

//
// SdCard CS Pin 
//

#define SDCARD_CHIP_SELECT 53

//
// Soft Reset Button Pin
//

#define RESET  31

//
// Carry & Zero Flag LEDs 
//
 
#define CARRY_LED     34
#define ZERO_LED      36

//
// LCD Backlight Pins 
// 

#define LIGHT_LED 32

//
// 1 Hz Clock Digital Output Pin 
//

#ifndef MICRO_SECOND_GEN_BOARD
#define CLOCK_OUT 7 
#endif 

#define CLOCK_1HZ_LED 38

//
// DOT Digital Output Pins
//

#define DOT_LED_1 46
#define DOT_LED_2 44
#define DOT_LED_3 42
#define DOT_LED_4 40

#ifndef MICRO_SECOND_GEN_BOARD 
#define DOT_1 16
#define DOT_2 18 
#define DOT_3 20 
#define DOT_4 22
#endif 

//
// DIN digital input
//

//
// DONT' CHANGE THE ORDER; DIFFERENT LAYOUT FOR 2nd GENERATION!!!
//

#define DIN_4 30
#define DIN_3 28
#define DIN_2 26
#define DIN_1 24

//
// For Initialization of Random Generator
//

#define RANDOM_ANALOG_PIN A5

//
// Nokia Display Pins 
//

#define NOKIA5 6
#define NOKIA4 5
#define NOKIA3 4
#define NOKIA2 3
#define NOKIA1 2 

//
// Hexpad Pins 
//

#define HEX_COL_PINS {8, 9, 10, 11}
#define HEX_ROW_PINS {17, 19, 21, 23}

//
// Function Keypad Pins
//

#define FN_COL_PINS {12, 13, 14, 15}
#define FN_ROW_PINS {17, 19, 21, 23}

//
// Busch 2095 Interface Emu 
//

//
// Ports of the Busch 2090 - either connect
// to the IC Socket for the 2095 interface using
// an 14 PIN DIP IC socket, or connect to the
// external input / outputs of the 2090.
//

#ifdef MICRO_SECOND_GEN_BOARD 
#define BUSCH_IN1 DOT_LED_1
#define BUSCH_IN2 DOT_LED_2
#define BUSCH_IN3 DOT_LED_3
#define BUSCH_IN4 DOT_LED_4
#endif 

#ifndef MICRO_SECOND_GEN_BOARD 
#define BUSCH_IN1 DOT_1
#define BUSCH_IN2 DOT_2
#define BUSCH_IN3 DOT_3
#define BUSCH_IN4 DOT_4
#endif 


#define BUSCH_OUT1 DIN_1
#define BUSCH_OUT3 DIN_3 


//
// Busch 2090 read / write port operations delays (ms)
//

#define READ_CLOCK_DELAY 4 // SAVE PGM2 Clock Delay 
#define WRITE_CLOCK_DELAY 10 // LOAD PGM1 Clock Delay 
#define READ_DELAY_NEXT_VALUE 200  // SAVE PGM2 Next Word Delay 
#define WRITE_DELAY_NEXT_VALUE 180 // LOAD PGM1 Next Word Delay
