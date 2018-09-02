#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include "secrets.h"
#include "config.h"

#ifdef __AVR__
  #include <avr/power.h>
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;

int brightness = 50;

void handleRoot() {
  server.send(200, "text/plain", "/led/single (index + either r, g, b OR w)\n/brightness (?value=0-255)");
}

void handleNotFound() {
  server.send(404, "text/plain", "not found");
}

void handleSingleLed() {
  String ledIndexS = server.arg("index");
  String rS = server.arg("r");
  String gS = server.arg("g");
  String bS = server.arg("b");
  String wS = server.arg("w");

  if (ledIndexS.length() == 0) {
    server.send(400, "text/plain", "index must be specified");
    return;
  }

  if ((rS.length() == 0 && gS.length() == 0 && bS.length() == 0) && wS.length() == 0) {
    server.send(400, "text/plain", "color or whiteness must be specified");
    return;
  }

  int ledIndex = ledIndexS.toInt();

  if (wS.length() > 0) {
    int w = constrain(wS.toInt(), 0, 255);
    server.send(200, "text/plain", "setting white " + String(w));
  } else {
    int r = rS.length() == 0 ? 0 : constrain(rS.toInt(), 0, 255);
    int g = gS.length() == 0 ? 0 : constrain(gS.toInt(), 0, 255);
    int b = bS.length() == 0 ? 0 : constrain(bS.toInt(), 0, 255);
    server.send(200, "text/plain", "setting rgb " + String(r) + ", " + String(g) + ", " + String(b));
  }
}

void handleBrightness() {
  String bS = server.arg("value");
  if (bS.length() == 0) {
    server.send(400, "text/plain", "value must be specified");
    return;
  }
  brightness = constrain(bS.toInt(), 0, 255);
  server.send(200, "text/plain", String(brightness));
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/led/single", handleSingleLed);
  // server.on("/led/range", handleRangeLed);
  server.on("/brightness", handleBrightness);

  server.onNotFound(handleNotFound);

  server.begin();
  digitalWrite(LED_BUILTIN, 1);
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}
