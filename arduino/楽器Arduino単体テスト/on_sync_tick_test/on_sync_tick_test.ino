// =============================================================================
// テスト11: NOTE_ON / NOTE_OFF ペア不変条件テスト
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

// 検証用のシンプルな譜面（カエルの歌の冒頭2音を模擬）
const NoteEvent score_part_0[] PROGMEM = {
    {0,  16,  60, 100}, // ド
    {16,  32,  62, 100}  // レ
};
const uint8_t score_lengths[4] = {2, 0, 0, 0};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;
bool note_active[2] = {false};

int note_on_count = 0;
int note_off_count = 0;

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    note_on_count++;
}
void serial_tx_note_off(uint8_t pitch) {
    note_off_count++;
}

void score_init(uint8_t instrument_id) {
    my_score = score_part_0;
    my_score_length = score_lengths[instrument_id];
    for (uint8_t i = 0; i < my_score_length; i++) { note_active[i] = false; }
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

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: NOTE_ON/OFF Pair Invariant Test ---");

    score_init(0);
    note_on_count = 0;
    note_off_count = 0;

    // 1ループ分 (0〜512tick) 完全に回す
    for (uint16_t tick = 0; tick <= 512; tick++) {
        score_step(tick);
    }

    int test_pass = 0;

    // 検証1: ONとOFFのトータル回数が一致しているか
    Serial.print("Count Check: NOTE_ON="); Serial.print(note_on_count);
    Serial.print(", NOTE_OFF="); Serial.print(note_off_count);
    if (note_on_count == note_off_count && note_on_count > 0) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // 検証2: ループ終了後に鳴りっぱなし（activeのまま）の音符がないか
    bool remaining_active = false;
    for (uint8_t i = 0; i < my_score_length; i++) {
        if (note_active[i]) remaining_active = true;
    }
    Serial.print("Stuck Note Check: ");
    if (!remaining_active) {
        Serial.println("No stuck notes -> [PASS]");
        test_pass++;
    } else {
        Serial.println("Found stuck notes! -> [FAIL]");
    }

    Serial.print("Result: "); Serial.print(test_pass); Serial.println("/2 Passed.");
    Serial.println("--- END: NOTE_ON/OFF Pair Invariant Test ---\n");
}

void loop() {}