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

#define NEOPIXEL_PIN 2

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);

int brightness = 50;

byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

boolean isValidColor() {
  if ((server.arg("r").length() == 0 &&
       server.arg("g").length() == 0 &&
       server.arg("b").length() == 0) &&
       server.arg("w").length() == 0) {
    return false;
  }
  return true;
}

uint32_t getColor() {
  String rS = server.arg("r");
  String gS = server.arg("g");
  String bS = server.arg("b");
  String wS = server.arg("w");

  if (wS.length() > 0) {
    int w = constrain(wS.toInt(), 0, 255);
    return strip.Color(0, 0, 0, w);
  } else {
    int r = rS.length() == 0 ? 0 : constrain(rS.toInt(), 0, 255);
    int g = gS.length() == 0 ? 0 : constrain(gS.toInt(), 0, 255);
    int b = bS.length() == 0 ? 0 : constrain(bS.toInt(), 0, 255);
    return strip.Color(r, g, b);
  }
}

void handleRoot() {
  server.send(200, "text/plain", "/led/single (index + either r, g, b OR w)\n/led/range (start, stop + either r, g, b OR w)\n/brightness (?value=0-255)");
}

void handleNotFound() {
  server.send(404, "text/plain", "not found");
}

void handleSingleLed() {
  if (!isValidColor()) {
    server.send(400, "text/plain", "color must be specified (w (0-255) OR r, g, b (0-255)");
    return;
  }
  uint32_t color = getColor();

  String indexS = server.arg("index");
  if (indexS.length() == 0) {
    server.send(400, "text/plain", "index must be specified");
    return;
  }

  int index = constrain(indexS.toInt(), 0, NUM_LEDS-1);
  strip.setPixelColor(index, color);
  strip.show();
  server.send(200, "text/plain", "setting color " + String(color) + " for led at index " + String(index));
}

void handleRangeLed() {
  if (!isValidColor()) {
    server.send(400, "text/plain", "color must be specified (w (0-255) OR r, g, b (0-255)");
    return;
  }
  uint32_t color = getColor();

  String startIndexS = server.arg("start");
  String stopIndexS = server.arg("stop");

  if (startIndexS.length() == 0 || stopIndexS.length() == 0) {
    server.send(400, "text/plain", "range (start, stop) must be specified");
    return;
  }

  int startIndex = constrain(startIndexS.toInt(), 0, NUM_LEDS-1);
  int stopIndex = constrain(stopIndexS.toInt(), 0, NUM_LEDS-1);

  if (stopIndex <= startIndex) {
    server.send(400, "text/plain", "stop must be greater than start");
    return;
  }

  for(uint16_t i=startIndex; i<=stopIndex; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  server.send(400, "text/plain", "start index: " + String(startIndex) + ", stop index: " + String(stopIndex));
}

void handleBrightness() {
  String bS = server.arg("value");
  if (bS.length() == 0) {
    server.send(400, "text/plain", "value must be specified");
    return;
  }
  brightness = constrain(bS.toInt(), 0, 255);
  strip.setBrightness(brightness);
  strip.show();
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

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/led/single", handleSingleLed);
  server.on("/led/range", handleRangeLed);
  server.on("/brightness", handleBrightness);

  server.onNotFound(handleNotFound);

  server.begin();
  digitalWrite(LED_BUILTIN, 1);
  Serial.println("HTTP server started");

  strip.setBrightness(brightness);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop(void) {
  server.handleClient();
}
