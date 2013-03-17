/*
  POV display with embeded HTTP Server
  
  Mathieu Darse <hello@mathieudarse.fr>
*/

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include "ASCII.h"

// MAC Address of the device
static byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x50, 0x92 };
static IPAddress ip(192, 168, 1, 30); // Static IPv4 address

// Initialize the Webduino library (80 -> HTTP)
#define PREFIX ""
WebServer webserver(PREFIX, 80);


//////////////////////
// Display settings //
//////////////////////
#define SIZE(x)  (sizeof(x) / sizeof(x[0]))
const int charWidth = 5;
// Define pin setup
const int coilPin = 2; // Coil driver
const int ratePin = A0;
const int ledPins[] = {3,4,5,6,7,8}; // LEDs
// Init coil vars
int coilState = LOW;
long coilLastUpdate = 0;
long coilTickInterval = 100000;
// Init display vars
char    displayString[32]; // 31 char + NULL byte
int     displayStringLength    = 0;
int     displayCharacterIndex  = 0;
int     displayCharacterColumn = 0;
boolean displayNeedsSpace      = false;
boolean displayReverse         = false;
long    displayLastRefresh     = 0;
long    displayRefreshInterval = 100000;
// Geometry
int displayWidth       = 30; // Display horizontal resolution
int displayStringWidth = 0;



////////////////////////
// ROM-based messages (we only have 1k of RAM we need to be conservative)
////////////////////////

P(Page_start) = "<!DOCTYPE html>\n<html><head><title>TrainsPOVing</title></head><body>\n";
P(Page_end) = "</body></html>";
P(Hello_world) = "<h1>Hello POV !</h1>\n<img src=\"http://i.imgur.com/f6V29.gif\">\n<br>\n<a href=\"display\">manage display</a>";
P(Display_form) = "<form method=\"POST\">\n<label>Display text <input name=\"text\"></label>\n<input type=\"submit\">\n</form>";

P(Get_head) = "<h1>GET from ";
P(Post_head) = "<h1>POST to ";
P(Unknown_head) = "<h1>UNKNOWN request for ";
P(Parsed_head) = "diag.html requested.</h1><br>\n";
P(Good_tail_begin) = "URL tail = '";
P(Bad_tail_begin) = "INCOMPLETE URL tail = '";
P(Tail_end) = "'<br>\n";
P(Parsed_tail_begin) = "URL parameters:<br>\n";
P(Parsed_item_separator) = " = '";
P(Params_end) = "End of parameters<br>\n";
P(Post_params_begin) = "Parameters sent by POST:<br>\n";



//////////////
// Commands //
//////////////

void helloCommand(WebServer &server, WebServer::ConnectionType type, char * url_tail, bool tail_complete) {
  server.httpSuccess(); // HTTP 200
  if (type == WebServer::HEAD) return;
  // Print a Hello World
  server.printP(Page_start);
  server.printP(Hello_world);
  server.printP(Page_end);
}

#define NAMELEN 32
#define VALUELEN 32

void parseCommand(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];

  server.httpSuccess();

  if (type == WebServer::HEAD) return;

  server.printP(Page_start);
  switch (type) {
    case WebServer::GET:
      server.printP(Get_head);
      break;
    case WebServer::POST:
      server.printP(Post_head);
      break;
    default:
      server.printP(Unknown_head);
  }

  server.printP(Parsed_head);
  server.printP(tail_complete ? Good_tail_begin : Bad_tail_begin);
  server.print(url_tail);
  server.printP(Tail_end);

  if (strlen(url_tail)) {
    server.printP(Parsed_tail_begin);
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_EOS)
        server.printP(Params_end);
      else {
        server.print(name);
        server.printP(Parsed_item_separator);
        server.print(value);
        server.printP(Tail_end);
      }
    }
  }
  
  if (type == WebServer::POST) {
    server.printP(Post_params_begin);
    while (server.readPOSTparam(name, NAMELEN, value, VALUELEN)) {
      server.print(name);
      server.printP(Parsed_item_separator);
      server.print(value);
      server.printP(Tail_end);
    }
  }
  server.printP(Page_end);
}

