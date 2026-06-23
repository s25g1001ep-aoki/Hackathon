// =============================================================================
// 【修正版】テスト6: instrument_id = 3 (リズム) バグ検出テスト
// =============================================================================

#include <avr/pgmspace.h>

struct NoteEvent {
    uint16_t start_tick;
    uint16_t end_tick;
    uint8_t pitch;
    uint8_t velocity;
};

// 実際のscore_data.hの構造を模した配列
// 1〜3小節目（0〜191tick）の間に、ハイハットやキックなどが「合計20個」あるとします。
const NoteEvent score_part_3[] PROGMEM = {
    // 1小節目 (0〜63tick) : 8個の音符
    {0,8,42,100},  {8,16,42,100}, {16,24,42,100}, {24,32,42,100},
    {32,40,42,100}, {40,48,42,100}, {48,56,42,100}, {56,64,42,100},
    
    // 2小節目 (64〜127tick) : 6個の音符
    {64,72,42,100}, {72,80,42,100}, {80,96,36,100}, 
    {96,104,42,100}, {104,112,42,100}, {112,128,38,100},

    // 3小節目 (128〜191tick) : 6個の音符 (ここまでで合計20個)
    {128,136,42,100}, {136,144,42,100}, {144,160,36,100},
    {160,168,42,100}, {168,176,42,100}, {176,192,38,100},
    
    // ==========================================
    // 【21個目の音符】4小節目（192tick）の頭で鳴るはずのキック
    // ==========================================
    {192, 208, 36, 100} 
};

// 実機コード(score_player.ino)と同じバグの数値「20」を設定
const uint8_t score_lengths = 20; 

bool note_active[25] = {false};
bool kick_4_fired = false; // 4小節目のキック(192tick)が発火したかフラグ

void serial_tx_note_on(uint8_t pitch, uint8_t velocity) {}
void serial_tx_note_off(uint8_t pitch) {}

// 実際のscore_player.inoのscore_stepと完全に同じロジック
void score_step_actual(uint16_t local_tick) {
    // ループ上限が「score_lengths (20)」になっているため、
    // 配列のi = 0 〜 19 までしかループが回りません（i = 20 の4小節目データに届かない）
    for (uint8_t i = 0; i < score_lengths; i++) {
        uint16_t start_t = pgm_read_word(&(score_part_3[i].start_tick));
        uint8_t pitch = pgm_read_byte(&(score_part_3[i].pitch));

        if (local_tick == start_t && !note_active[i]) {
            // 21番目の音符（i=20, start_tick=192, pitch=36）に到達できればここが走る
            if (local_tick == 192 && pitch == 36) {
                kick_4_fired = true;
            }
            note_active[i] = true;
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: Score Step ID=3 Bug Detect Test (Fixed) ---");

    // 配列のフラグ初期化
    for (uint8_t i = 0; i < 25; i++) { note_active[i] = false; }

    // 0 から 256 tick まで1ずつ時間を進めて再生テスト
    for (uint16_t tick = 0; tick <= 256; tick++) {
        score_step_actual(tick);
    }

    // 判定
    if (kick_4_fired) {
        Serial.println("Result: [PASS] 4小節目の音が鳴りました。");
    } else {
        // ループが20回で打ち切られるため、21個目(i=20)の音符を無視してしまい、ここを通過します。
        Serial.println("Result: [FAIL] 4小節目のキックが出ない(バグ検出)");
    }
    Serial.println("--- END: Score Step ID=3 Bug Detect Test (Fixed) ---");
}

void loop() {}