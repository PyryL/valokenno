
// #define MOTION_DETECTION_MIN_THRESHOLD 10 // centimeters
#define MOTION_DETECTION_MAX_THRESHOLD 100 // centimeters

HardwareSerial serial(1);

bool is_sensor_set_up_successfully = false;

unsigned long last_sensor_timestamp = 0;
bool did_detect_motion_last_time = false;


void set_led(bool is_on) {
  digitalWrite(LED_PIN, is_on ? HIGH : LOW);
}


bool setup_sensor() {
  serial.begin(115200, SERIAL_8N1, 14, 15);

  delay(100);

  // try to get a valid reading with 10 retries
  for (int i=0; i<10; i++) {
    unsigned long int timestamp;
    int distance = measure(timestamp);
    if (distance > 0) {
      is_sensor_set_up_successfully = true;
      break;
    }
  }

  return is_sensor_set_up_successfully;
}

/*
Reads the latest distance information from the sensor and returns it in centrimeters.
-1 is returned in case of an error.
Timestamp is only valid if the returned distance is valid.
*/
int measure(unsigned long int& timestamp) {
  // clear serial buffer to ensure latest data is used
  int cleared = 0;
  while (serial.available() && cleared < 512) {
    serial.read();
    cleared++;
  }

  // read the next frame
  uint8_t frame[9];
  int pos = 0;
  unsigned long int start_time = millis();

  while (pos < 9 && millis() - start_time < 100) {
    if (!serial.available()) continue;

    uint8_t byte = serial.read();

    if (pos < 2) {
      if (pos == 0 && byte == 0x59) {
        frame[0] = byte;
        pos = 1;
        timestamp = millis();
      } else if (pos == 1 && byte == 0x59) {
        frame[1] = byte;
        pos = 2;
      } else {
        pos = 0;
      }
    } else {
      frame[pos++] = byte;
    }
  }

  // check timeout
  if (pos < 9) {
    Serial.println("Sensor timed out");
    return -1;
  }

  // check the checksum
  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    checksum += frame[i];
  }
  if ((checksum & 0xFF) != frame[8]) {
    Serial.println("Sensor invalid checksum");
    return -1;
  }

  int distance = frame[2] + (frame[3] << 8);
  return distance;
}

void loop_sensor() {
  if (!is_sensor_set_up_successfully) {
    return;
  }

  unsigned long int timestamp;
  int distance_reading = measure(timestamp);
  Serial.printf("Distance: %3d\n", distance_reading);

  if (distance_reading > 0 && distance_reading <= MOTION_DETECTION_MAX_THRESHOLD) {
    if (!did_detect_motion_last_time) {
      motion_timestamps.push_back(timestamp);
      did_detect_motion_last_time = true;
      set_led(true);
    }
  } else {
    did_detect_motion_last_time = false;
    set_led(false);
  }

  last_sensor_timestamp = timestamp;
}
