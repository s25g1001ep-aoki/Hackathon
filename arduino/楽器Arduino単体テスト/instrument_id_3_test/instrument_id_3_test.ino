// =============================================================================
// テスト7: ピッチシフト バグ検出テスト
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

const NoteEvent score_part_0[] PROGMEM = {
    {0,  16,  60, 100} // 元の音は ド(60)
};
const uint8_t score_lengths[4] = {1, 0, 0, 0};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;
bool note_active[1] = {false};
uint8_t last_fired_pitch = 0;

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    last_fired_pitch = pitch; // 送信されたピッチを記録
}
void serial_tx_note_off(uint8_t pitch) {}

void score_init(uint8_t instrument_id) {
    my_score = score_part_0;
    my_score_length = score_lengths[instrument_id];
    note_active[0] = false;
}

// 現状のscore_stepの再現
void score_step(uint16_t local_tick, int8_t pitch_offset) {
    if (my_score == NULL) return;
    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint16_t end_t = pgm_read_word(&(my_score[i].end_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        // 計算はしているが...
        int16_t shifted_pitch = (int16_t)pitch + pitch_offset;
        if (shifted_pitch < 0)   shifted_pitch = 0;
        if (shifted_pitch > 127) shifted_pitch = 127;

        if (local_tick == start_t && !note_active[i]) {
            // ★バグ：引数に shifted_pitch ではなく、元の pitch を渡している
            serial_tx_note_on(pitch, velocity); 
            note_active[i] = true;
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: Pitch Shift Bug Detect Test ---");

    score_init(0);
    
    // オフセット +12 (1オクターブ高く) を指定して実行
    int8_t test_offset = 12; 
    score_step(0, test_offset);

    Serial.print("Input Base Pitch: 60 | Offset: +12\n");
    Serial.print("Expected Pitch: 72   | Actual Output: "); Serial.println(last_fired_pitch);

    if (last_fired_pitch == 72) {
        Serial.println("Result: [PASS] ピッチシフトが正常に反映されています。");
    } else if (last_fired_pitch == 60) {
        // シフトが反映されず元の音が鳴るため、ここを通過します。
        Serial.println("Result: [FAIL] ピッチが変わっていない(バグ検出)");
    } else {
        Serial.println("Result: [ERROR] 想定外の挙動");
    }
    Serial.println("--- END: Pitch Shift Bug Detect Test ---");
}

void loop() {}