#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <vector>


#define PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

#define NUM_LEDS 150
#define DATA_PIN 5 // D1 

#define WIFI_LED 5 // light on the led strip

const char * WIFI_SSID = "Linden";
const char * WIFI_PASS = "PASSWORD";

CRGB leds[NUM_LEDS];
CRGB hiddenleds[NUM_LEDS];
CRGB bgcolor;

bool isOn = false;


ESP8266WebServer http_rest_server(PORT);

// void log(const char * string) {
//   Serial.println(string);
// }

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
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = hiddenleds[i];
  }
  FastLED.show();
  isOn = true;
}

void turnOnService() {
  showLEDs();
  http_rest_server.send(201);
}

void updateLeds() {
  if (isOn) showLEDs();
}


void set_led() {
  Serial.println("Setting LED...");
  hiddenleds[http_rest_server.arg("n").toInt()] = CRGB(
    http_rest_server.arg("r").toInt(), 
    http_rest_server.arg("g").toInt(), 
    http_rest_server.arg("b").toInt()
  );
  updateLeds();
  http_rest_server.send(201);
}

void clear_led() {
  hiddenleds[http_rest_server.arg("n").toInt()] = bgcolor;
  updateLeds();
  http_rest_server.send(201);
}

void set_bg_color() {
  CRGB newbgcolor = CRGB(
    http_rest_server.arg("r").toInt(), 
    http_rest_server.arg("g").toInt(), 
    http_rest_server.arg("b").toInt()
  );
  for (int i = 0; i < 150; i++) {
    if (hiddenleds[i] = bgcolor) hiddenleds[i] = newbgcolor;
  }
  bgcolor = newbgcolor;
  updateLeds();
  http_rest_server.send(201);
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  int retries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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
  http_rest_server.on("/setLed", HTTP_POST, set_led);
  http_rest_server.on("/clearLed", HTTP_POST, clear_led);
  http_rest_server.on("/setBgColor", HTTP_POST, set_bg_color);
  http_rest_server.on("/turnOn", HTTP_POST, turnOnService);
  http_rest_server.on("/turnOff", HTTP_POST, turnOffService);
  http_rest_server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  http_rest_server.handleClient();
  ArduinoOTA.handle();
}