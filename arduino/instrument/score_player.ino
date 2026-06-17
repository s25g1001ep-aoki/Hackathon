// score_player.ino
#include "score_data.h"
#include <avr/pgmspace.h>

#define DRUM_KICK  36
#define DRUM_SNARE 38
#define DRUM_HH    42


const uint8_t score_lengths[4] = {29, 29, 29, 20};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;

static const uint8_t MAX_NOTES = 29;
bool note_active[MAX_NOTES] = {false};

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

    for (uint8_t i = 0; i < my_score_length; i++) {
        note_active[i] = false;
    }
}

void score_step(uint16_t local_tick, int8_t pitch_offset) {  //引数追加
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint16_t end_t = pgm_read_word(&(my_score[i].end_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        //追加部：オフセットを加算（0〜127の範囲にクランプ）
        int16_t shifted_pitch = (int16_t)pitch + pitch_offset;
        if (shifted_pitch < 0)   shifted_pitch = 0;
        if (shifted_pitch > 127) shifted_pitch = 127;

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

void score_loop_check(volatile uint16_t &local_tick) {
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