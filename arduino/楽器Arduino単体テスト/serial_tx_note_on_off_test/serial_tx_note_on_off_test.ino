// =============================================================================
// テスト5: instrument_id = 0 (カエルの歌) 時系列テスト
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

// 擬似的にscore_data.hの一部（最初の数音とゲゲゲゲ地帯）を定義
const NoteEvent score_part_0[] PROGMEM = {
    {0,  16,  60, 100}, // ド
    {16,  32,  62, 100}, // レ
    {32,  48,  64, 100}, // ミ
    {48,  64,  65, 100}, // ファ
    {64,  80,  64, 100}, // ミ
    // (中略) テスト用に配列の一部のみ再現
    {384, 392, 60, 100}, {392, 400, 60, 100}  // ゲゲゲゲの一部
};
const uint8_t score_lengths[4] = {6, 0, 0, 0}; // テスト用に要素数を調整

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;
bool note_active[10] = {false};

// シリアル送信のモック（モニターへのログ出力）
void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    Serial.print(" -> [NOTE_ON]  Pitch: "); Serial.print(pitch);
    Serial.print(", Vel: "); Serial.println(velocity);
}
void serial_tx_note_off(uint8_t pitch) {
    Serial.print(" -> [NOTE_OFF] Pitch: "); Serial.println(pitch);
}

void score_init(uint8_t instrument_id) {
    my_score = score_part_0;
    my_score_length = score_lengths[instrument_id];
    for (uint8_t i = 0; i < my_score_length; i++) { note_active[i] = false; }
}

void score_step(uint16_t local_tick, int8_t pitch_offset) {
    if (my_score == NULL) return;
    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint16_t end_t = pgm_read_word(&(my_score[i].end_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        if (local_tick == start_t && !note_active[i]) {
            Serial.print("Tick "); Serial.print(local_tick);
            serial_tx_note_on(pitch, velocity);
            note_active[i] = true;
        }
        if (local_tick == end_t && note_active[i]) {
            Serial.print("Tick "); Serial.print(local_tick);
            serial_tx_note_off(pitch);
            note_active[i] = false;
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: Score Step ID=0 Test ---");

    score_init(0);

    // 0 から 512 tick までクロックを擬似的に進める
    for (uint16_t tick = 0; tick <= 512; tick++) {
        score_step(tick, 0);
    }

    Serial.println("--- END: Score Step ID=0 Test ---");
}

void loop() {}