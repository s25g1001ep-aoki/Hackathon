// sync_isr.ino
#include "score_data.h"

const int SYNC_PIN = 9;

extern volatile bool is_playing;
extern volatile uint16_t local_tick;
extern void score_player_on_tick(uint16_t tick);

void sync_init() {
    pinMode(SYNC_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SYNC_PIN), on_sync_tick, RISING);
}

// server.ino が global_tick++ のたびに CS_SYNC を HIGH パルスとして出力する。
// その立ち上がりに同期して tick を進め、そのtickに該当する音を鳴らす。
void on_sync_tick() {
    if (is_playing) {
        local_tick++;
        score_player_on_tick(local_tick);
    }
}