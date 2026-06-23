// =============================================================================
// テスト2: pressure_get_pitch_offset 境界値テスト
// =============================================================================

const int ONSET_THRESHOLD = 200;
const int LIGHT_THRESHOLD = 400;
const int HEAVY_THRESHOLD = 700;

// テスト用：内部ロジックをシミュレートする関数
int8_t evaluate_pitch_offset(int raw_value) {
    if (raw_value < ONSET_THRESHOLD) {
        // 何も乗っていない → オフセットなし
        return 0;
    } else if (raw_value < LIGHT_THRESHOLD) {
        // 軽い → 高い音（+1オクターブ）
        return +12;
    } else if (raw_value < HEAVY_THRESHOLD) {
        // 普通 → そのまま
        return 0;
    } else {
        // 重い → 低い音（-1オクターブ）
        return -12;
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: pressure_get_pitch_offset Test ---");

    // 各境界値（下、ちょうど、上）を網羅するテストケース
    int test_cases[] = {199, 200, 399, 400, 699, 700};
    int8_t expected[]  = {  0,  12,  12,   0,   0, -12};

    int success_count = 0;
    int total_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < total_cases; i++) {
        int input = test_cases[i];
        int8_t result = evaluate_pitch_offset(input);

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

    Serial.print("Result: "); Serial.print(success_count); 
    Serial.print("/"); Serial.print(total_cases); Serial.println(" Passed.");
    Serial.println("--- END: pressure_get_pitch_offset Test ---\n");
}

void loop() {
}