#include <Arduino.h>
#include "secrets.h"

int reedPin = 4;
int ledPin = 4;

// int reedPin = 13;  // GPIO 13 (D7 on ESP-12F)
int reedState = 0;

void setup() {
  pinMode(reedPin, INPUT_PULLUP);  // Enable internal pull-up resistor
  Serial.begin(9600);  // Start serial communication at 9600 baud rate
}

void loop() {
  reedState = digitalRead(reedPin);  // Read the state of the reed switch
  Serial.println(reedState);
  if (reedState == HIGH) {
    Serial.println("Reed switch is CLOSED (magnet detected)");
  } else {
    Serial.println("Reed switch is OPEN");
  }
  delay(1000);  // Delay for 1 second
}


// int reedState;
// bool changeState = false;

// bool state;
// String doorState;

// unsigned long previousMillis = 0; 
// const long interval = 1500;

// ICACHE_RAM_ATTR void changeSensorStatus() {
//   changeState = true;
// }

// void setup() {
//   pinMode(ledPin, OUTPUT);
//   pinMode(reedPin, INPUT_PULLUP);
//   // state = digitalRead(reedPin);
//   // digitalWrite(ledPin, state);
//   Serial.begin(9600);
//   // attachInterrupt(digitalPinToInterrupt(reedPin), changeSensorStatus, CHANGE);

// }

// void loop() {
//   // Serial.println("Status: " + changeState);
//   // if (changeState){
//   //   unsigned long currentMillis = millis();
//   //   if(currentMillis - previousMillis >= interval) {
//   //     previousMillis = currentMillis;
//   //     // If a state has occured, invert the current door state   
//   //     state = !state;
//   //     if(state) {
//   //       doorState = "closed";
//   //     }
//   //     else{
//   //       doorState = "open";
//   //     }
//   //     digitalWrite(ledPin, state);
//   //     changeState = false;
//   //     Serial.println(state);
//   //     Serial.println(doorState);
        
//   //   }
//   // }
//     reedState = digitalRead(reedPin);
//   Serial.println("reedState: " + reedState);
//   if (reedState == HIGH) {
//     digitalWrite(ledPin, HIGH);
//     Serial.println("Should be ON");
//   } else {
//     digitalWrite(ledPin, LOW);
//     Serial.println("Should be off");
//   }
//   delay(1000);
// }

// void setup() {
//   Serial.begin(9600);  // Ensure this matches the Serial Monitor's baud rate
// }

// void loop() {
//   Serial.println("Testing Serial Output");
//   delay(1000);  // Delay for 1 second
// }
