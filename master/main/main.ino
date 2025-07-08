#include <esp_now.h>
#include <WiFi.h>

// MASTER

#define LED_PIN 4 // led

uint8_t slaveMac[] = {0xC8, 0xF0, 0x9E, 0x4D, 0xB5, 0xA8}; // C8:F0:9E:4D:B5:A8

bool is_slave_connected = false;

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

bool connect_slave() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, slaveMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result == ESP_OK) {
    Serial.println("Slave lisätty onnistuneesti");
  } else {
    Serial.printf("Slaven lisäys epäonnistui: %d\n", result);
    return false;
  }

  // TODO: ping-pong handshake

  return true;
}

bool send_message(String message) {
  return esp_now_send(slaveMac, (uint8_t*)message.c_str(), message.length()) == ESP_OK;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Valokenno-IoT Master node");

  blink(1);

  is_slave_connected = connect_slave();

  if (is_slave_connected) {
    blink(2);
  } else {
    blink(7);
  }
}

void loop() {
  if (is_slave_connected) {
    send_message("Moikka slave");
  }

  delay(10000);
}
