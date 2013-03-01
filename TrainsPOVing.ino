/*
 POV display
*/

#include "ASCII.h"

const int charHeight = 5;
const int charWidth = 5;

// Define pin setup
const int coilPin = 2; // Coil driver
const int coilIntervalPin = A0;
const int ledPins[] = {3,4,5,6,7,8}; // LEDs
const int ledPinsSize = 6;

// Init coil vars
int coilState = LOW;
long coilLastUpdate = 0;
long coilTickInterval = 100;
// Init text vars
char displayString[] = "10:12";
int displayCharacterIndex = 0;
int displayCharacterColumn = 0;
int displayNeedsSpace = false;
long displayLastRefresh = 0;
long displayRefreshInterval = 2000;

void setup() {
  pinMode(coilPin, OUTPUT);
  // Setup each LED pin
  for (int pin = 0; pin < ledPinsSize; pin++) {
    pinMode(ledPins[pin], OUTPUT);
  }
  Serial.begin(9600);
}

void loop() {
  // Coil interval
  coilTickInterval = analogRead(coilIntervalPin);
 // coilTickInterval = 235;
  Serial.print(coilTickInterval);
  Serial.print('\n');
  // Coil actuator
  unsigned long now = millis();
  if (now - coilLastUpdate > coilTickInterval) {
    coilLastUpdate = now; // update last tick
    // It's time to move the coil
    if (coilState == LOW) {
      coilState = HIGH;
    }
    else {
      coilState = LOW;
      clearDisplay();
    }
    digitalWrite(coilPin, coilState);
  }
  // Update LEDs
  if (now - displayLastRefresh > coilTickInterval && coilState == HIGH) {
  // if (now - displayLastRefresh > displayRefreshInterval) {
    displayLastRefresh = now;
    if (displayNeedsSpace) {
      clearDisplay();
      displayNeedsSpace = false;
    }
    else
      printNextColumn();
  }
}

void printNextColumn() {
  // Show next column
  char character = displayString[displayCharacterIndex];
  byte column = ASCII[character - 0x20][displayCharacterColumn];
  printColumn(column, charHeight);
  
  // Move markers to next column
  if (displayCharacterColumn < charWidth - 1) // We are in the middle of a char
    displayCharacterColumn++;
  else { // We jump to next character
    if (displayString[displayCharacterIndex + 1] == '\0') // If end of string
      displayCharacterIndex = 0;
    else
      displayCharacterIndex++;
    displayCharacterColumn = 0;
    displayNeedsSpace = true; // Mark next loop for inserting a thin space
  }

  Serial.print('\n');
}

void clearDisplay() {
  printColumn(B00000, charHeight);
}

void printColumn(byte column, int size) {
  for (int y=0; y<size; y++) {
    // Get current pixel
    bool pixel = column & (1 << y);
    //Serial.print(pixel);
    digitalWrite(ledPins[y], pixel);
  }
}
