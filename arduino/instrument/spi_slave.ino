// spi_slave.ino
#include "score_data.h"

const int SS_PIN = 10;

// マスター側と完全に一致させるための列挙型・構造体定義
enum ControlCommand : uint8_t {
    CMD_PLAY = 0x01,
    CMD_STOP = 0x02,
    CMD_ENTRY_CUE = 0x03,
    CMD_BPM_UPDATE = 0x04
};

// マスター側の InstrumentStatus の構造に合わせる（想定される配置の例）
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

// 送受信バッファをマスターのサイズに合わせる
volatile uint8_t spi_rx_buffer[sizeof(ControlCommand)];
volatile InstrumentStatus spi_tx_buffer;
volatile uint8_t spi_index = 0;
volatile uint8_t last_sequence = 0;
volatile uint8_t ack_status = 0x01; // 0x01: OK, 0x00: NG

void prepare_tx_buffer() {
    // マスターの InstrumentStatus 構造体の並び順にそのまま代入
    spi_tx_buffer.instrument_id = get_instrument_id();
    spi_tx_buffer.frog_state    = frog_state;
    spi_tx_buffer.last_sequence = last_sequence;
    spi_tx_buffer.ack_status    = ack_status;
}

void init_spi_slave() {
    pinMode(MISO, OUTPUT);
    pinMode(MOSI, INPUT);
    pinMode(SCK, INPUT);
    pinMode(SS_PIN, INPUT);

    prepare_tx_buffer();
    // 最初の1バイト（instrument_id）をSPDRに仕込んでおく
    SPDR = ((uint8_t*)&spi_tx_buffer)[0];

    SPCR |= _BV(SPE);
    SPCR |= _BV(SPIE);
}

ISR(SPI_STC_vect) {
    uint8_t rx_byte = SPDR;

    // バッファオーバーフロー防止
    if (spi_index < sizeof(ControlCommand)) {
        spi_rx_buffer[spi_index] = rx_byte;
    }
    spi_index++;

    // マスターからの期待バイト数（ControlCommandのサイズ）に達したか確認
    if (spi_index >= sizeof(ControlCommand)) {
        
        // --- データ解析と処理 ---
        // マスターが単一バイトのenum、または最初のバイトにコマンドを入れていると仮定
        uint8_t command = spi_rx_buffer[0]; 
        
        // 必要に応じてマスター側からシーケンス番号を受け取る場合はここで処理
        // last_sequence = spi_rx_buffer[1]; // 例

        ack_status = 0x01; // 正常受信

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
                ack_status = 0x00; // 未定義コマンド
                break;
        }

        // 次回のトランザクション（SPI通信）に向けた送信データの更新
        prepare_tx_buffer();
        spi_index = 0;
        SPDR = ((uint8_t*)&spi_tx_buffer)[0]; // 次の最初の1バイトを準備
    } else {
        // 次のバイトをSPDRに仕込む（InstrumentStatusの次のバイト）
        if (spi_index < sizeof(InstrumentStatus)) {
            SPDR = ((uint8_t*)&spi_tx_buffer)[spi_index];
        } else {
            SPDR = 0x00; // 送信データが足りない場合のダミー
        }
    }
}