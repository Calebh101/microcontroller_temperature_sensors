#include <DHT.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include "env.h"

#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15

#define DHTIN D1

#define DSP1 D6
#define DSP2 D7

#define SER D5
#define RCLK D2
#define SRCLK D8

#define DSP_ZERO   0b01111110
#define DSP_ONE    0b00011000
#define DSP_TWO    0b01010111
#define DSP_THREE  0b01011011
#define DSP_FOUR   0b00111001
#define DSP_FIVE   0b01101011
#define DSP_SIX    0b01101111
#define DSP_SEVEN  0b01011000
#define DSP_EIGHT  0b01111111
#define DSP_NINE   0b01111011

#define DSP_MINUS  0b00000001
#define DSP_DP     0b10000000

#define DSP_E      0b01100111
#define DSP_OFF    0b00000000

#define TICK_US 500
#define TICKS_PER_DIGIT 20

#define SCREEN_DIM_FACTOR_MAX 3
#define DHT_OFFSET 0
#define TOGGLE_BUTTON digitalRead(0) == 0

DHT dht(DHTIN, DHT22);
ESP8266WebServer server(80);

float temp = NAN;
float hum = NAN;

unsigned long lastRead = 0;
unsigned long bootTime = 0;
unsigned long disconnects = 0;

volatile byte digitA = DSP_OFF, digitB = DSP_OFF;
volatile bool showingA = true;
volatile int tickCount = 0;
volatile uint8_t screenDimFactor = 1;

void IRAM_ATTR dispISR() {
  int onTicks;

  switch (screenDimFactor) {
    case 1: onTicks = TICKS_PER_DIGIT / 2; break;
    case 2: onTicks = TICKS_PER_DIGIT / 4; break;
    case 3: onTicks = 1; break;
    default: onTicks = TICKS_PER_DIGIT; break;
  }

  if (tickCount == 0) {
    digitalWrite(DSP1, HIGH);
    digitalWrite(DSP2, HIGH);

    if (showingA) {
      shiftData(digitA);
      digitalWrite(DSP1, LOW);
    } else {
      shiftData(digitB);
      digitalWrite(DSP2, LOW);
    }
  }

  if (tickCount == onTicks) {
    digitalWrite(DSP1, HIGH);
    digitalWrite(DSP2, HIGH);
  }

  tickCount++;

  if (tickCount >= TICKS_PER_DIGIT) {
    tickCount = 0;
    showingA = !showingA;
  }

  timer1_write(2500);
}

byte numberToDisplay(int n, bool dp) {
  byte number;

  if (n == 0) number = DSP_ZERO;
  else if (n == 1) number = DSP_ONE;
  else if (n == 2) number = DSP_TWO;
  else if (n == 3) number = DSP_THREE;
  else if (n == 4) number = DSP_FOUR;
  else if (n == 5) number = DSP_FIVE;
  else if (n == 6) number = DSP_SIX;
  else if (n == 7) number = DSP_SEVEN;
  else if (n == 8) number = DSP_EIGHT;
  else if (n == 9) number = DSP_NINE;
  else number = DSP_E;

  if (dp) number = number | DSP_DP;
  return number;
}

void handleGetRequest() {
  String json = "{\"t\":\"" + String(temp) + "\",\"h\":\"" + String(hum) + "\"}";
  server.send(200, "application/json", json);
}

void setupWifi() {
  Serial.println("Setting up WiFi...");
  digitalWrite(16, LOW);

  #ifdef WIFI_ENABLED
    #if WIFI_ENABLED
      unsigned long startTime = millis();
      const unsigned long timeout = 15000; // ms

      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);

      Serial.print("Connecting to ");
      Serial.print(WIFI_SSID);
      Serial.print("... ");

      while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
        delay(500);
        Serial.print(WiFi.status());
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        server.on("/", HTTP_GET, handleGetRequest);
        server.begin();
        Serial.println("Server started");
      } else {
        Serial.println();
        Serial.println("Unable to connect to WiFi");
      }
    #endif
  #endif

  digitalWrite(16, HIGH);
}

void shiftData(byte data) {
  digitalWrite(RCLK, LOW);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(SRCLK, LOW);
    digitalWrite(SER, (data >> i) & 1);
    digitalWrite(SRCLK, HIGH);
  }

  digitalWrite(RCLK, HIGH);
}

void display(byte a, byte b) {
  digitA = a;
  digitB = b;
}

bool displayNumber(int n) {
  if (n < 0 && n > -10) {
    display(DSP_MINUS, numberToDisplay(-n, false));
  } else if (n >= 0 && n < 10) {
    display(DSP_OFF, numberToDisplay(n, false));
  } else if (n < 100) {
    int tens = n / 10;
    int ones = n % 10;

    display(numberToDisplay(tens, false), numberToDisplay(ones, false));
  } else {
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.print("Reset reason: ");
  Serial.println(ESP.getResetReason());

  pinMode(DHTIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(DSP1, OUTPUT);
  pinMode(DSP2, OUTPUT);

  pinMode(SER, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);

  setupWifi();
  dht.begin();

  timer1_attachInterrupt(dispISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(2500);

  bootTime = millis();
}

void loop() {
  #if WIFI_ENABLED
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      disconnects++;
      digitalWrite(LED_BUILTIN, LOW);

      Serial.print("Disconnected from WiFi: ");
      Serial.println(disconnects);

      displayNumber(disconnects);
      setupWifi();
    }
  #endif

  if (millis() - lastRead >= 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t)) temp = t + DHT_OFFSET;
    if (!isnan(h)) hum = h;

    Serial.print("DHTIN(" + String(DHTIN) + "): ");
    Serial.print(digitalRead(DHTIN));
    Serial.print(" (t="); Serial.print(temp);
    Serial.print(" f="); Serial.print((temp * 9.0 / 5.0) + 32 + 0.5);
    Serial.print(" h="); Serial.print(hum);
    Serial.println(")");

    lastRead = millis();
  }

  if (TOGGLE_BUTTON) {
    digitalWrite(LED_BUILTIN, LOW);
    while (TOGGLE_BUTTON);
    digitalWrite(LED_BUILTIN, HIGH);

    screenDimFactor++;
    if (screenDimFactor > SCREEN_DIM_FACTOR_MAX) screenDimFactor = 0;

    Serial.print("Toggle activated: ");
    Serial.println(screenDimFactor);
  }

  if (screenDimFactor > 0) {
    if (isnan(temp) || isnan(hum)) {
      display(isnan(temp) ? DSP_E : DSP_OFF, isnan(hum) ? DSP_E : DSP_OFF);
    } else {
      int f = (temp * 9.0 / 5.0) + 32 + 0.5;
      displayNumber(f);
    }
  } else {
    display(DSP_OFF, DSP_OFF);
  }

  yield();
  server.handleClient();
}