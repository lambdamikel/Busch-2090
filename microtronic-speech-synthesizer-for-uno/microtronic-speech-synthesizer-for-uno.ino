
#include <SoftwareSerial.h>

#define rxPin 2 // Serial input (connects to Emic 2 SOUT)
#define txPin 3 // Serial output (connects to Emic 2 SIN)

#define IN1 A4  // we are using the analog inputs to connect to the Microtronic outputs
#define IN2 A3  // the digital inputs turned out to be difficult for that purpose 
#define IN3 A1 
#define IN4 A0

byte in = 0 ; 
byte in1 = 0 ; 
byte last = 255; 

SoftwareSerial emicSerial =  SoftwareSerial(rxPin, txPin);

char buffer[100]; 

void readInputs() {
  
  long t1 = millis(); 

  byte ar1 = analogRead(IN1) > 1020 ? 1 : 0; 
  byte ar2 = analogRead(IN2) > 1020 ? 2 : 0; 
  byte ar3 = analogRead(IN3) > 1020 ? 4 : 0; 
  byte ar4 = analogRead(IN4) > 1020 ? 8 : 0; 
  
  in = ar1 | ar2 | ar3 | ar4;   
  
  in1 = in;
   
  while (true) {    
    
   if (in == in1) {   
  
    if (millis() - t1 > 7) {
        t1 = 0; 
        if (last != in) {
          last = in; 
          return;        
        } 
    }
  } else
    t1 = millis(); 

  in1 = in; 

  delay(3);
  ar1 = analogRead(IN1) > 1020 ? 1 : 0; 
  ar2 = analogRead(IN2) > 1020 ? 2 : 0; 
  ar3 = analogRead(IN3) > 1020 ? 4 : 0; 
  ar4 = analogRead(IN4) > 1020 ? 8 : 0; 
    
  in = ar1 | ar2 | ar3 | ar4;   
 
  }
  
}

char readInputChar() { 

  byte nibble1 = 0; 
  byte nibble2 = 0; 

  while (true) {
    readInputs(); 
    if (in != 0) {
      if (nibble1 == 0) 
         nibble1 = in-1; 
      else { 
         nibble2 = in-1;      
         return nibble1*10 + nibble2; 
      }
    }
  }
 
}

void readString() {

  int count = 0; 
  char ch = '0'; 
  long r = 0; 
  
  do {

    ch = readInputChar(); 
    //Serial.print(ch); 
    buffer[count++] = ch; 
  } while ( ch != 13 ) ; 
  buffer[count] = 0; 
  
  Serial.println("String received: ");
  Serial.print(buffer); 
  Serial.println(); 
  
}

void setup()  
{
  
  Serial.begin(9600);       
  Serial.println("Serial connection established!");
  
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  
  emicSerial.begin(9600);


  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  pinMode(IN3, INPUT); 
  pinMode(IN4, INPUT); 

}

void loop() 
{

  while (true) {
    
    emicSerial.print('\n');           
    while (emicSerial.read() != ':');  
    delay(10);                          
    emicSerial.flush();     
  
    while (true) { 
      readString();    
      emicSerial.print(buffer);     
    }   
  }
   
}



