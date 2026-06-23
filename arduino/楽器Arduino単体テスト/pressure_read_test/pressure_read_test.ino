// =============================================================================
// テスト1: pressure_read 境界値テスト
// =============================================================================

const int ONSET_THRESHOLD = 200;

// テスト用：引数から値を評価する関数に分離
uint8_t evaluate_pressure(int raw_value) {
    uint8_t frog_state = 0;
    if (raw_value > ONSET_THRESHOLD) {
        frog_state = 1;
    } else {
        frog_state = 0;
    }
    return frog_state;
}

void setup() {
    Serial.begin(9600);
    while (!Serial); // シリアルモニターの起動を待つ
    Serial.println("--- START: pressure_read Test ---");

    int test_cases[] = {199, 200, 201};
    uint8_t expected[] = {0, 0, 1}; // 200より大きい場合のみ1

    int success_count = 0;

    for (int i = 0; i < 3; i++) {
        int input = test_cases[i];
        uint8_t result = evaluate_pressure(input);
        
        Serial.print("Input: "); Serial.print(input);
        Serial.print(" | Expected: "); Serial.print(expected[i]);
        Serial.print(" | Result: "); Serial.print(result);

        if (result == expected[i]) {
            Serial.println(" -> [PASS]");
            success_count++;
        } else {
            Serial.println(" -> [FAIL]");
        }
    }

    Serial.print("Result: "); Serial.print(success_count); Serial.println("/3 Passed.");
    Serial.println("--- END: pressure_read Test ---\n");
}

void loop() {
    // テストは一回のみ実行
}