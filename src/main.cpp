#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <vector>
#include "config.h"

#define PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

#define NUM_LEDS 91
#define DATA_PIN 5 // D1 

#define WIFI_LED 20 // light on the led strip that will flicker to indicate WiFi status

CRGB leds[NUM_LEDS];
CRGB hiddenleds[NUM_LEDS];
CRGB bgcolor = CRGB(0, 0, 0);

bool isOn = false;

ESP8266WebServer http_rest_server(PORT);

void turnOff() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
  isOn = false;
}

void turnOffService() {
  turnOff();
  http_rest_server.send(201);
}

void showLEDs() {
  FastLED.show();
  isOn = true;
}

void turnOnService() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = hiddenleds[i];
  }
  showLEDs();
  http_rest_server.send(201);
}

void flush() {
  if (!isOn) return;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = hiddenleds[i];
  }
  showLEDs();
  http_rest_server.send(201);
}

void updateIntermediate() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if      (leds[i].r > hiddenleds[i].r) leds[i].r--;
    else if (leds[i].r < hiddenleds[i].r) leds[i].r++;

    if      (leds[i].g > hiddenleds[i].g) leds[i].g--;
    else if (leds[i].g < hiddenleds[i].g) leds[i].g++;

    if      (leds[i].b > hiddenleds[i].b) leds[i].b--;
    else if (leds[i].b < hiddenleds[i].b) leds[i].b++;
  }
  FastLED.show();
}

void gradient() {
  if (!isOn) return;
  for (int i = 0; i < 256; i++) {
    updateIntermediate();
  }
  http_rest_server.send(201);
}

void set_led() {
  // Serial.println("Setting LED...");
  hiddenleds[http_rest_server.arg("n").toInt()] = CRGB(
    http_rest_server.arg("r").toInt(), 
    http_rest_server.arg("g").toInt(), 
    http_rest_server.arg("b").toInt()
  );
  http_rest_server.send(201);
}

void clear_led() {
  hiddenleds[http_rest_server.arg("n").toInt()] = bgcolor;
  http_rest_server.send(201);
}

void set_bg_color() {
  // Serial.println("Setting bg color");
  // Serial.printf("bgcolor: %d %d %d\n", bgcolor.r, bgcolor.g, bgcolor.b);
  CRGB newbgcolor = CRGB(
    http_rest_server.arg("r").toInt(), 
    http_rest_server.arg("g").toInt(), 
    http_rest_server.arg("b").toInt()
  );
  for (int i = 0; i < NUM_LEDS; i++) {
    // Serial.printf("Old color: %d %d %d\n", hiddenleds[i].r, hiddenleds[i].g, hiddenleds[i].b);
    if (hiddenleds[i] = bgcolor) hiddenleds[i] = newbgcolor;
  }
  bgcolor = newbgcolor;
  // Serial.printf("New bg color: %d %d %d\n", newbgcolor.r, newbgcolor.g, newbgcolor.b);
  http_rest_server.send(201);
}

void set_range() {
  CRGB newcolor = CRGB(
    http_rest_server.arg("r").toInt(), 
    http_rest_server.arg("g").toInt(), 
    http_rest_server.arg("b").toInt()
  );
  int nStart = http_rest_server.arg("ns").toInt();
  int nEnd   = http_rest_server.arg("ne").toInt();
  for (int i = nStart; i <= nEnd; i++) {
    hiddenleds[i] = newcolor;
  }
  http_rest_server.send(201);
}

void clear_range() {
  // Serial.print("Clearing range ");
  int nStart = http_rest_server.arg("ns").toInt();
  int nEnd   = http_rest_server.arg("ne").toInt();
  // Serial.print(nStart);
  // Serial.println(nEnd);
  for (int i = nStart; i <= nEnd; i++) {
    hiddenleds[i] = bgcolor;
    // Serial.printf("Setting %d to bgcolor\n", i);
  }
  http_rest_server.send(201);
}


void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++) hiddenleds[i] = CRGB(0, 0, 0);

  int retries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi...");
  while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
    retries++;
    Serial.print('.');

    leds[WIFI_LED] = CRGB(255, 0, 0);
    FastLED.show();
    delay(WIFI_RETRY_DELAY / 2);

    leds[WIFI_LED] = CRGB(0, 0, 0);
    FastLED.show();
    delay(WIFI_RETRY_DELAY / 2);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 0, 0);
    }
    FastLED.show();
    while (true) {
      delay(1000000);
    }
  } else {
    ArduinoOTA.setHostname("bedlight");
    ArduinoOTA.onStart([]() { // switch off all the LEDs during upgrade
      turnOff();
    });
    ArduinoOTA.begin();
    Serial.println("Connected!");
    leds[WIFI_LED] = CRGB(0, 255, 0);
    delay(1000);
    leds[WIFI_LED] = CRGB(0, 0, 0);
  }

  http_rest_server.on("/", HTTP_GET, []() {
    http_rest_server.send(200, "text/html", "HTTP works!");
  });
  http_rest_server.on("/setLed", HTTP_GET, set_led);
  http_rest_server.on("/clearLed", HTTP_GET, clear_led);
  http_rest_server.on("/setBgColor", HTTP_GET, set_bg_color);
  http_rest_server.on("/turnOn", HTTP_GET, turnOnService);
  http_rest_server.on("/turnOff", HTTP_GET, turnOffService);
  http_rest_server.on("/setRange", HTTP_GET, set_range);
  http_rest_server.on("/clearRange", HTTP_GET, clear_range);
  http_rest_server.on("/flush", HTTP_GET, flush);
  http_rest_server.on("/gradient", HTTP_GET, gradient);
  http_rest_server.begin();
}

void loop() {
  http_rest_server.handleClient();
  ArduinoOTA.handle();
}