#include <Wire.h>
#include <VL53L0X.h>

#define MOTION_DETECTION_THRESHOLD 1200 // millimeters

VL53L0X sensor;

unsigned long last_sensor_timestamp = 0;
bool did_detect_motion_last_time = false;


void set_led(bool is_on) {
  digitalWrite(LED_PIN, is_on ? HIGH : LOW);
}


void setup_sensor() {
  Wire.begin(14, 15);

  sensor.init();
  sensor.setTimeout(500);

  sensor.setMeasurementTimingBudget(20000);

  sensor.startContinuous();
}

void loop_sensor() {
  uint16_t distance_reading = sensor.readRangeContinuousMillimeters();
  unsigned long timestamp = millis();

  if (distance_reading <= MOTION_DETECTION_THRESHOLD) {
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
