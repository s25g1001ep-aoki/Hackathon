// =============================================================================
// テスト4: serial_tx_note_on/off チェックサム検証テスト
// =============================================================================

// グローバルにパケットをキャプチャするバッファ（実送出の代わりに格納してテスト）
uint8_t tx_buffer[16];
uint8_t tx_len = 0;

// テスト用に、Serial.writeの代わりにバッファに詰めるモック関数
void mock_serial_write(uint8_t b) {
    if (tx_len < sizeof(tx_buffer)) {
        tx_buffer[tx_len++] = b;
    }
}

// NOTE_ON 関数（シミュレート版）
void test_serial_tx_note_on(uint8_t pitch, uint8_t velocity) {
    tx_len = 0; // バッファクリア
    
    uint8_t type = 0x90;
    uint8_t length = 2;
    uint8_t checksum = type ^ length ^ pitch ^ velocity;

    mock_serial_write(0xAA); // START
    mock_serial_write(type);
    mock_serial_write(length);
    mock_serial_write(pitch);
    mock_serial_write(velocity);
    mock_serial_write(checksum);
}

// NOTE_OFF 関数（シミュレート版）
void test_serial_tx_note_off(uint8_t pitch) {
    tx_len = 0; // バッファクリア
    
    uint8_t type = 0x80;
    uint8_t length = 1;
    uint8_t checksum = type ^ length ^ pitch;

    mock_serial_write(0xAA); // START
    mock_serial_write(type);
    mock_serial_write(length);
    mock_serial_write(pitch);
    mock_serial_write(checksum);
}

// 全バイト（STARTを除く）のXOR和を計算するヘルパー
uint8_t verify_checksum_zero() {
    if (tx_len <= 1) return 0xFF; // エラー
    uint8_t xor_sum = 0;
    // tx_buffer[0]は0xAAなので除外、[1]のTYPEから末尾のCHECKSUMまでXOR
    for (uint8_t i = 1; i < tx_len; i++) {
        xor_sum ^= tx_buffer[i];
    }
    return xor_sum;
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("--- START: serial_tx Checksum Test ---");

    // --- ケース1: NOTE_ON(60, 100) ---
    test_serial_tx_note_on(60, 100);
    uint8_t xor_on = verify_checksum_zero();
    
    Serial.print("NOTE_ON(60,100) Packet: ");
    for(int i=0; i<tx_len; i++) { Serial.print("0x"); Serial.print(tx_buffer[i], HEX); Serial.print(" "); }
    Serial.print("| XOR Sum (excl. 0xAA): "); Serial.print(xor_on);
    if(xor_on == 0 && tx_buffer[tx_len-1] == 0xCA) { // 0x90^0x02^0x3C^0x64 = 0xCA
        Serial.println(" -> [PASS]");
    } else {
        Serial.println(" -> [FAIL]");
    }

    // --- ケース2: NOTE_OFF(60) ---
    test_serial_tx_note_off(60);
    uint8_t xor_off = verify_checksum_zero();

    Serial.print("NOTE_OFF(60) Packet:    ");
    for(int i=0; i<tx_len; i++) { Serial.print("0x"); Serial.print(tx_buffer[i], HEX); Serial.print(" "); }
    Serial.print("| XOR Sum (excl. 0xAA): "); Serial.print(xor_off);
    if(xor_off == 0 && tx_buffer[tx_len-1] == 0xBD) { // 0x80^0x01^0x3C = 0xBD
        Serial.println(" -> [PASS]");
    } else {
        Serial.println(" -> [FAIL]");
    }

    Serial.println("--- END: serial_tx Checksum Test ---\n");
}

void loop() {
}