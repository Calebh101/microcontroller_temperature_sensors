/*#include <Arduino.h>

#define D2 4
#define D4 2
#define D5 14
#define D6 12
#define D7 13

#define DSP1 D6
#define DSP2 D7

#define SER   D5
#define RCLK  D2
#define SRCLK D4

#define DSP_EIGHT 0b01111111
#define DSP_NINE  0b01111011
#define DSP_SEVEN 0b01011000

void shiftData(byte data) {
  digitalWrite(RCLK, LOW);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(SRCLK, LOW);
    digitalWrite(SER, (data >> i) & 1);
    digitalWrite(SRCLK, HIGH);
  }

  digitalWrite(RCLK, HIGH);
}

void showDigit(byte data, int digitPin) {
  digitalWrite(DSP1, HIGH);
  digitalWrite(DSP2, HIGH);

  shiftData(data);

  digitalWrite(digitPin, LOW);
  delay(5);

  digitalWrite(DSP1, HIGH);
  digitalWrite(DSP2, HIGH);
}

void setup() {
  pinMode(DSP1, OUTPUT);
  pinMode(DSP2, OUTPUT);

  pinMode(SER, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);

  digitalWrite(DSP1, HIGH);
  digitalWrite(DSP2, HIGH);
}

void loop() {
  showDigit(DSP_EIGHT, DSP1);
  showDigit(DSP_EIGHT, DSP2);
}*/