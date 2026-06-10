// score_player.ino
#include "score_data.h"
#include <avr/pgmspace.h>

//この辺は後で譜面をもらって変更する
const NoteEvent score_part_0[] PROGMEM = {
    {0, 8, 60, 100}, {16, 24, 64, 100}, {32, 40, 67, 100}, {48, 56, 72, 100}
};
const NoteEvent score_part_1[] PROGMEM = {
    {0, 12, 64, 90}, {16, 28, 67, 90}, {32, 44, 72, 90}, {48, 60, 76, 90}
};
const NoteEvent score_part_2[] PROGMEM = {
    {0, 16, 67, 85}, {16, 32, 72, 85}, {32, 48, 76, 85}, {48, 64, 79, 85}
};
const NoteEvent score_part_3[] PROGMEM = {
    {0, 32, 48, 95}, {32, 64, 55, 95}
};

const uint8_t score_lengths[] = {
    sizeof(score_part_0) / sizeof(NoteEvent),
    sizeof(score_part_1) / sizeof(NoteEvent),
    sizeof(score_part_2) / sizeof(NoteEvent),
    sizeof(score_part_3) / sizeof(NoteEvent)
};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;
bool note_active[16] = {false};

extern void serial_tx_note_on(uint8_t pitch, uint8_t velocity);
extern void serial_tx_note_off(uint8_t pitch);

void score_init(uint8_t instrument_id) {
    switch (instrument_id) {
        case 0: my_score = score_part_0; break;
        case 1: my_score = score_part_1; break;
        case 2: my_score = score_part_2; break;
        case 3: my_score = score_part_3; break;
        default: my_score = score_part_0; break;
    }
    my_score_length = score_lengths[instrument_id];

    for (uint8_t i = 0; i < 16; i++) {
        note_active[i] = false;
    }
}

void score_step(uint16_t local_tick) {
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint16_t end_t = pgm_read_word(&(my_score[i].end_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        if (local_tick == start_t && !note_active[i]) {
            serial_tx_note_on(pitch, velocity);
            note_active[i] = true;
        }

        if (local_tick == end_t && note_active[i]) {
            serial_tx_note_off(pitch);
            note_active[i] = false;
        }
    }
}

void score_loop_check(uint16_t &local_tick) {
    if (local_tick >= LOOP_MAX_TICK) {
        noInterrupts();
        local_tick = 0;
        interrupts();

        for (uint8_t i = 0; i < my_score_length; i++) {
            note_active[i] = false;
        }
    }
}

void score_stop_all() {
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        if (note_active[i]) {
            uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
            serial_tx_note_off(pitch);
            note_active[i] = false;
        }
    }
}