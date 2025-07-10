#include <esp_now.h>
#include <WiFi.h>
#include <inttypes.h> // todo

// SLAVE

#define LED_PIN 4 // led

uint8_t masterMac[] = {0xC8, 0xF0, 0x9E, 0x4D, 0xB5, 0xA8};

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

void send_response(String message) {
  esp_now_send(masterMac, (uint8_t*)message.c_str(), message.length());
}

void onReceive(const esp_now_recv_info* info, const unsigned char* data, int len) {
  String msg = "";
  for (int i = 0; i < len; i++) {
    msg += (char)data[i];
  }
  
  String message_type = msg.substring(0, 3);

  if (message_type == "pin") {
    handle_ping_pong(msg);
  } else if (message_type == "syn") {
    handle_clock_sync();
  } else {
    Serial.println("Received unexpected message: " + msg);
  }
}

void handle_ping_pong(String message) {
  Serial.println("Received ping-pong " + message);
  String random_payload = message.substring(3, 11);
  send_response("pon" + random_payload);
}

void handle_clock_sync() {
  Serial.println("Received clock sync");
  unsigned long time_now = millis();
  char time_string[11];
  sprintf(time_string, "%010lu", time_now);
  String message = "syr" + String(time_string);
  send_response(message);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);

  // Serial.print("Waiting mac address...");
  // while (WiFi.macAddress() == "00:00:00:00:00:00") {
  //   delay(100);
  //   Serial.print(".");
  // }
  // Serial.println("\nMAC address: " + WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, masterMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK) {
    Serial.printf("Masterin lisäys epäonnistui: %d\n", result);
  }
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
