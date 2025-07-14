#include <WebServer.h>

uint8_t slaveMac[] = {0xC8, 0xF0, 0x9E, 0x4D, 0x64, 0x0C};

enum RadioMode {
  ESP_NOW_MODE,
  AP_MODE
};

RadioMode current_radio_mode = AP_MODE;

volatile bool esp_now_waiting_response = false;
char esp_now_received_response[256];

volatile bool pending_timestamp_process = false;
String timestamp_response = "";

WebServer ap_server(80);


void switchToEspNow() {
  if (current_radio_mode == AP_MODE) {
    bool ap_success = WiFi.disconnect(true);
    delay(100);

    WiFi.mode(WIFI_STA);
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
    WiFi.mode(WIFI_AP);
    bool ap_success = WiFi.softAP("Valokenno", "pyrypyrypyry", 1);

    current_radio_mode = AP_MODE;
  }
}



void espNowReceived(const esp_now_recv_info* info, const unsigned char* data, int len) {
  if (!esp_now_waiting_response) {
    Serial.println("ESP-NOW received unexpected response");
    return;
  }

  if (current_radio_mode != ESP_NOW_MODE) {
    Serial.println("ESP-NOW received response while in another mode");
    return;
  }

  memcpy(esp_now_received_response, data, len);
  esp_now_received_response[len] = '\0';
  esp_now_waiting_response = false;
}

String send_message(String message) {
  if (current_radio_mode != ESP_NOW_MODE) {
    Serial.println("Tried sending in wrong mode");
    return "";
  }

  esp_now_waiting_response = true;
  esp_err_t send_success = esp_now_send(slaveMac, (uint8_t*)message.c_str(), message.length());

  while (esp_now_waiting_response) {
    delay(1);
  }

  String response = String(esp_now_received_response);
  esp_now_received_response[0] = '\0';

  return response;
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


void setup_communications() {
  ap_server.on("/status", HTTP_GET, handle_ap_status_request);
  ap_server.on("/timestamps", HTTP_GET, handle_ap_timestamp_request);
  ap_server.on("/timestamps/result", HTTP_GET, handle_ap_timestamp_result_request);
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
    // motion_timestamps.clear();

    timestamp_response += ";dev2,";
    String slave_timestamps_str = send_message("tim");
    Serial.println("Slave timestamps: " + slave_timestamps_str);
    std::vector<String> slave_timestamps;
    split(slave_timestamps_str, ",", &slave_timestamps);
    for (String slave_timestamp_str : slave_timestamps) {
      unsigned long slave_timestamp = strtoul(slave_timestamp_str.c_str(), NULL, 10);
      unsigned long unshifted_timestamp = slave_timestamp - slave_clock_offset;
      timestamp_response += String(unshifted_timestamp) + ",";
    }
    timestamp_response.remove(timestamp_response.length() - 1);

    switchToApMode();
    Serial.println("Timestamp response: " + timestamp_response);
    pending_timestamp_process = false;
  }
}


