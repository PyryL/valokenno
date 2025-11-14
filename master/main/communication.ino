#include <WebServer.h>
#include "esp_wifi.h"

uint8_t slaveMac[] = {0xDC, 0x54, 0x75, 0xC1, 0xDD, 0xA4};

enum RadioMode {
  ESP_NOW_MODE,
  AP_MODE
};

RadioMode current_radio_mode = AP_MODE;

uint32_t esp_now_waiting_response_id = 0;
volatile bool esp_now_waiting_response = false;
uint8_t esp_now_received_response[256];
volatile int esp_now_received_response_len;

volatile bool pending_timestamp_process = false;
String timestamp_response = "";

volatile bool pending_clear_process = false;
String clear_process_response = "";

WebServer ap_server(80);


void switchToEspNow() {
  if (current_radio_mode == AP_MODE) {
    bool ap_success = WiFi.disconnect(true);
    delay(100);

    WiFi.mode(WIFI_STA);

    // while (WiFi.macAddress() == "00:00:00:00:00:00") {
    //   delay(100);
    // }
    // Serial.println("MAC address: " + WiFi.macAddress());

    esp_err_t esp_now_success = esp_now_init();

    esp_now_register_recv_cb(espNowReceived);

    esp_now_peer_info_t peer;
    // memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, slaveMac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    peer.ifidx = WIFI_IF_STA;
    esp_err_t result = esp_now_add_peer(&peer);

    current_radio_mode = ESP_NOW_MODE;
  }
}

void switchToApMode() {
  if (current_radio_mode == ESP_NOW_MODE) {
    if (esp_now_waiting_response) {
      Serial.println("Switching to AP while ESP-NOW still waiting for response");
      esp_now_waiting_response = false;
    }

    esp_now_deinit();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    esp_wifi_stop();
    delay(100);
    esp_wifi_start();
    delay(100);
    WiFi.mode(WIFI_AP);
    bool ap_success = WiFi.softAP("Valokenno", "pyrypyrypyry", 1);

    current_radio_mode = AP_MODE;
  }
}



void espNowReceived(const esp_now_recv_info* info, const unsigned char* data, int len) {
  if (!esp_now_waiting_response) {
    return;
  }

  // if (current_radio_mode != ESP_NOW_MODE) {
  //   Serial.println("ESP-NOW received response while in another mode");
  //   return;
  // }

  if (len < 4) {
    // invalid response
    return;
  }

  uint8_t request_id_bytes[4];
  memcpy(request_id_bytes, data, 4);
  uint32_t request_id = bytes_to_int32(request_id_bytes);

  if (request_id != esp_now_waiting_response_id) {
    // received outdated response
    return;
  }

  if (len > 256) {
    Serial.println("Received too long response");
    return;
  }

  int payload_len = len - 4;
  memcpy(esp_now_received_response, data + 4, payload_len);
  esp_now_received_response_len = payload_len;
  esp_now_waiting_response = false;
}

/*
Copies the given integer into 4-byte big endian byte array.
*/
void int32_to_bytes(uint32_t value, uint8_t destination[4]) {
  destination[0] = (value >> 24) & 0xFF; // most sig
  destination[1] = (value >> 16) & 0xFF;
  destination[2] = (value >> 8) & 0xFF;
  destination[3] = value & 0xFF;
}

/*
Exactly 4 first bytes of the buffer are used.
*/
uint32_t bytes_to_int32(uint8_t bytes[4]) {
  return ((uint32_t)bytes[0] << 24) |
         ((uint32_t)bytes[1] << 16) |
         ((uint32_t)bytes[2] << 8) |
         ((uint32_t)bytes[3]);
}

/*
Response payload buffer must be 256 bytes long.
Returns the response payload length, or `-1` on error.
*/
int send_message(uint8_t message_type[3], uint8_t *request_payload, int request_payload_len, uint8_t *response_payload) {
  if (current_radio_mode != ESP_NOW_MODE) {
    Serial.println("Tried sending in wrong mode");
    return -1;
  }

  if (esp_now_waiting_response) {
    return -1;
  }

  // request:
  // * request id, 4 bytes
  // * msg type, 3 bytes
  // * payload, rest of the bytes

  esp_now_waiting_response_id++;
  uint8_t request_id_bytes[4];
  int32_to_bytes(esp_now_waiting_response_id, request_id_bytes);

  int request_length = 4 + 3 + request_payload_len;
  uint8_t request[256];

  if (request_length > 256 || request_length > ESP_NOW_MAX_DATA_LEN) {
    Serial.println("Tried to send too long request");
    return -1;
  }

  memcpy(request, request_id_bytes, 4);
  memcpy(request + 4, message_type, 3);
  memcpy(request + 4 + 3, request_payload, request_payload_len);

  esp_now_waiting_response = true;

  esp_err_t send_success = esp_now_send(slaveMac, request, request_length);
  if (send_success != ESP_OK) {
    esp_now_waiting_response = false;
    return -1;
  }


  // response:
  // * request id, 4 bytes
  // * payload, rest of the bytes

  unsigned long message_sent_time = millis();
  while (esp_now_waiting_response && (millis() - message_sent_time) < 3000) {
    delay(1);
  }
  if (esp_now_waiting_response) {
    esp_now_waiting_response = false;
    return -1; // timeout
  }

  memcpy(response_payload, esp_now_received_response, esp_now_received_response_len);
  return esp_now_received_response_len;
}