void displayCommand(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (type == WebServer::POST) {
    bool repeat;
    char name[NAMELEN], value[VALUELEN];
    
    int i = 0;
    
    // handle each POST parameter
    do {
      // returns false when there are no more parameters to read
      repeat = server.readPOSTparam(name, NAMELEN, value, VALUELEN);
      
      if (strcmp(name, "text") == 0) {
        setText(value, strlen(value));
      }
    }
    while (repeat);
    
    // tell the web browser to reload
    // the page using a GET method
    server.httpSeeOther(PREFIX);
    return;
  }
  
  // for non-POST request, send HTTP OK
  server.httpSuccess();
  // stop here for HEAD requests
  if (type == WebServer::HEAD) return;
  
  server.printP(Page_start);
  server.printP(Display_form);
  server.printP(Page_end);
}



///////////////////
// Arduino stuff //
///////////////////

void setup() {
  // Pin setup
  pinMode(coilPin, OUTPUT);
  for (int pin = 0; pin < SIZE(ledPins); pin++) {
    pinMode(ledPins[pin], OUTPUT);
  }
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&helloCommand);
  webserver.addCommand("display", &displayCommand); // handle text setting
  webserver.addCommand("diag.html", &parseCommand);
  webserver.begin();
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  char buffer[64];
  int length = 64;
  
  // process incoming connections (one at a time)
  webserver.processConnection(buffer, &length);
  
  unsigned long now = micros();
  
  // Coil actuator
  if (now - coilLastUpdate > coilTickInterval) {
    coilLastUpdate = now; // update last tick
    // It's time to move the coil
    if (coilState == LOW) {
      coilState = HIGH;
      displayCharacterIndex  = 0;
      displayCharacterColumn = 0;
      displayNeedsSpace      = false;
      displayReverse         = false;
    }
    else {
      coilState = LOW;
      displayReverse = true;
    }
    digitalWrite(coilPin, coilState);
    
    // Rate
    int sensorValue = analogRead(ratePin);
    coilTickInterval = map(sensorValue, 0, 1023, 10000, 1000000);
    displayRefreshInterval = coilTickInterval / displayStringWidth;
  }

  // Update LEDs
  if (now - displayLastRefresh > displayRefreshInterval) {
    displayLastRefresh = now;
    if (displayNeedsSpace) {
       // Serial.println("-----");
      printColumn(B00000);
      displayNeedsSpace = false;
    }
    else if (displayReverse)
      printColumn(B00000);
    else
      printNextColumn();
  }
}

void setText(char * text, unsigned int size) {
  if (!text || !size) return;
  strncpy(displayString, text, size);
  displayStringLength = size;
  // width = letter number x letter width + a space between each letter
  displayStringWidth = displayStringLength * (charWidth + 1) - 1;
  
  Serial.print("set text \"");
  Serial.print(displayString);
  Serial.println("\"");
}

void printNextColumn() {
  // Show next column
  char character = displayString[displayCharacterIndex];
  if (character == '\0') { // If end of string
    displayCharacterIndex = 0;
    displayCharacterColumn = 0;
    character = displayString[0];
  }
  byte column = ASCII[character - 0x20][displayCharacterColumn];
  // Serial.print(displayCharacterIndex);
  // Serial.print(':');
  // Serial.print(displayCharacterColumn);
  // Serial.print('=');
  // Serial.println(column, BIN);
  printColumn(column);
  
  // Move markers to next column
  if (displayCharacterColumn == charWidth - 1) { // If end of a char
    displayCharacterIndex++;
    displayCharacterColumn = 0;
    displayNeedsSpace = true; // Mark next loop for inserting a thin space
  }
  else {
    displayCharacterColumn++;
  }
}

void printColumn(byte column) {
  for (int y=0; y < SIZE(ledPins); y++) {
    // Get current pixel
    boolean pixel = column & (1 << y);
    digitalWrite(ledPins[y], pixel);
  }
}

