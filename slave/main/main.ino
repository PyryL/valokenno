#include <esp_now.h>
#include <WiFi.h>

// SLAVE

#define LED_PIN 4 // led

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

void onReceive(const esp_now_recv_info* info, const unsigned char* data, int len) {
  String msg = "";
  for (int i = 0; i < len; i++) {
    msg += (char)data[i];
  }
  Serial.println("Vastaanotettiin: " + msg);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);

  // Serial.print("Waiting mac address...");
  // while(WiFi.macAddress() == "00:00:00:00:00:00") {
  //   delay(100);
  //   Serial.print(".");
  // }
  // Serial.println("\nMAC address: " + WiFi.macAddress());

  esp_now_init();
  // if (esp_now_init() != ESP_OK) {
  //   Serial.println("ESP-NOW init failed");
  //   return false;
  // }
  esp_now_register_recv_cb(onReceive);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Valokenno-IoT Slave node");

  setup_wifi();

  blink(1);
}

void loop() {
  //
  delay(1000);
}
