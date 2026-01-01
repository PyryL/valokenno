// this entire file is wrapped in this conditional
#ifdef MASTER_TYPE_STARTER

#define PIEZO_PIN 16

unsigned long starter_set_sound_time = 0;
unsigned long starter_beep_sound_time = 0;

void play_set_sound() {
  ledcWrite(PIEZO_PIN, 100);
  delay(180);
  ledcWrite(PIEZO_PIN, 0);
  delay(50);
  ledcWrite(PIEZO_PIN, 100);
  delay(180);
  ledcWrite(PIEZO_PIN, 0);
}

void play_beep() {
  pixel.setPixelColor(0, 255, 255, 255);
  pixel.show();

  unsigned long timestamp = millis();

  ledcWrite(PIEZO_PIN, 255);

  delay(200);

  ledcWrite(PIEZO_PIN, 0);

  pixel.setPixelColor(0, 0, 0, 0);
  pixel.show();

  motion_timestamps.push_back(timestamp);
}

unsigned long randomize_set_beep_delay() {
  unsigned long rand_val = (unsigned long)esp_random();

  unsigned long min = 900;
  unsigned long max = 1200;

  return (rand_val % (max - min + 1)) + min;
}

void setup_starter() {
  ledcAttach(PIEZO_PIN, 250, 8);
}

void loop_starter() {
  // TODO: must check that starter_set_sound_time and starter_beep_sound_time are zero, that is, there is no ongoing starter sequence
  if (has_new_starter_request) {
    starter_set_sound_time = millis() + 10000;
    has_new_starter_request = false;
  }

  if (starter_set_sound_time > 0 && millis() >= starter_set_sound_time) {
    play_set_sound();
    starter_set_sound_time = 0;
    starter_beep_sound_time = millis() + randomize_set_beep_delay();
  }

  if (starter_beep_sound_time > 0 && millis() >= starter_beep_sound_time) {
    play_beep();
    starter_beep_sound_time = 0;
  }
}

#endif
