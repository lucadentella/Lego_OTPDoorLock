#include <Keypad.h>
#include <Servo.h>

#include <swRTC.h>
#include <sha1.h>
#include <TOTP.h>

// debug print, use #define DEBUG to enable Serial output
// thanks to http://forum.arduino.cc/index.php?topic=46900.0
#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// servo configuration: PIN, door closed/opened position and speed
#define SERVO_PIN     9
#define SERVO_CLOSED  5
#define SERVO_OPENED  100
#define SERVO_DELAY   20

// shared secret is MyLegoDoor
uint8_t hmacKey[] = {0x4d, 0x79, 0x4c, 0x65, 0x67, 0x6f, 0x44, 0x6f, 0x6f, 0x72};
TOTP totp = TOTP(hmacKey, 10);

// keypad configuration
const byte rows = 4;
const byte cols = 3;
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[rows] = {2, 3, 4, 5};
byte colPins[cols] = {6, 7, 8};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

Servo doorServo;

swRTC rtc;
char* totpCode;
char inputCode[7];
int inputCode_idx;
boolean doorOpen;


void setup() {
  
  Serial.begin(9600);
  DEBUG_PRINTLN("TOTP Door lock");
  DEBUG_PRINTLN("");
  
  // attach servo object to the correct PIN
  doorServo.attach(SERVO_PIN);
  DEBUG_PRINTLN("Servo initialized");

  // init software RTC with the current time
  rtc.stopRTC();
  rtc.setDate(14, 9, 2013);
  rtc.setTime(9, 12, 00);
  rtc.startRTC();
  DEBUG_PRINTLN("RTC initialized and started");
  
  // reset input buffer index
  inputCode_idx = 0;
  
  // close the door
  doorServo.write(SERVO_CLOSED);
  doorOpen = false;
  DEBUG_PRINTLN("Door closed");  
}


void loop() {
  
  char key = keypad.getKey();

  // a key was pressed
  if (key != NO_KEY) {

    // # resets the input buffer    
    if(key == '#') {
      DEBUG_PRINTLN("# pressed, resetting the input buffer...");
      inputCode_idx = 0;      
    }
    
    // * closes the door
    else if(key == '*') {

      if(doorOpen == false) DEBUG_PRINTLN("* pressed but the door is already closed");

      else {

        DEBUG_PRINTLN("* pressed, closing the door...");
        for(int i = 0; i < SERVO_OPENED - SERVO_CLOSED; i++) {
          doorServo.write(SERVO_OPENED - i);
          delay(SERVO_DELAY);
        }
        doorOpen = false;
      }
    }
    else {      
      
      // save key value in input buffer
      inputCode[inputCode_idx++] = key;
      
      // if the buffer is full, add string terminator, reset the index
      // get the actual TOTP code and compare to the buffer's content
      if(inputCode_idx == 6) {
        
        inputCode[inputCode_idx] = '\0';
        inputCode_idx = 0;
        DEBUG_PRINT("New code inserted: ");
        DEBUG_PRINTLN(inputCode);
        
        long GMT = rtc.getTimestamp();
        totpCode = totp.getCode(GMT);
        
        // code is ok :)
        if(strcmp(inputCode, totpCode) == 0) {
          
          if(doorOpen == true) DEBUG_PRINTLN("Code ok but the door is already open");
          
          else {
            DEBUG_PRINTLN("Code ok, opening the door...");
            for(int i = 0; i < SERVO_OPENED - SERVO_CLOSED; i++) {
              doorServo.write(SERVO_CLOSED + i);
              delay(SERVO_DELAY);
            }
            doorOpen = true;
          }
          
        // code is wrong :(  
        } else {
          DEBUG_PRINT("Wrong code... the correct was: ");
          DEBUG_PRINTLN(totpCode);         
        }
      }      
    }
  }
}


