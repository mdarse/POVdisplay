/*
 POV display
*/

#include "ASCII.h"

const int charHeight = 5;
const int charWidth = 5;

// Define pin setup
const int coilPin = 2; // Coil driver
const int ratePin = A0;
const int ledPins[] = {3,4,5,6,7,8}; // LEDs
const int ledPinsSize = 6;

// Init coil vars
int coilState = LOW;
long coilLastUpdate = 0;
long coilTickInterval = 100000;
// Init display vars
char    displayString[8]     = "";
int     displayStringLength    = 0;
int     displayCharacterIndex  = 0;
int     displayCharacterColumn = 0;
boolean displayNeedsSpace      = false;
long    displayLastRefresh     = 0;
long    displayRefreshInterval = 100000;

int displayWidth       = 30; // Display horizontal resolution

// Init text vars
String inputString = "";        // a string to hold incoming data
boolean stringComplete = false; // whether the string is complete

void setup() {
  pinMode(coilPin, OUTPUT);
  // Setup each LED pin
  for (int pin = 0; pin < ledPinsSize; pin++) {
    pinMode(ledPins[pin], OUTPUT);
  }
  // Init data input
  Serial.begin(9600);
  inputString.reserve(sizeof(displayString));
}

void loop() {
  // print the string when a newline arrives) 
  if (stringComplete) {
    displayCharacterIndex  = 0;
    displayCharacterColumn = 0;
    displayNeedsSpace      = false;
    inputString.toCharArray(displayString, sizeof(displayString));
    displayStringLength = inputString.length();
    // width = letter number x letter width + a space between each letter
    displayStringWidth = displayStringLength * (charWidth + 1) - 1;
    Serial.println("Received \"" + inputString + "\"");
    Serial.println(inputString.length());
    // clear the string
    inputString = "";
    stringComplete = false;
  }
  
  unsigned long now = micros();
  
  // Coil actuator
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
    
    // Rate
    int sensorValue = analogRead(ratePin);
    coilTickInterval = map(sensorValue, 0, 1023, 100000, 1000000);
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

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == ' ') { // if the incoming character is a newline
      stringComplete = true;
    }
    else {
      inputString += inChar;
    }
  }
}
