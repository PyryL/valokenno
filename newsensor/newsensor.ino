
void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");

  setup_sensor();
  Serial.println("Setup done");
}

bool motion_last_frame = false;
unsigned long int last_frame_time = 0;

void loop() {
  unsigned long int time = 0;
  int dist = measure(time);

  if (dist > 0 && dist < 120) {
    if (!motion_last_frame) {
      unsigned long int time_delta = time - last_frame_time;
      Serial.printf("Motion: %d ms\n", time_delta);
    }
    motion_last_frame = true;
  } else {
    motion_last_frame = false;
  }

  last_frame_time = time;

  // delay(10);
}
