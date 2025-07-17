#include <Wire.h>
#include <VL53L0X.h>

#define MOTION_DETECTION_MIN_THRESHOLD 100 // millimeters
#define MOTION_DETECTION_MAX_THRESHOLD 1000 // millimeters

VL53L0X sensor;

bool is_sensor_set_up_successfully = false;

unsigned long last_sensor_timestamp = 0;
bool did_detect_motion_last_time = false;


void set_led(bool is_on) {
  digitalWrite(LED_PIN, is_on ? HIGH : LOW);
}


void setup_sensor() {
  bool i2c_success = Wire.begin(14, 15);
  if (!i2c_success) {
    Serial.println("I2C setup failed");
    return;
  }

  bool sensor_success = sensor.init();
  if (!sensor_success) {
    Serial.println("Sensor init failed"); // happens when sensor not wired up
    return;
  }

  sensor.setTimeout(500);

  bool budget_success = sensor.setMeasurementTimingBudget(20000);
  if (!budget_success) {
    Serial.println("Measurement timing budget setting failed");
    return;
  }

  sensor.startContinuous();

  is_sensor_set_up_successfully = true;
}

void loop_sensor() {
  if (!is_sensor_set_up_successfully) {
    return;
  }

  uint16_t distance_reading = sensor.readRangeContinuousMillimeters();
  unsigned long timestamp = millis();

  if (distance_reading > MOTION_DETECTION_MIN_THRESHOLD && distance_reading <= MOTION_DETECTION_MAX_THRESHOLD) {
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
