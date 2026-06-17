// spi_slave.ino
#include "score_data.h"
#include "SPI.h"

// 【配線】
// マスター側のCS(SS)ピンを、UNO R4（スレーブ）の SLAVE_CS_PIN に接続。
// MOSI/MISO/SCKは下記ピンに接続する（ソフトウェアエミュレーションのため自由に選べる）。
const int SLAVE_CS_PIN   = 10;
const int SLAVE_MOSI_PIN = 11;
const int SLAVE_MISO_PIN = 12;
const int SLAVE_SCK_PIN  = 13;

// SPI規格: 1Mbps, MSBFIRST, MODE0 (server.ino の SPI_CONFIG と同一仕様)
// MODE0 = CPOL0,CPHA0 → SCKがLOW→HIGHに上がる瞬間にデータを確定して読む(サンプリング)。
// このソフトウェアエミュレーションは MODE0 を前提に実装している。
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
//extern void score_player_on_tick(uint16_t tick);

volatile uint8_t spi_rx_buffer[sizeof(ControlCommand)];
volatile uint8_t spi_tx_buffer[sizeof(InstrumentStatus)];
volatile uint8_t ack_status = 0x01;
volatile uint8_t last_received_sequence = 0;

// 受信した5バイト(ControlCommand)を検証し、コマンドを実行する。
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
    status.ack_ok        = ack_status;
    status.checksum      = 0;

    uint8_t* raw = (uint8_t*)&status;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(InstrumentStatus) - 1; i++) sum += raw[i];
    status.checksum = (uint8_t)(0 - sum);

    memcpy((void*)spi_tx_buffer, &status, sizeof(InstrumentStatus));
}

// MOSIから1ビット読みつつ、同じタイミングでMISOに1ビット出す(全二重)。
// MODE0: SCK LOW→HIGH の立ち上がりでサンプリング、HIGH→LOW の立ち下がりで次ビット準備。
uint8_t spi_transfer_byte_slave(uint8_t tx_byte) {
    uint8_t rx_byte = 0;

    // 最初のビットはSCKが上がる前にMISOへ出しておく(MSBFIRST)
    digitalWrite(SLAVE_MISO_PIN, (tx_byte & 0x80) ? HIGH : LOW);

    for (int i = 0; i < 8; i++) {
        while (digitalRead(SLAVE_SCK_PIN) == LOW);  // SCK立ち上がりを待つ
        rx_byte |= (digitalRead(SLAVE_MOSI_PIN) << (7 - i)); // この瞬間にMOSIを確定

        // 次に出すビットを、SCKが下がる前に準備しておく
        if (i < 7) {
            digitalWrite(SLAVE_MISO_PIN, (tx_byte & (0x40 >> i)) ? HIGH : LOW);
        }

        while (digitalRead(SLAVE_SCK_PIN) == HIGH); // SCK立ち下がりを待つ
    }

    return rx_byte;
}

// マスターが通信を開始した瞬間（CSがLOWになった時）に発動する割り込み。
// ControlCommand(5バイト)を受信しつつ、同時にInstrumentStatus(5バイト)を返す。
void on_cs_falling() {
    uint8_t tx_len = sizeof(InstrumentStatus);
    uint8_t rx_len = sizeof(ControlCommand);
    uint8_t max_len = (tx_len > rx_len) ? tx_len : rx_len;

    for (uint8_t i = 0; i < max_len; i++) {
        uint8_t tx_byte = (i < tx_len) ? spi_tx_buffer[i] : 0x00;
        uint8_t rx_byte = spi_transfer_byte_slave(tx_byte);
        if (i < rx_len) {
            spi_rx_buffer[i] = rx_byte;
        }
    }

    process_received_command();
    prepare_tx_buffer();
}

void spi_setup() {
    pinMode(SLAVE_MOSI_PIN, INPUT);
    pinMode(SLAVE_MISO_PIN, OUTPUT);
    pinMode(SLAVE_SCK_PIN, INPUT);
    digitalWrite(SLAVE_MISO_PIN, LOW);
}

void init_spi_slave() {
    pinMode(SLAVE_CS_PIN, INPUT_PULLUP);

    spi_setup();
    prepare_tx_buffer();

    // CSピンがLOW（通信開始）になった瞬間に割り込み関数を起動する
    attachInterrupt(digitalPinToInterrupt(SLAVE_CS_PIN), on_cs_falling, FALLING);
}