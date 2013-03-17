/*
  POV display with embeded HTTP Server
  
  Mathieu Darse <hello@mathieudarse.fr>
*/

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>

// MAC Address of the device
static byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x50, 0x92 };
static IPAddress ip(192, 168, 1, 30); // Static IPv4 address

// Initialize the Webduino library (80 -> HTTP)
#define PREFIX ""
WebServer webserver(PREFIX, 80);

//// ROM-based messages (we only have 1k of RAM we need to be conservative)
P(Page_start) = "<!DOCTYPE html>\n<html><head><title>TrainsPOVing</title></head><body>\n";
P(Page_end) = "</body></html>";
P(Get_head) = "<h1>GET from ";
P(Post_head) = "<h1>POST to ";
P(Unknown_head) = "<h1>UNKNOWN request for ";
P(Default_head) = "unidentified URL requested.</h1><br>\n";
P(Raw_head) = "raw.html requested.</h1><br>\n";
P(Parsed_head) = "parsed.html requested.</h1><br>\n";
P(Good_tail_begin) = "URL tail = '";
P(Bad_tail_begin) = "INCOMPLETE URL tail = '";
P(Tail_end) = "'<br>\n";
P(Parsed_tail_begin) = "URL parameters:<br>\n";
P(Parsed_item_separator) = " = '";
P(Params_end) = "End of parameters<br>\n";
P(Post_params_begin) = "Parameters sent by POST:<br>\n";
P(Line_break) = "<br>\n";


//// Start of commands

void helloCommand(WebServer &server, WebServer::ConnectionType type, char * url_tail, bool tail_complete) {
  // HTTP OK
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

  server.printP(Default_head);
  server.printP(tail_complete ? Good_tail_begin : Bad_tail_begin);
  server.print(url_tail);
  server.printP(Tail_end);
  server.printP(Page_end);
}

//// End of commands


void setup() {
   // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&helloCommand);
  webserver.addCommand("index.html", &helloCommand); // alias for / (defaultCommand)
  webserver.begin();
  
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  char buffer[64];
  int length = 64;
  
  // process incoming connections (one at a time)
  webserver.processConnection(buffer, &length);
}

