#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <Adafruit_NeoPixel.h>

// Exactly one of these should be defined
#define MASTER_TYPE_STARTER
// #define MASTER_TYPE_SENSOR


#define MAX_SLAVE_COUNT 4

// MASTER

Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);

long slave_clock_offsets[MAX_SLAVE_COUNT];

std::vector<unsigned long> motion_timestamps = {};

uint8_t slaveMacs[MAX_SLAVE_COUNT][6] = {
  {0xDC, 0x54, 0x75, 0xC1, 0xDD, 0xA4}, // slave 1
  {0xB4, 0x3A, 0x45, 0x34, 0x0E, 0xDC}, // slave 2 (new)
};
int slave_count = 2;


bool send_ping_pong(int slave_index) {
  Serial.println("Sending ping-pong");

  uint8_t message_type[3] = {'p', 'i', 'n'};
  uint8_t random_payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  uint8_t response[256];

  int response_len = send_message(slave_index, message_type, random_payload, 8, response);

  if (response_len != 8) {
    return false;
  }

  for (int i=0; i<8; i++) {
    if (response[i] != random_payload[7-i]) {
      return false;
    }
  }
  return true;
}



void blink(int count, bool is_error) {
  for (int i=0; i<count; i++) {
    if (is_error) {
      pixel.setPixelColor(0, 255, 0, 0); // red
    } else {
      pixel.setPixelColor(0, 255, 153, 0); // orange
    }
    pixel.show();

    delay(200);

    pixel.setPixelColor(0, 0, 0, 0);
    pixel.show();

    if (i < count-1) {
      delay(100);
    } else {
      delay(10);
    }
  }
}

bool sync_clocks(int slave_index) {
  long avg_offset = 0;
  long min_offset = LONG_MAX;
  long max_offset = LONG_MIN;

  unsigned long rtt[5];

  uint8_t message_type[3] = {'s', 'y', 'n'};
  uint8_t response_buffer[256];

  // warm the connection up
  for (int i=0; i<3; i++) {
    int response_len = send_message(slave_index, message_type, nullptr, 0, response_buffer);
    if (response_len < 0) {
      return false;
    }
    delay(100);
  }

  for (int i=0; i<5; i++) {
    unsigned long t0 = millis();
    int response_len = send_message(slave_index, message_type, nullptr, 0, response_buffer);
    unsigned long t2 = millis();

    // response payload should be just 4 bytes of the timestamp

    if (response_len != 4) {
      Serial.println("Sync failed: invalid response from slave, invalid format.");
      return false;
    }

    unsigned long t1 = bytes_to_int32(response_buffer);

    long offset = (long)t1 - (long)((t0 + t2) / 2);
    avg_offset += offset;
    if (offset < min_offset) {
      min_offset = offset;
    }
    if (offset > max_offset) {
      max_offset = offset;
    }

    rtt[i] = t2 - t0;

    delay(100);
  }

  unsigned long avg_rtt = (rtt[0] + rtt[1] + rtt[2] + rtt[3] + rtt[4]) / 5;
  Serial.printf("Sync for slave %d done. Average RTT %lu ms\n", slave_index, avg_rtt);

  if (max_offset - min_offset > 20) {
    Serial.printf("Warning! Great variance in slave %d clock offsets: %ld - %ld ms\n", slave_index, min_offset, max_offset);
    Serial.printf("Individual RTTs were %lu, %lu, %lu, %lu, %lu\n", rtt[0], rtt[1], rtt[2], rtt[3], rtt[4]);
  }

  slave_clock_offsets[slave_index] = avg_offset / 5;

  return true;
}


void setup() {
  pixel.begin();
  pixel.setBrightness(255);

  blink(1, false);

  Serial.begin(115200);
  #if ARDUINO_USB_CDC_ON_BOOT
    unsigned long serial_init_time = millis();
    while (!Serial && (millis() - serial_init_time) < 1000) {
      delay(10);
    }
    if (!Serial) {
      blink(10, true);
      delay(1000);
    }
  #endif
  Serial.println("Valokenno-IoT Master node");

  while (true) {
    switchToEspNow();

    bool ping_pong_successful = true;
    for (int i=0; i<slave_count; i++) {
      if (!send_ping_pong(i)) {
        blink(2, true);
        delay(1000);
        ping_pong_successful = false;
        break;
      }
    }
    if (!ping_pong_successful) {
      continue;
    }

    bool clock_sync_successful = true;
    for (int i=0; i<slave_count; i++) {
      if (!sync_clocks(i)) {
        blink(4, true);
        delay(1000);
        clock_sync_successful = false;
        break;
      }
    }
    if (!clock_sync_successful) {
      continue;
    }

    switchToApMode();
    break;
  }

  setup_communications();

  #ifdef MASTER_TYPE_SENSOR
    while (true) {
      if (!setup_sensor()) {
        Serial.println("Sensor setup failed");
        blink(6, true);
        delay(1000);
        continue;
      }
      break;
    }
  #endif

  blink(1, false);
  Serial.println("Setup completed");
}

void loop() {
  loop_communications();

  #ifdef MASTER_TYPE_SENSOR
    loop_sensor();
  #endif

  // delay(100);
}