void split(String s, String delim, std::vector<String>* result) {
  int start = 0;
  int end = s.indexOf(delim);

  while (end != -1) {
    result->push_back(s.substring(start, end));
    start = end + 1;
    end = s.indexOf(delim, start);
  }

  result->push_back(s.substring(start));
}

void handle_ap_timestamp_request() {
  // TODO: check if pending_timestamp_process already true
  Serial.println("Received timestamp request");
  pending_timestamp_process = true;
  ap_server.send(200, "text/plain", "Process started");
}

void handle_ap_timestamp_result_request() {
  if (timestamp_response.length() > 0) {
    ap_server.send(200, "text/plain", timestamp_response);
    timestamp_response = "";
  } else {
    ap_server.send(404, "text/plain", "Still processing");
  }
}

void handle_ap_status_request() {
  Serial.println("Status request received");
  ap_server.send(200, "text/plain", "Valokenno toiminnassa");
}

void handle_ap_clear_request() {
  Serial.println("Clear request received");
  pending_clear_process = true;
  ap_server.send(200, "text/plain", "Started");
}

void handle_ap_clear_result_request() {
  if (clear_process_response.length() > 0) {
    ap_server.send(200, "text/plain", clear_process_response);
    clear_process_response = "";
  } else {
    ap_server.send(404, "text/plain", "Still processing");
  }
}


void setup_communications() {
  ap_server.on("/status", HTTP_GET, handle_ap_status_request);
  ap_server.on("/timestamps", HTTP_GET, handle_ap_timestamp_request);
  ap_server.on("/timestamps/result", HTTP_GET, handle_ap_timestamp_result_request);
  ap_server.on("/clear", HTTP_GET, handle_ap_clear_request);
  ap_server.on("/clear/result", HTTP_GET, handle_ap_clear_result_request);
  ap_server.begin();

  // Serial.println("Access point IP: " + WiFi.softAPIP().toString());
}

void loop_communications() {
  if (current_radio_mode == AP_MODE) {
    ap_server.handleClient();
  }

  if (pending_timestamp_process) {
    switchToEspNow();

    timestamp_response = "dev1,";
    for (unsigned long timestamp : motion_timestamps) {
      timestamp_response += String(timestamp) + ",";
    }
    timestamp_response.remove(timestamp_response.length() - 1);

    uint8_t message_type[3] = {'t', 'i', 'm'};
    uint8_t slave_response[256];
    int slave_response_len = send_message(message_type, nullptr, 0, slave_response);

    if (slave_response_len < 0 || slave_response_len % 4 != 0) {
      Serial.println("Slave timestamp response failed");
    } else {
      timestamp_response += ";dev2,";
      int timestamp_count = slave_response_len / 4;
      for (int i=0; i<timestamp_count; i++) {
        unsigned long slave_timestamp = bytes_to_int32(slave_response + (4 * i));
        unsigned long unshifted_timestamp = (unsigned long)((long)slave_timestamp - slave_clock_offset);
        timestamp_response += String(unshifted_timestamp) + ",";
      }
      timestamp_response.remove(timestamp_response.length() - 1);
    }

    switchToApMode();
    Serial.println("Timestamp response: " + timestamp_response);
    pending_timestamp_process = false;
  }

  if (pending_clear_process) {
    switchToEspNow();

    uint8_t message_type[3] = {'c', 'l', 'e'};
    uint8_t slave_response[256];
    int slave_response_len = send_message(message_type, nullptr, 0, slave_response);

    if (slave_response_len == 2 && slave_response[0] == 'o' && slave_response[1] == 'k') {
      motion_timestamps.clear();
      clear_process_response = "ok";
    } else {
      clear_process_response = "failed";
    }

    switchToApMode();
    Serial.println("Clear process response: " + clear_process_response);
    pending_clear_process = false;
  }
}


