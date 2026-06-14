// sync_isr.ino
#include "score_data.h"

const int SYNC_PIN = 2;

//volatile uint16_t local_tick;
extern volatile bool is_playing;

void sync_init() {
    pinMode(SYNC_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SYNC_PIN), on_sync_tick, RISING);
}

void on_sync_tick() {
    if (is_playing) {
        local_tick++;
    }
}