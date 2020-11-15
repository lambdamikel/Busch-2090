// For Microtronic 2nd Generation: Uncomment this!

//#define MICRO_SECOND_GEN_BOARD 


//
// SdCard CS Pin 
//

#define SDCARD_CHIP_SELECT 53

//
// Soft Reset Button Pin
//

#define RESET  49 // soft reset 

//
// Carry & Zero Flag LEDs 
//
 
#define CARRY_LED     38
#define ZERO_LED      36

//
// LCD Backlight Pins 
// 

#define LIGHT_LED 35

//
// 1 Hz Clock Digital Output Pin 
//

#ifndef MICRO_SECOND_GEN_BOARD
#define CLOCK_OUT 7 
#endif 

#define CLOCK_1HZ_LED 34

//
// DOT Digital Output Pins
//

#define DOT_LED_1 40
#define DOT_LED_2 42
#define DOT_LED_3 44
#define DOT_LED_4 46

#ifndef MICRO_SECOND_GEN_BOARD 
#define DOT_1 31 
#define DOT_2 29 
#define DOT_3 27 
#define DOT_4 25
#endif 

//
// DIN digital input
//

#define DIN_1 24
#define DIN_2 26
#define DIN_3 28
#define DIN_4 30

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

#define HEX_COL_PINS {14, 12, 10, 8}
#define HEX_ROW_PINS {15, 13, 11, 9}

//
// Function Keypad Pins
//

#define FN_COL_PINS {22, 20, 18, 16}
#define FN_ROW_PINS {23, 21, 19, 17}

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
#define BUSCH_IN1 DOT_1
#define BUSCH_IN2 DOT_2
#define BUSCH_IN3 DOT_3
#define BUSCH_IN4 DOT_4
#endif 

#ifndef MICRO_SECOND_GEN_BOARD 
#define BUSCH_IN1 DOT_1
#define BUSCH_IN2 DOT_2
#define BUSCH_IN3 DOT_3
#define BUSCH_IN4 DOT_4
#endif 


#define BUSCH_OUT1 DIN_1
#define BUSCH_OUT2 DIN_2
#define BUSCH_OUT3 DIN_3 


//
// Busch 2090 read / write port operations delays (ms)
//

#define READ_CLOCK_DELAY 4 // SAVE PGM2 Clock Delay 
#define WRITE_CLOCK_DELAY 10 // LOAD PGM1 Clock Delay 
#define READ_DELAY_NEXT_VALUE 200  // SAVE PGM2 Next Word Delay 
#define WRITE_DELAY_NEXT_VALUE 180 // LOAD PGM1 Next Word Delay

