#include <esp_now.h>
#include <WiFi.h>
#include <vector>

// SLAVE

#define LED_PIN 4 // led

uint8_t masterMac[] = {0xC8, 0xF0, 0x9E, 0x4D, 0x5E, 0x08};

char pending_request[256];
volatile bool has_pending_request = false;

std::vector<unsigned long> motion_timestamps = {};

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
  if (message.length() >= ESP_NOW_MAX_DATA_LEN) {
    Serial.println("Tried to send too long response");
    return;
  }

  esp_err_t success = esp_now_send(masterMac, (uint8_t*)message.c_str(), message.length());
  if (success != ESP_OK) {
    Serial.println("Response sending failed: " + success);
  }
}

void onReceive(const esp_now_recv_info* info, const unsigned char* data, int len) {
  if (has_pending_request) {
    Serial.println("Received request while previous was still unhandled");
    return;
  }

  if (len >= sizeof(pending_request)) {
    Serial.println("Received too long request");
    return;
  }

  memcpy(pending_request, data, len);
  pending_request[len] = '\0';
  has_pending_request = true;
}

void handle_ping_pong(String message) {
  Serial.println("Received ping-pong " + message);
  String random_payload = message.substring(3, 11);
  send_response("pon" + random_payload);
}

void handle_clock_sync() {
  // Serial.println("Received clock sync");
  String message = "syr" + String(millis());
  send_response(message);
}

void handle_motion_timestamp_request() {
  Serial.println("Received motion timestamps request");
  String message = "";
  for (unsigned long timestamp : motion_timestamps) {
    message += String(timestamp) + ",";
  }
  if (message.length() > 0) {
    message.remove(message.length() - 1);
  }
  if (message.length() == 0) {
    message = "NA";
  }
  Serial.println("Sending this timestamp response: " + message);
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

  setup_sensor();

  blink(1);
}

void loop() {
  if (has_pending_request) {
    String msg = String(pending_request);

    String message_type = msg.substring(0, 3);

    if (message_type == "pin") {
      handle_ping_pong(msg);
    } else if (message_type == "syn") {
      handle_clock_sync();
    } else if (message_type == "tim") {
      handle_motion_timestamp_request();
    } else {
      Serial.println("Received unexpected message: " + msg);
    }

    pending_request[0] = '\0';
    has_pending_request = false;
  }



  loop_sensor();
  // delay(100);
}
