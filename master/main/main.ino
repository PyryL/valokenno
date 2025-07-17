#include <esp_now.h>
#include <WiFi.h>
#include <vector>

// MASTER

#define LED_PIN 4 // led

unsigned long slave_clock_offset;

std::vector<unsigned long> motion_timestamps = {};


bool send_ping_pong() {
  Serial.println("Sending ping-pong");
  String random_payload = "12345678";
  String response = send_message("pin" + random_payload);
  if (response == ("pon" + random_payload)) {
    return true;
  } else {
    Serial.println("Ping-pong epäonnistui: \"" + response + "\"");
    return false;
  }
}



void blink(int count) {
  for (int i=0; i<count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    if (i < count-1) {
      delay(100);
    } else {
      delay(10);
    }
  }
}

bool sync_clocks() {
  unsigned long avg_offset;
  unsigned long min_offset = 9999999999999;
  unsigned long max_offset = 0;

  unsigned long rtt[5];

  // warm the connection up
  for (int i=0; i<3; i++) {
    send_message("syn");
    delay(100);
  }

  for (int i=0; i<5; i++) {
    unsigned long t0 = millis();
    String response = send_message("syn");
    unsigned long t2 = millis();

    if (response.length() < 4 || response.substring(0, 3) != "syr") {
      Serial.println("Sync failed: invalid response from slave, invalid format. \"" + response + "\"");
      return false;
    }
    char* end_ptr;
    unsigned long t1 = strtoul(response.substring(3).c_str(), &end_ptr, 10);
    if (*end_ptr != '\0') { // conversion failed
      Serial.println("Sync failed: invalid response from slave, conversion failed. \"" + response + "\"");
      return false;
    }

    unsigned long offset = t1 - (t0 + t2) / 2;
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
  Serial.printf("Sync done. Average RTT %d ms\n", avg_rtt);

  if (max_offset - min_offset > 20) {
    Serial.printf("Warning! Great variance in clock offsets: %d - %d ms\n", min_offset, max_offset);
    Serial.printf("Individual RTTs were %d, %d, %d, %d, %d\n", rtt[0], rtt[1], rtt[2], rtt[3], rtt[4]);
  }

  slave_clock_offset = avg_offset / 5;

  return true;
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Valokenno-IoT Master node");

  blink(1);

  switchToEspNow();
  send_ping_pong();
  sync_clocks();
  switchToApMode();

  setup_communications();

  setup_sensor();

  Serial.println("Setup completed");
}

void loop() {
  loop_communications();

  loop_sensor();

  // delay(100);
}
