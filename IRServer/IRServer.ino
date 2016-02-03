/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * An IR LED must be connected to ESP8266 pin 0.
 * Version 0.1 June, 2015
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <limits.h>
 
const char* ssid = "***";
const char* password = "***";
MDNSResponder mdns;

ESP8266WebServer server(80);

IRsend irsend(5);

void handleRoot() {
 server.send(200, "text/html", 
  "<html><head>"
  "<title>ESP8266 Demo</title>"
  "</head>"
  "<body>"
    "<h1>Hello from ESP8266, you can send IR signals from here!</h1>"
    "<form action=\"/ir\">"
      "<select name=\"protocol\">"
        "<option value=\"NEC\">NEC</option>"
        "<option value=\"Dyson\">Dyson</option>"
      "</select>"
      "<input type=\"text\" name=\"code\" />"
      "<input type=\"submit\" />"
    "</form>"
  "</body></html>");
}

void handleIr(){
  int repeats = 0;
  unsigned long code = ULONG_MAX;
  unsigned int protocol = NEC;
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "code") 
    {
      //unsigned long code = server.arg(i).toInt();
      code = strtoul(server.arg(i).c_str(), NULL, 0);
      Serial.print("request "); Serial.println(code, HEX);
    }
    else if(server.argName(i) == "repeat")
    {
      repeats = server.arg(i).toInt();
    } 
    else if (server.argName(i).substring(0,5) == "proto") {
      for (int count = 0; count < NUM_DECODE_TYPES; ++count) {
        if (decode_names[count] == server.arg(i)) {
          protocol = count;
          break;
        }
      }
    }
  }
  if (code != ULONG_MAX && protocol > 0) {
    switch(protocol) {
      case NEC: sendNECProtocol(code, repeats); break;
      case DYSON: sendDysonProtocol(code); break;
      default: break;
    }
  }
  handleRoot();
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
 
void setup(void){
  irsend.begin();
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("irserver1", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  server.on("/ir", handleIr); 
 
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void){
  server.handleClient();
} 

void sendNECProtocol(uint32_t code, int repeats)
{
    uint32_t protoCode = 0;
    //for each byte, reverse the bits, then send the byte + the inverse
    uint8_t topByte = reverseBits(code >> 8UL);
    protoCode = (uint32_t)topByte << 24UL;
    protoCode |= (((uint32_t)~topByte & 0xff) << 16UL);
    uint8_t bottomByte = reverseBits(code & 0xff);
    protoCode |= ((uint32_t)bottomByte << 8UL);
    protoCode |= ((uint32_t)~bottomByte & 0xff);
    Serial.print("result code ");
    Serial.println(protoCode,HEX);
    irsend.sendNEC(protoCode, 32, repeats);
}

void sendDysonProtocol(uint16_t code)
{
  static uint8_t count = 0;
  code = code << 2 + count;
  irsend.sendDyson(code, 15);
  count = (count + 1) & 3;
}

uint8_t reverseBits(uint16_t byteval) 
{
    uint8_t result = 0;
    for (int bitnum = 0; bitnum < 8; ++bitnum) {
        result = (result << 1UL) | (byteval & 1UL);
        byteval >>= 1UL;
    }
    return result;
}
