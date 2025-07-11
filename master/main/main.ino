#include <esp_now.h>
#include <WiFi.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include <vector>

// MASTER

#define LED_PIN 4 // led

uint8_t slaveMac[] = {0xC8, 0xF0, 0x9E, 0x4D, 0x64, 0x0C};

bool is_slave_connected = false;
bool is_waiting_for_response = false;
String received_response = "";

unsigned long slave_clock_offset;

BLEServer* pServer;
BLECharacteristic* pCharacteristic;

bool was_detecting_motion_last_time = false;
std::vector<unsigned long> motion_timestamps;

String send_message(String message) {
  if (!is_slave_connected) {
    Serial.println("Trying to send message without connected slave");
    return "";
  }

  is_waiting_for_response = true;
  bool send_success = esp_now_send(slaveMac, (uint8_t*)message.c_str(), message.length());
  if (send_success != ESP_OK) {
    Serial.println("Message send failed: " + String(send_success));
    is_waiting_for_response = false;
    return "";
  }

  while (is_waiting_for_response) {
    delay(1);
  }

  String response = received_response;
  received_response = "";

  return response;
}

void onReceive(const esp_now_recv_info* info, const unsigned char* data, int len) {
  String msg = "";
  for (int i = 0; i < len; i++) {
    msg += (char)data[i];
  }

  received_response = msg;
  is_waiting_for_response = false;
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


class BluetoothCallback: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic* pCharacteristic) {
    Serial.println("Sending motion timestamps via bluetooth");

    String value = "dev1:";
    for (unsigned long timestamp : motion_timestamps) {
      value += String(timestamp) + ",";
    }
    motion_timestamps.clear();

    value += "dev2:";
    String slave_timestamps_str = send_message("tim");
    Serial.println("Slave timestamps: " + slave_timestamps_str);
    std::vector<String> slave_timestamps;
    split(slave_timestamps_str, ",", &slave_timestamps);
    slave_timestamps.pop_back(); // trailing comma
    for (String slave_timestamp_str : slave_timestamps) {
      unsigned long slave_timestamp = strtoul(slave_timestamp_str.c_str(), NULL, 10);
      unsigned long unshifted_timestamp = slave_timestamp - slave_clock_offset;
      value += String(unshifted_timestamp) + ",";
    }

    pCharacteristic->setValue(value);
  }

  void onWrite(BLECharacteristic* pCharacteristic) {
    // should not end up in here
  }
};




bool send_ping_pong() {
  Serial.println("Sending ping-pong");
  String random_payload = "12345678";
  String response = send_message("pin" + random_payload);
  if (response == ("pon" + random_payload)) {
    return true;
  } else {
    Serial.println("Ping-pong epäonnistui: \"" + response + "\"");
    return false;
  }
}



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

void connect_slave() {
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
  memcpy(peer.peer_addr, slaveMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK) {
    Serial.printf("Slaven lisäys epäonnistui: %d\n", result);
    return;
  }
  is_slave_connected = true;

  if (!send_ping_pong()) {
    is_slave_connected = false;
  }
}

bool sync_clocks() {
  unsigned long avg_offset;
  unsigned long min_offset = 9999999999999;
  unsigned long max_offset = 0;

  unsigned long rtt[5];

  // warm the connection up
  for (int i=0; i<3; i++) {
    send_message("syn");
    delay(100);
  }

  for (int i=0; i<5; i++) {
    unsigned long t0 = millis();
    String response = send_message("syn");
    unsigned long t2 = millis();

    if (response.length() != 13 || response.substring(0, 3) != "syr") {
      Serial.println("Sync failed: invalid response from slave, invalid format. \"" + response + "\"");
      return false;
    }
    char* end_ptr;
    unsigned long t1 = strtoul(response.substring(3, 13).c_str(), &end_ptr, 10);
    if (*end_ptr != '\0') { // conversion failed
      Serial.println("Sync failed: invalid response from slave, conversion failed. \"" + response + "\"");
      return false;
    }

    unsigned long offset = t1 - (t0 + t2) / 2;
    avg_offset += offset;
    if (offset < min_offset) {
      min_offset = offset;
    }
    if (offset > max_offset) {
      max_offset = offset;
    }

    rtt[i] = t2 - t0;

    delay(100);
  }

  unsigned long avg_rtt = (rtt[0] + rtt[1] + rtt[2] + rtt[3] + rtt[4]) / 5;
  Serial.printf("Sync done. Average RTT %d ms\n", avg_rtt);

  if (max_offset - min_offset > 20) {
    Serial.printf("Warning! Great variance in clock offsets: %d - %d ms\n", min_offset, max_offset);
    Serial.printf("Individual RTTs were %d, %d, %d, %d, %d\n", rtt[0], rtt[1], rtt[2], rtt[3], rtt[4]);
  }

  slave_clock_offset = avg_offset / 5;

  return true;
}


void init_bluetooth() {
  BLEDevice::init("Valokenno");
  pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService("6459c4ca-d023-43ca-a8d4-c43710315b7f");

  pCharacteristic = pService->createCharacteristic(
    "5e076e58-df1d-4630-b418-74079207a520",
    BLECharacteristic::PROPERTY_READ
  );
  pCharacteristic->setCallbacks(new BluetoothCallback());

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID("6459c4ca-d023-43ca-a8d4-c43710315b7f");
  pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);  // helps with iPhone connections
  // pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  Serial.println("Bluetooth service started");
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Valokenno-IoT Master node");

  blink(1);

  init_bluetooth();

  connect_slave();

  bool sync_success = false;
  if (is_slave_connected) {
    sync_success = sync_clocks();
  }

  if (is_slave_connected && sync_success) {
    blink(2);
  } else {
    blink(7);
    return;
  }

  Serial.println("Setup completed successfully");
}

int counter = 0; // dev only

void loop() {
  // bool detects_motion = digitalRead(MOTION_SENSOR_PIN);
  bool detects_motion = (counter % 50 == 0);
  if (detects_motion && !was_detecting_motion_last_time) {
    unsigned long motion_timestamp = millis();
    motion_timestamps.push_back(motion_timestamp);
    Serial.println("Detected motion");
  }
  was_detecting_motion_last_time = detects_motion;
  counter += 1;


  delay(100);
}
