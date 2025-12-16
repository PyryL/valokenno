#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <Adafruit_NeoPixel.h>

// SLAVE

Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);

volatile uint8_t masterMac[6];

bool fatal_setup_error = false;

uint8_t pending_request[256];
int pending_request_len;
volatile bool has_pending_request = false;

std::vector<unsigned long> motion_timestamps = {};

void blink(int count, bool is_error) {
  for (int i=0; i<count; i++) {
    if (is_error) {
      pixel.setPixelColor(0, 255, 0, 0); // red
    } else {
      pixel.setPixelColor(0, 255, 153, 0); // orange
    }
    pixel.show();

    delay(200);

    pixel.setPixelColor(0, 0, 0, 0);
    pixel.show();

    if (i < count-1) {
      delay(100);
    } else {
      delay(10);
    }
  }
}

void send_response(uint8_t *response, int response_len) {
  const int MAX_RESPONSE_LEN = (256 < ESP_NOW_MAX_DATA_LEN) ? 256 : ESP_NOW_MAX_DATA_LEN;

  if (response_len > MAX_RESPONSE_LEN) {
    Serial.println("Tried to send too long response");
    return;
  }

  esp_err_t success = esp_now_send((const uint8_t*)masterMac, response, response_len);
  if (success != ESP_OK) {
    Serial.println("Response sending failed: " + success);
  }
}

void onReceive(const esp_now_recv_info* info, const unsigned char* data, int len) {
  if (len < 7) {
    // invalid request
    return;
  }

  // new master detection
  if (memcmp((const void*)masterMac, (const void*)info->src_addr, 6) != 0) {
    update_master_mac(info->src_addr);
  }

  // handle clock sync immediately in the interruption to minimize latency
  if (data[4] == 's' && data[5] == 'y' && data[6] == 'n') {
    handle_clock_sync(data);
    return;
  }

  // TODO: should the new incoming request replace the existing unhandled one?
  if (has_pending_request) {
    Serial.println("Received request while previous was still unhandled");
    return;
  }

  if (len > sizeof(pending_request)) {
    Serial.println("Received too long request");
    return;
  }

  memcpy(pending_request, data, len);
  pending_request_len = len;
  has_pending_request = true;
}

void handle_ping_pong() {
  Serial.println("Received ping-pong");

  if (pending_request_len != 15) {
    Serial.println("Invalid ping-pong request");
    return;
  }

  uint8_t response[12];
  memcpy(response, pending_request, 4); // request id

  // reverse the payload for response
  for (int i=0; i<8; i++) {
    response[i+4] = pending_request[14-i];
  }

  send_response(response, 12);
}

void int32_to_bytes(uint32_t value, uint8_t destination[4]) {
  destination[0] = (value >> 24) & 0xFF; // most sig
  destination[1] = (value >> 16) & 0xFF;
  destination[2] = (value >> 8) & 0xFF;
  destination[3] = value & 0xFF;
}

void handle_clock_sync(const uint8_t *request) {
  // Serial.println("Received clock sync");

  uint8_t response[8];
  memcpy(response, request, 4); // request id

  unsigned long current_time = millis();
  int32_to_bytes((uint32_t)current_time, response + 4);

  esp_err_t success = esp_now_send((const uint8_t*)masterMac, response, 8);
  if (success != ESP_OK) {
    Serial.println("Sync response sending failed: " + String(success));
  }
}

void handle_motion_timestamp_request() {
  Serial.println("Received motion timestamps request");

  int response_len = 4 + 4 * motion_timestamps.size();
  uint8_t response[256];

  if (response_len > 256) {
    Serial.println("Too many timestamps to send");
    return;
  }

  memcpy(response, pending_request, 4); // request id

  for (int i=0; i<motion_timestamps.size(); i++) {
    int32_to_bytes((uint32_t)motion_timestamps[i], response + (4 + 4 * i));
  }

  send_response(response, response_len);
}

void handle_clear_request() {
  Serial.println("Clearing timestamps");

  motion_timestamps.clear();

  uint8_t response[6];
  memcpy(response, pending_request, 4); // request id
  response[4] = 'o';
  response[5] = 'k';

  send_response(response, 6);
}

void update_master_mac(const uint8_t *new_mac) {
  Serial.printf(
    "Updated to new master: %02X:%02X:%02X:%02X:%02X:%02X\n",
    new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]
  );

  // delete any existing peer
  esp_now_del_peer((const uint8_t*)masterMac);

  memcpy((void*)masterMac, new_mac, 6);

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, (const void*)masterMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;

  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK) {
    Serial.printf("ESP-NOW peer adding failed: %d\n", result);
  }
}

bool setup_wifi() {
  WiFi.mode(WIFI_STA);

  // Serial.print("Waiting mac address...");
  // while (WiFi.macAddress() == "00:00:00:00:00:00") {
  //   delay(100);
  //   Serial.print(".");
  // }
  // Serial.println("\nMAC address: " + WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  if (esp_now_register_recv_cb(onReceive) != ESP_OK) {
    Serial.println("ESP-NOW callback function register failed");
    return false;
  }

  return true;
}

void setup() {
  pixel.begin();
  pixel.setBrightness(255);

  blink(1, false);

  Serial.begin(115200);
  #if ARDUINO_USB_CDC_ON_BOOT
    unsigned long serial_init_time = millis();
    while (!Serial && (millis() - serial_init_time) < 1000) {
      delay(10);
    }
  #endif
  Serial.println("Valokenno-IoT Slave node");

  if (!setup_wifi()) {
    fatal_setup_error = true;
    return;
  }

  while (true) {
    if (!setup_sensor()) {
      Serial.println("Sensor setup failed");
      blink(6, true);
      delay(1000);
      continue;
    }
    break;
  }

  blink(1, false);
  Serial.println("Setup completed");
}

void loop() {
  if (fatal_setup_error) {
    pixel.setPixelColor(0, 255, 0, 0); // red
    pixel.show();
    delay(1000);
    return;
  }

  if (has_pending_request) {
    if (memcmp(pending_request + 4, "pin", 3) == 0) {
      handle_ping_pong();
    } else if (memcmp(pending_request + 4, "tim", 3) == 0) {
      handle_motion_timestamp_request();
    } else if (memcmp(pending_request + 4, "cle", 3) == 0) {
      handle_clear_request();
    } else {
      Serial.println("Received unexpected message");
    }

    has_pending_request = false;
  }

  loop_sensor();
}
