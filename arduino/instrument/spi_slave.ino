// spi_slave.ino
#include "score_data.h"

// 【配線の変更点】
// マスター側のCS(SS)ピンを、UNO R4（スレーブ）の「2番ピン（または3番ピン）」に接続してください。
// MOSIピンは通常通り11番ピン（または任意のデジタルピン）に接続します。
const int SLAVE_CS_PIN = 2;  
const int SLAVE_MOSI_PIN = 11;

enum ControlCommand : uint8_t {
    CMD_PLAY = 0x01,
    CMD_STOP = 0x02,
    CMD_ENTRY_CUE = 0x03,
    CMD_BPM_UPDATE = 0x04
};

struct InstrumentStatus {
    uint8_t instrument_id;
    uint8_t frog_state;
    uint8_t last_sequence;
    uint8_t ack_status;
};

extern volatile bool is_playing;
extern volatile uint16_t local_tick;
extern uint8_t frog_state;
extern void score_init(uint8_t instrument_id);
extern void score_stop_all();
extern uint8_t get_instrument_id();

volatile uint8_t spi_rx_buffer[sizeof(ControlCommand)];
volatile InstrumentStatus spi_tx_buffer;
volatile uint8_t ack_status = 0x01;

void prepare_tx_buffer() {
    spi_tx_buffer.instrument_id = get_instrument_id();
    spi_tx_buffer.frog_state    = frog_state;
    spi_tx_buffer.last_sequence = 0;
    spi_tx_buffer.ack_status    = ack_status;
}

// マスターが通信を開始した瞬間（CSがLOWになった時）に発動する高速割り込み
void instance_spi_emulate_isr() {
    // マスターが1バイト（8ビット）送る波形をソフトウェアで超高速キャプチャ
    uint8_t received_byte = 0;
    
    // マスターのSCK（13番ピン）の立ち上がりに同期してMOSIのデータを8回読む
    // ※ハッカソン本番で動かす際、もしデータがズレる場合はここを調整
    for (int i = 0; i < 8; i++) {
        while (digitalRead(13) == LOW); // クロックが上がるのを待つ
        received_byte |= (digitalRead(SLAVE_MOSI_PIN) << (7 - i));
        while (digitalRead(13) == HIGH); // クロックが下がるのを待つ
    }

    uint8_t command = received_byte;
    ack_status = 0x01;

    switch (command) {
        case CMD_PLAY:
            is_playing = true;
            break;
        case CMD_STOP:
            is_playing = false;
            score_stop_all();
            break;
        case CMD_ENTRY_CUE:
            local_tick = 0;
            score_init(get_instrument_id());
            is_playing = true;
            break;
        case CMD_BPM_UPDATE:
            break;
        default:
            ack_status = 0x00;
            break;
    }

    prepare_tx_buffer();
}

void init_spi_slave() {
    pinMode(SLAVE_MOSI_PIN, INPUT);
    pinMode(13, INPUT); // SCKピンを監視
    pinMode(SLAVE_CS_PIN, INPUT_PULLUP);

    prepare_tx_buffer();

    // CSピンがLOW（通信開始）になった瞬間に割り込み関数を起動する
    attachInterrupt(digitalPinToInterrupt(SLAVE_CS_PIN), instance_spi_emulate_isr, FALLING);
}