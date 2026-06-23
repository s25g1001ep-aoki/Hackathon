// =============================================================================
// テスト13: tick飛び時の音符取りこぼし検証テスト (バグ検出)
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

// 【重要】実機のscore_data.hの後半には、奇数tickから始まる8分音符が存在します。
// テスト用に、奇数tick「15」から始まる音符を用意します。
const NoteEvent score_part_test[] PROGMEM = {
    {15, 31, 64, 100} // 奇数tickで発音
};
const uint8_t score_lengths_test = 1;

const NoteEvent* my_score = NULL;
bool note_active[1] = {false};
bool note_fired = false;

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    note_fired = true; 
}
void serial_tx_note_off(uint8_t pitch) {}

// 等号一致判定(==)を行っている現在のコードの再現
void score_step_current_logic(uint16_t local_tick) {
    uint16_t start_t = pgm_read_word(&(score_part_test[0].start_tick));
    uint8_t pitch = pgm_read_byte(&(score_part_test[0].pitch));
    uint8_t velocity = pgm_read_byte(&(score_part_test[0].velocity));

    // ★バグ懸念：完全一致で判定しているため、tickがジャンプするとすり抜ける
    if (local_tick == start_t && !note_active[0]) {
        serial_tx_note_on(pitch, velocity);
        note_active[0] = true;
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: Tick-Skip Missed Note Test ---");

    note_fired = false;
    note_active[0] = false;

    Serial.println("Simulating heavy loop delay: calling only EVEN ticks (0, 2, 4, ..., 32)");

    // 処理遅延を模擬し、偶数tickのみでタイムラインを進める (15を跨ぐ)
    for (uint16_t tick = 0; tick <= 32; tick += 2) {
        score_step_current_logic(tick);
    }

    // 判定
    if (note_fired) {
        Serial.println("Result: [PASS] 遅延が発生しても音符を取りこぼしませんでした。");
    } else {
        // tick=14 の次が tick=16 に飛ぶため、if(local_tick == 15) を通過せず
        // 音符が完全に無視されます。ここを通るのが「期待通りのバグ検出」です。
        Serial.println("Result: [FAIL] 奇数tickの音符を取りこぼしました。(等号判定バグの検出)");
    }
    
    Serial.println("--- END: Tick-Skip Missed Note Test ---\n");
}

void loop() {}