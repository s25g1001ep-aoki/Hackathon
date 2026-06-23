// =============================================================================
// テスト8: PLAY / STOP コマンド 状態遷移テスト
// =============================================================================

// グローバル状態の定義
volatile bool is_playing = false;
bool note_active[5] = {true, false, true, false, false}; // 音符0と2が発音中と仮定
uint8_t my_score_length = 5;

int note_off_count = 0; // NOTE_OFFが呼ばれた回数をカウント

// モック関数
void serial_tx_note_off(uint8_t pitch) {
    note_off_count++;
}

// アクティブな音をすべて止める対象関数
void score_stop_all() {
    for (uint8_t i = 0; i < my_score_length; i++) {
        if (note_active[i]) {
            serial_tx_note_off(i); // テスト用にピッチの代わりにインデックスを渡す
            note_active[i] = false;
        }
    }
}

// コマンド処理のテスト用モック（擬似コマンド受信）
void simulate_process_command(uint8_t cmd_type) {
    if (cmd_type == 1) {       // 擬似 CMD_PLAY
        is_playing = true;
    } else if (cmd_type == 2) { // 擬似 CMD_STOP
        is_playing = false;
        score_stop_all();
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: PLAY / STOP Command Test ---");

    int test_pass = 0;

    // --- 1. PLAY コマンドのテスト ---
    is_playing = false;
    simulate_process_command(1); // PLAY実行
    
    Serial.print("PLAY Test: is_playing = "); Serial.print(is_playing ? "true" : "false");
    if (is_playing == true) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // --- 2. STOP コマンドのテスト ---
    note_off_count = 0; // カウントリセット
    simulate_process_command(2); // STOP実行

    Serial.print("STOP Test: is_playing = "); Serial.print(is_playing ? "true" : "false");
    if (is_playing == false) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // --- 3. 消音（score_stop_all）のテスト ---
    // 初期状態で2つの音が鳴っていたので、note_off_countが2になっていれば合格
    Serial.print("Stop All Test: note_off_count = "); Serial.print(note_off_count);
    if (note_off_count == 2) {
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    Serial.print("Result: "); Serial.print(test_pass); Serial.println("/3 Passed.");
    Serial.println("--- END: PLAY / STOP Command Test ---\n");
}

void loop() {}