
HardwareSerial serial(1);

void setup_sensor() {
  serial.begin(115200, SERIAL_8N1, 14, 15);
}

/*
Reads the latest distance information from the sensor and returns it in centrimeters.
-1 is returned in case of an error.
*/
int measure(int& clear_info) {
    // clear serial buffer to ensure latest data is used
    int cleared = 0;
    while (serial.available() && cleared < 512) {
        serial.read();
        cleared++;
    }
    clear_info = cleared;

    // read the next frame
    uint8_t frame[9];
    int pos = 0;
    unsigned long start_time = millis();

    while (pos < 9 && millis() - start_time < 100) {
        if (!serial.available()) continue;

        uint8_t byte = serial.read();

        if (pos < 2) {
            if (pos == 0 && byte == 0x59) {
                frame[0] = byte;
                pos = 1;
            } else if (pos == 1 && byte == 0x59) {
                frame[1] = byte;
                pos = 2;
            } else {
                pos = 0;
            }
        } else {
            frame[pos++] = byte;
        }
    }

    // check timeout
    if (pos < 9) {
        Serial.println("Sensor timed out");
        return -1;
    }

    // check the checksum
    uint8_t checksum = 0;
    for (int i = 0; i < 8; i++) {
        checksum += frame[i];
    }
    if ((checksum & 0xFF) != frame[8]) {
        Serial.println("Sensor invalid checksum");
        return -1;
    }

    int distance = frame[2] + (frame[3] << 8);
    return distance;
}
