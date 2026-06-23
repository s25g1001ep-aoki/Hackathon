// =============================================================================
// テスト12: 二重 NOTE_ON 防止テスト
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

const NoteEvent score_part_0[] PROGMEM = {
    {10, 26, 60, 100} // tick=10 で発音開始する音
};
const uint8_t score_lengths[4] = {1, 0, 0, 0};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;
bool note_active[1] = {false};

int note_on_call_count = 0;

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    note_on_call_count++; // 発火回数をカウント
}
void serial_tx_note_off(uint8_t pitch) {}

void score_init(uint8_t instrument_id) {
    my_score = score_part_0;
    my_score_length = score_lengths[instrument_id];
    note_active[0] = false;
}

void score_step(uint16_t local_tick) {
    if (my_score == NULL) return;
    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        if (local_tick == start_t && !note_active[i]) {
            serial_tx_note_on(pitch, velocity);
            note_active[i] = true;
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: Duplicate NOTE_ON Prevention Test ---");

    score_init(0);
    note_on_call_count = 0;

    // 同一の開始タイミング (tick=10) で関数を「3回」連続で叩いてみる
    score_step(10); 
    score_step(10); 
    score_step(10); 

    Serial.print("Triggered score_step(10) 3 times. NOTE_ON count: ");
    Serial.print(note_on_call_count);

    // 内部フラグが効いていれば、3回呼んでも最初の1回しかNOTE_ONは出ないはず
    if (note_on_call_count == 1) {
        Serial.println(" -> [PASS] (二重発火が正しく防止されました)");
    } else {
        Serial.println(" -> [FAIL] (二重発火してしまっています！)");
    }

    Serial.println("--- END: Duplicate NOTE_ON Prevention Test ---\n");
}

void loop() {}