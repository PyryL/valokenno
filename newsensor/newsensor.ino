
void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");

  setup_sensor();
  Serial.println("Setup done");
}

int previous_time = millis();

void loop() {
  int clear_info;
  int dist = measure(clear_info);
  int t = millis();
  Serial.printf("Distance: %3d %3d %6d\n", dist, clear_info, t - previous_time);
  previous_time = t;

  // delay(10);
}
