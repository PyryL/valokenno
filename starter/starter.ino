#include <Adafruit_NeoPixel.h>

#define PIEZO_PIN 16

Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);

void setup() {
  Serial.begin(115200);

  pixel.begin();
  pixel.setBrightness(255);

  ledcAttach(PIEZO_PIN, 250, 8);
}

void play_set_sound() {
  ledcWrite(PIEZO_PIN, 100);
  delay(100);
  ledcWrite(PIEZO_PIN, 0);
  delay(50);
  ledcWrite(PIEZO_PIN, 100);
  delay(100);
  ledcWrite(PIEZO_PIN, 0);
  delay(100);
}

void play_beep() {
  pixel.setPixelColor(0, 255, 255, 255);
  pixel.show();

  ledcWrite(PIEZO_PIN, 255);

  delay(200);

  ledcWrite(PIEZO_PIN, 0);

  pixel.setPixelColor(0, 0, 0, 0);
  pixel.show();
}

void loop() {
  play_set_sound();
  delay(1000);
  play_beep();
  delay(10000);
}
