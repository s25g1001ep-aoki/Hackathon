// =============================================================================
// テスト10: on_sync_tick 割り込み・条件テスト
// =============================================================================

const int SYNC_PIN = 2;       // 割り込み入力ピン (D2)
const int PULSE_GEN_PIN = 3;  // 擬似パルス出力ピン (D3) -> D2と結線してください

volatile bool is_playing = false;
volatile uint16_t local_tick = 0;

// 割り込みハンドラ (sync_isr.ino の実装通り)
void on_sync_tick() {
    if (is_playing) {
        local_tick++;
    }
}

// 1パルスをD3から出力してD2の割り込みを起こすヘルパー関数
void trigger_sync_pulse() {
    digitalWrite(PULSE_GEN_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(PULSE_GEN_PIN, HIGH); // 立ち上がりエッジ発生
    delayMicroseconds(10);
    digitalWrite(PULSE_GEN_PIN, LOW);
    delay(5); // 割り込み処理を待つための僅かなディレイ
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: on_sync_tick ISR Test ---");
    Serial.println("注意: テストを実行するには [D2ピン] と [D3ピン] をジャンパ線で接続してください。");
    delay(2000); // 接続を待つための猶予

    pinMode(PULSE_GEN_PIN, OUTPUT);
    digitalWrite(PULSE_GEN_PIN, LOW);

    // 2番ピンの立ち上がりエッジ割り込みを設定
    pinMode(SYNC_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SYNC_PIN), on_sync_tick, RISING);

    int test_pass = 0;

    // --- 検証1: is_playing = false のとき、tickは増えないか ---
    is_playing = false;
    local_tick = 10;
    
    trigger_sync_pulse(); // 1回目パルス
    trigger_sync_pulse(); // 2回目パルス

    Serial.print("is_playing=false Test: local_tick = "); Serial.print(local_tick);
    if (local_tick == 10) { // パルスを入れても10のままならPASS
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    // --- 検証2: is_playing = true のとき、パルスに応じてtickが増えるか ---
    is_playing = true;
    
    trigger_sync_pulse(); // 3回目パルス (ここで+1されるはず)
    trigger_sync_pulse(); // 4回目パルス (ここで+1されるはず)

    Serial.print("is_playing=true  Test: local_tick = "); Serial.print(local_tick);
    if (local_tick == 12) { // 10から2回パルスを受けて12になっていればPASS
        Serial.println(" -> [PASS]");
        test_pass++;
    } else {
        Serial.println(" -> [FAIL]");
    }

    Serial.print("Result: "); Serial.print(test_pass); Serial.println("/2 Passed.");
    Serial.println("--- END: on_sync_tick ISR Test ---\n");
}

void loop() {}