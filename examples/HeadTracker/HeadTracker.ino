/*
 * This example turns the ESP32 into a Bluetooth LE controller with 3 axes defined as dials
 * 
 * bleHeadTracker.setAxes takes the following signed char parameters: 
 * (Dial_1, Dial_2 , Dial_3);
 */
 
#include <BleHeadTracker.h> 

BleHeadTracker bleHeadTracker;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleHeadTracker.begin();
}

void loop() {
  if(bleHeadTracker.isConnected()) {
    Serial.println("Move all dials to max.");
    bleHeadTracker.setAxes(127, 127, 127);
    delay(500);

    Serial.println("Move all dials to min.");
    bleHeadTracker.setAxes(-127, -127, -127);
    delay(500);
  }
}