// spi_slave.ino
#include "score_data.h"
#include "SPI.h"

// 【配線】UNO R4 ハードウェアSPIスレーブ
// SS=10, MOSI=11, MISO=12, SCK=13 (固定ピン、ソフトウェア指定不可)
const int SLAVE_CS_PIN = 10;
const int SLAVE_MOSI_PIN = 11;

// SPI規格: 1Mbps, MSBFIRST, MODE0 (server.ino の SPI_CONFIG と同一仕様)
// ※スレーブ動作はハードウェア側のSCK/CPOL/CPHAに追従するため
//   SPISettingsは使わないが、仕様としてここに明記する。
const unsigned long SPI_CLOCK_HZ = 1000000;
const uint8_t SPI_BIT_ORDER = MSBFIRST;
const uint8_t SPI_MODE = SPI_MODE0;

// コマンド種別 (server.ino の generate_cmd() と対応)
enum CommandType : uint8_t {
    CMD_STATUS_POLL = 0x00, // 圧力センサ監視用ポーリング
    CMD_PLAY        = 0x01,
    CMD_STOP        = 0x02,
    CMD_ENTRY_CUE   = 0x03,
    CMD_BPM_UPDATE  = 0x04
};

// server.ino の ControlCommand と完全一致させる (5バイト, packed)
struct __attribute__((packed)) ControlCommand {
    uint8_t  command_type;
    uint16_t payload;
    uint8_t  sequence;
    uint8_t  checksum;
};

// server.ino の InstrumentStatus と完全一致させる (5バイト, packed)
struct __attribute__((packed)) InstrumentStatus {
    uint8_t instrument_id;
    uint8_t frog_state;
    uint8_t sequence_ack;
    uint8_t ack_ok;
    uint8_t checksum;
};

extern volatile bool is_playing;
extern volatile uint16_t local_tick;
extern uint8_t frog_state;
extern void score_init(uint8_t instrument_id);
extern void score_stop_all();
extern uint8_t get_instrument_id();
extern void score_player_on_tick(uint16_t tick);

volatile uint8_t spi_rx_buffer[sizeof(ControlCommand)];
volatile uint8_t spi_tx_buffer[sizeof(InstrumentStatus)];
volatile uint8_t spi_byte_index = 0;
volatile uint8_t ack_status = 0x01;
volatile uint8_t last_received_sequence = 0;

// 受信した5バイト(ControlCommand)を検証し、コマンドを実行する。
// SPI割り込みの外（loop内など）で呼んでも良いように、ISR本体は最小限にする。
void process_received_command() {
    ControlCommand cmd;
    memcpy(&cmd, (const void*)spi_rx_buffer, sizeof(ControlCommand));

    // チェックサム検証 (server.ino の generate_checksum() と同方式: 全バイト総和が0)
    uint8_t* raw = (uint8_t*)&cmd;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(ControlCommand); i++) sum += raw[i];
    if (sum != 0) {
        ack_status = 0x00; // チェックサム不一致
        return;
    }

    last_received_sequence = cmd.sequence;
    ack_status = 0x01;

    switch (cmd.command_type) {
        case CMD_STATUS_POLL:
            // 圧力センサ監視用。状態更新のみ行い、再生制御はしない。
            break;
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
            // cmd.payload に新しいBPMが入る想定。BPM反映処理を実装する場合はここに追加。
            break;
        default:
            ack_status = 0x00;
            break;
    }
}

// 送信用バッファ(InstrumentStatus 5バイト)を最新の状態で作り直す。
// 次にマスターがCSをLOWにした瞬間から、この内容が1バイトずつ返される。
void prepare_tx_buffer() {
    InstrumentStatus status;
    status.instrument_id = get_instrument_id();
    status.frog_state    = frog_state;
    status.sequence_ack  = last_received_sequence;
    status.ack_ok         = ack_status;
    status.checksum       = 0;

    uint8_t* raw = (uint8_t*)&status;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(InstrumentStatus) - 1; i++) sum += raw[i];
    status.checksum = (uint8_t)(0 - sum);

    memcpy((void*)spi_tx_buffer, &status, sizeof(InstrumentStatus));
}

// SPIハードウェア受信完了割り込み: 1バイト受信されるたびに呼ばれる。
// この中では重い処理をしない（次の送信バイトのセットのみを最優先で行う）。
ISR(SPI_STC_vect) {
    uint8_t rx = SPDR; // 受信バイトを読む(これがMISOラインを次バイト用に解放する)

    if (spi_byte_index < sizeof(ControlCommand)) {
        spi_rx_buffer[spi_byte_index] = rx;
    }

    spi_byte_index++;

    // 次に送るバイトを即座にSPDRへセットする
    if (spi_byte_index < sizeof(InstrumentStatus)) {
        SPDR = spi_tx_buffer[spi_byte_index];
    }

    // ControlCommand(5バイト)を受信し終えたらコマンド処理＋送信バッファ更新
    if (spi_byte_index >= sizeof(ControlCommand)) {
        process_received_command();
        prepare_tx_buffer();
    }
}

// マスターがCSをLOWにした瞬間（通信開始）にバイトインデックスをリセットし、
// 先頭バイト(instrument_id)を送信レジスタに事前セットする。
void on_ss_falling() {
    spi_byte_index = 0;
    SPDR = spi_tx_buffer[0];
}

void spi_setup() {
    pinMode(SLAVE_MOSI_PIN, INPUT);
    pinMode(MISO, OUTPUT);  // スレーブはMISOのみ出力
    SPCR |= _BV(SPE);       // ハードウェアSPIスレーブモード有効化
    SPI.attachInterrupt();  // SPI_STC_vect 割り込みを有効化
}

void init_spi_slave() {
    pinMode(SLAVE_CS_PIN, INPUT_PULLUP);

    prepare_tx_buffer();
    spi_setup();
    SPDR = spi_tx_buffer[0]; // 最初の送信バイトを事前セット

    // CSピン(SS)がLOWになった瞬間にバイトインデックスをリセット
    attachInterrupt(digitalPinToInterrupt(SLAVE_CS_PIN), on_ss_falling, FALLING);
}