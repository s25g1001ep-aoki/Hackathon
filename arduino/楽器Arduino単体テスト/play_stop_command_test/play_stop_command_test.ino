// =============================================================================
// テスト9: ENTRY_CUE コマンド 初期化テスト
// =============================================================================

volatile bool is_playing = false;
volatile uint16_t local_tick = 0;
bool note_active[5] = {true, true, false, true, false}; // 途中の発音状態
uint8_t my_score_length = 5;

// score_initの簡易再現
void score_init() {
    for (uint8_t i = 0; i < my_score_length; i++) {
        note_active[i] = false; // すべて初期化
    }
}

// ENTRY_CUE受信時の処理をシミュレート
void simulate_entry_cue() {
    local_tick = 0;  // 譜面先頭へリセット
    score_init();    // フラグ初期化
    is_playing = true; // 再生開始
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: ENTRY_CUE Command Test ---");

    // テスト前の状態設定 (曲の途中をシミュレート)
    local_tick = 240; 
    is_playing = false;

    // ENTRY_CUE 実行
    simulate_entry_cue();

    int test_pass = 0;

    // 検証1: local_tickが0になっているか
    Serial.print("local_tick reset: "); Serial.print(local_tick);
    if (local_tick == 0) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // 検証2: is_playingがtrueになっているか
    Serial.print("is_playing state: "); Serial.print(is_playing ? "true" : "false");
    if (is_playing == true) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // 検証3: note_activeフラグがすべてfalseに戻っているか
    bool flags_cleared = true;
    for(int i=0; i<my_score_length; i++) {
        if(note_active[i] == true) flags_cleared = false;
    }
    Serial.print("note_active cleared: "); Serial.print(flags_cleared ? "true" : "false");
    if (flags_cleared) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    Serial.print("Result: "); Serial.print(test_pass); Serial.println("/3 Passed.");
    Serial.println("--- END: ENTRY_CUE Command Test ---\n");
}

void loop() {}