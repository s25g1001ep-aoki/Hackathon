// score_data.h
#ifndef SCORE_DATA_H
#define SCORE_DATA_H

#include <Arduino.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

const uint8_t TOTAL_PARTS = 4;
const uint16_t LOOP_MAX_TICK = 64;

#endif