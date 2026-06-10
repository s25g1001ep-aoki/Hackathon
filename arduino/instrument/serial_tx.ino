// serial_tx.ino
#include "score_data.h"

const uint8_t SERIAL_HEADER_NOTE_ON  = 0x01; // type: NOTE_ON
const uint8_t SERIAL_HEADER_NOTE_OFF = 0x02; // type: NOTE_OFF

void init_serial_tx() {
    Serial.begin(115200);
}

void send_serial_frame(uint8_t type, uint8_t length, const uint8_t* payload) {
    uint8_t sum = type + length;
    Serial.write(type);
    Serial.write(length);
    for (uint8_t i = 0; i < length; i++) {
        Serial.write(payload[i]);
        sum += payload[i];
    }
    // 2の補数チェックサム (全バイト和の下位8bitが0になる値)
    uint8_t checksum = (~sum + 1) & 0xFF;
    Serial.write(checksum);
}

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    uint8_t payload[2] = {pitch, velocity};
    send_serial_frame(SERIAL_HEADER_NOTE_ON, 2, payload);
}

void serial_tx_note_off(uint8_t pitch) {
    uint8_t payload[1] = {pitch};
    send_serial_frame(SERIAL_HEADER_NOTE_OFF, 1, payload);
}
