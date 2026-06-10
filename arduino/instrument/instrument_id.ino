// instrument_id.ino
#include <EEPROM.h>

const int EEPROM_ID_ADDR = 0x00;
const int ERROR_LED_PIN = 13;

uint8_t instrument_id = 0xFF;

uint8_t eeprom_read_instrument_id() {
    uint8_t id = EEPROM.read(EEPROM_ID_ADDR);
    if (id >= 4) {
        return 0xFF;
    }
    return id;
}

void instrument_id_init() {
    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, LOW);

    instrument_id = eeprom_read_instrument_id();

    if (instrument_id == 0xFF) {
        digitalWrite(ERROR_LED_PIN, HIGH);
        while (true) {
            delay(1000);
        }
    }
}

uint8_t get_instrument_id() {
    return instrument_id;
}