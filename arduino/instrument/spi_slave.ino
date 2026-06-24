// spi_slave.ino
// 【役割】サーバArduino(SPIマスタ)とのSPI通信をスレーブ側として処理するファイル。
// instrument_spi_fix_notes.md の指摘1・2・4を反映し、指摘3(ハードウェアSPI Slave化)は
// 使用マイコン(Arduino UNO R4 WiFi / Renesas RA4M1)の制約上不可能なため対象外とする。
#include "score_data.h"
#include "SPI.h"

// ============================================================================
// 【マイコン制約に関する注記】
// UNO R4 WiFi (Renesas RA4M1, Arm Cortex-M4) は、Arduino核心の SPI ライブラリで
// ハードウェアSPIスレーブ動作をサポートしていない(SPIライブラリはマスター専用)。
// そのため、本ファイルは digitalRead/digitalWrite によるソフトウェアSPIエミュレー
// ションでスレーブ動作を実現する。
// ============================================================================

// ============================================================================
// 【通信プロトコルの要点】
// server.ino の spi_send() は、1回のCS LOW区間内で次の順に合計7バイトの
// クロックを出す(CS回数は1回のまま、通信回数を増やさない設計)。
//
//   1バイト目: ControlCommand.command_type 送信  ←→ InstrumentStatus.instrument_id 受信
//   2バイト目: ControlCommand.payload(下位)送信   ←→ InstrumentStatus.frog_state   受信
//   3バイト目: ControlCommand.payload(上位)送信   ←→ InstrumentStatus.sequence_ack 受信
//   4バイト目: ControlCommand.sequence    送信   ←→ InstrumentStatus.ack_ok(暫定) 受信
//   5バイト目: ControlCommand.checksum    送信   ←→ InstrumentStatus.checksum(暫定)受信
//   --- delay(1)地点(マスターはクロックを止める) ---
//   6バイト目: ダミー(DUMMY_ACK)送信              ←→ InstrumentStatus.ack_ok(確定) 受信
//   7バイト目: ダミー(DUMMY_SUM)送信              ←→ InstrumentStatus.checksum(確定)受信
//
// 【重要な制約】3バイト目の交換タイミングでは、まだ4バイト目(sequence)を
// 受信していないため、sequence_ack に「今回受信するsequence」を載せることは
// 原理的に不可能。そのため sequence_ack は「前回受信に成功したsequence」を
// 返す設計とする(サーバー側 verification_status() の sequence_ack 比較ロジック
// は変更しない前提のため、スレーブ側がこの制約を吸収する)。
// 一方 ack_ok と checksum は、5バイト目交換後の delay(1) の間にチェックサム
// 検証を完了させ、6,7バイト目で「今回受信したコマンドへの正しい検証結果」を
// 返すことができる。
// ============================================================================

// 【配線】マスター側のCS(SS)ピンを、UNO R4（スレーブ）の SLAVE_CS_PIN に接続。
// MOSI/MISO/SCKは下記ピンに接続する（ソフトウェアエミュレーションのため自由に選べる）。
const int SLAVE_CS_PIN   = 10;
const int SLAVE_MOSI_PIN = 11;
const int SLAVE_MISO_PIN = 12;
const int SLAVE_SCK_PIN  = 13;

// SPI規格: 1Mbps, MSBFIRST, MODE0 (server.ino の SPI_CONFIG と同一仕様)
// 【指摘3対応(可能な範囲)】ハードウェアSPI Slave化は本マイコンでは不可能なため、
// クロックを落として安定性を確保する対応にとどめる。通信が不安定な場合は、
// この値とサーバー側のSPI_CONFIGを一緒に下げて(例: 250000)動作確認すること。
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

// server.ino の wait_ack() ハンドシェイクと対応する値
// (server.ino: CMD_CONNECT=100, DUMMY=0x00, ACK_OK=200)
const uint8_t CMD_CONNECT = 100;
const uint8_t ACK_OK      = 200;

// SCKエッジ待ちのタイムアウト(マイクロ秒)。
// server.ino はバイト間に delay(1)(=1000us) を挟むため、タイムアウトは
// それより十分大きく取る必要がある。
const unsigned long SCK_EDGE_TIMEOUT_US = 5000UL;

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

// 受信中のControlCommandを一時的に保持するバッファ(割り込み内でのみ使用)
volatile uint8_t spi_rx_buffer[sizeof(ControlCommand)];

// 送信するInstrumentStatusのバッファ。通信中に随時更新される。
// [0]=instrument_id [1]=frog_state [2]=sequence_ack(前回値) [3]=ack_ok [4]=checksum
volatile uint8_t spi_tx_buffer[sizeof(InstrumentStatus)];

// 直前に正常受信できたControlCommandのsequence。
// 3バイト目交換のタイミングでは今回のsequenceがまだ受信できていないため、
// この「前回値」をsequence_ackとして返す。
volatile uint8_t last_acked_sequence = 0;

// 【指摘4対応】SPI割り込み内では「どのコマンドが来たか」を記録するだけにし、
// score_init/score_stop_allなどの重い処理は loop() 側の process_pending_command()
// で実行する。これにより割り込みハンドラの実行時間を最小化し、他の割り込み
// (SYNC tick等)への影響を抑える。
volatile bool has_pending_command = false;
volatile uint8_t pending_command_type = 0;
volatile uint16_t pending_payload = 0;

// ----------------------------------------------------------------------------
// loop() 側から毎周期呼ばれる、保留中コマンドの実処理。
// 【指摘4対応】score_init/score_stop_allなど時間のかかる処理はここに集約し、
// 割り込みハンドラ(on_cs_falling)からは絶対に呼ばない。
// ----------------------------------------------------------------------------
void process_pending_command() {
    if (!has_pending_command) return;

    uint8_t command_type;
    uint16_t payload;

    // 割り込みと競合しないよう、値を取り出す間だけ割り込みを止める
    noInterrupts();
    command_type = pending_command_type;
    payload = pending_payload;
    has_pending_command = false;
    interrupts();

    switch (command_type) {
        case CMD_STATUS_POLL:
            // 圧力センサ監視用ポーリング。状態更新のみで、再生制御は行わない。
            break;
        case CMD_PLAY:
            is_playing = true;
            break;
        case CMD_STOP:
            is_playing = false;
            score_stop_all(); // アクティブな全音符にNOTE_OFFを発行して停止
            break;
        case CMD_ENTRY_CUE:
            noInterrupts();
            local_tick = 0; // 自パート譜面の先頭から演奏を開始するためリセット
            interrupts();
            score_init(get_instrument_id());
            is_playing = true;
            break;
        case CMD_BPM_UPDATE:
            // payload に新しいBPMが入る想定。
            // 楽器側はBPM値そのものを保持しない設計のため、現状は受信のみ。
            (void)payload;
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------
// 通信開始前(CSがLOWになる前)に、送信バッファの先頭3バイトを確定させる。
// instrument_id・frog_state・sequence_ack(前回値)は、今回のControlCommand
// を受信するより前に送信され始めるため、事前に用意しておく必要がある。
// ----------------------------------------------------------------------------
void prepare_tx_buffer_head() {
    spi_tx_buffer[0] = get_instrument_id();
    spi_tx_buffer[1] = frog_state;
    spi_tx_buffer[2] = last_acked_sequence; // 前回受理したsequence(今回分はまだ不明)
}

// ----------------------------------------------------------------------------
// ControlCommand(5バイト)のチェックサムを検証し、ack_ok・checksumを確定する。
// server.ino の delay(1) の間(マスターがクロックを止めている間)に呼ばれる。
// 検証に成功した場合のみ、今回のsequenceをlast_acked_sequenceとして更新し、
// 次回の通信で返すsequence_ackに反映させる。
// ----------------------------------------------------------------------------
void finalize_tx_buffer_tail(const uint8_t* raw_rx) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(ControlCommand); i++) sum += raw_rx[i];
    bool checksum_ok = (sum == 0);

    spi_tx_buffer[3] = checksum_ok ? 0x01 : 0x00; // ack_ok: 今回コマンドの検証結果

    if (checksum_ok) {
        ControlCommand cmd;
        memcpy(&cmd, raw_rx, sizeof(ControlCommand));

        // 次回の通信でsequence_ackとして返すため記録する(今回は間に合わない)
        last_acked_sequence = cmd.sequence;

        // 実コマンド処理はloop()側へ予約するだけ(指摘4対応、重い処理を割り込み外に出す)
        pending_command_type = cmd.command_type;
        pending_payload = cmd.payload;
        has_pending_command = true;
    }
    // checksum不一致の場合はlast_acked_sequenceを更新しない
    // (= 次回もまだ「直前に成功した値」を返し続ける)

    // InstrumentStatus全体(checksum以外の4バイト)の和から、2の補数チェックサムを計算
    uint8_t out_sum = 0;
    for (uint8_t i = 0; i < sizeof(InstrumentStatus) - 1; i++) out_sum += spi_tx_buffer[i];
    spi_tx_buffer[4] = (uint8_t)(0 - out_sum);
}

// ----------------------------------------------------------------------------
// 起動直後など、ControlCommandを介さない場面用の初期送信バッファ作成。
// ----------------------------------------------------------------------------
void prepare_tx_buffer_idle() {
    prepare_tx_buffer_head();
    spi_tx_buffer[3] = 0x01; // 初期状態は異常なしとして扱う

    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(InstrumentStatus) - 1; i++) sum += spi_tx_buffer[i];
    spi_tx_buffer[4] = (uint8_t)(0 - sum);
}

// ----------------------------------------------------------------------------
// 【指摘1対応】MISOピンの駆動切り替え。
// 複数台が同じMISOバスを共有する構成のため、CS非選択時にMISOをOUTPUTで
// 駆動し続けるとバス衝突を起こす。通信中だけOUTPUTにし、終了後は必ずINPUT
// (ハイインピーダンス)へ戻す。
// ----------------------------------------------------------------------------
void miso_drive_enable() {
    pinMode(SLAVE_MISO_PIN, OUTPUT);
}
void miso_drive_disable() {
    pinMode(SLAVE_MISO_PIN, INPUT);
}

// ----------------------------------------------------------------------------
// MOSIから1ビット読みつつ、同じタイミングでMISOに1ビット出す(全二重・1バイト分)。
// MODE0(CPOL0,CPHA0): SCKの立ち上がりでサンプリング、立ち下がりで次ビットを準備する。
// タイムアウトまたはCSがHIGHに戻った場合は false を返し、その時点で通信を打ち切る。
// ----------------------------------------------------------------------------
bool spi_transfer_byte_slave(uint8_t tx_byte, uint8_t* byte_out) {
    uint8_t rx_byte = 0;

    // 最初のビットはSCKが上がる前にMISOへ出しておく(MSBFIRST)
    digitalWrite(SLAVE_MISO_PIN, (tx_byte & 0x80) ? HIGH : LOW);

    for (int i = 0; i < 8; i++) {
        unsigned long wait_start = micros();
        while (digitalRead(SLAVE_SCK_PIN) == LOW) {
            if (digitalRead(SLAVE_CS_PIN) == HIGH) return false; // 通信終了
            if (micros() - wait_start > SCK_EDGE_TIMEOUT_US) return false; // タイムアウト
        }
        rx_byte |= (digitalRead(SLAVE_MOSI_PIN) << (7 - i)); // SCK立ち上がりでMOSIを確定

        if (i < 7) {
            digitalWrite(SLAVE_MISO_PIN, (tx_byte & (0x40 >> i)) ? HIGH : LOW);
        }

        wait_start = micros();
        while (digitalRead(SLAVE_SCK_PIN) == HIGH) {
            if (digitalRead(SLAVE_CS_PIN) == HIGH) return false;
            if (micros() - wait_start > SCK_EDGE_TIMEOUT_US) return false;
        }
    }

    *byte_out = rx_byte;
    return true;
}

// ----------------------------------------------------------------------------
// マスターがCSをLOWにした瞬間に発動する割り込み。1回のCS LOW区間内で、
// ハンドシェイク(2バイト)または本通信(7バイト)のいずれかを処理する。
// 【指摘4対応】ここではバイトの送受信とチェックサム判定のみを行い、
// score_init/score_stop_allなどの実処理はprocess_pending_command()(loop側)に委譲する。
// ----------------------------------------------------------------------------
void on_cs_falling() {
    miso_drive_enable(); // 【指摘1対応】通信中のみMISOを駆動する

    uint8_t first_byte = 0;

    // 1バイト目交換: ControlCommand.command_type を受信しつつ
    // InstrumentStatus.instrument_id (spi_tx_buffer[0]) を返す。
    if (!spi_transfer_byte_slave(spi_tx_buffer[0], &first_byte)) {
        miso_drive_disable();
        return; // 1バイトも来ないまま終了(ノイズ等)
    }

    if (first_byte == CMD_CONNECT) {
        // 【ハンドシェイク経路】wait_ack(): CMD_CONNECT → DUMMY の2バイトのみ。
        // 2バイト目でACK_OKを返す。
        uint8_t dummy_byte = 0;
        spi_transfer_byte_slave(ACK_OK, &dummy_byte);
        miso_drive_disable();
        return;
    }

    // 【本通信経路】ControlCommand(5バイト) + 確定待ち2バイト(ack_ok, checksum)
    spi_rx_buffer[0] = first_byte;

    // 2〜5バイト目: payload下位/上位, sequence, checksum を受信しつつ、
    // frog_state, sequence_ack(前回値), ack_ok(暫定), checksum(暫定) を送信する。
    for (uint8_t i = 1; i < sizeof(ControlCommand); i++) {
        uint8_t tx_byte = spi_tx_buffer[i];
        uint8_t rx_byte = 0;
        if (!spi_transfer_byte_slave(tx_byte, &rx_byte)) {
            miso_drive_disable();
            return; // マスターが途中で通信を終えた(想定外の短い通信)
        }
        spi_rx_buffer[i] = rx_byte;
    }

    // ここでspi_rx_buffer[0..4] = ControlCommandの5バイトが揃った。
    // server.ino はここでdelay(1)を挟みクロックを止めるため、この間に
    // チェックサム検証とack_ok/checksum確定を完了させる。
    uint8_t rx_copy[sizeof(ControlCommand)];
    memcpy(rx_copy, (const void*)spi_rx_buffer, sizeof(ControlCommand));
    finalize_tx_buffer_tail(rx_copy); // spi_tx_buffer[3](ack_ok), [4](checksum)を確定

    // 6,7バイト目: マスターはDUMMY_ACK,DUMMY_SUMを送ってくるが内容は使わない。
    // スレーブは確定済みのack_ok, checksumを返す。
    for (uint8_t i = 0; i < 2; i++) {
        uint8_t dummy_in = 0;
        uint8_t tx_byte = spi_tx_buffer[3 + i]; // 3=ack_ok, 4=checksum
        if (!spi_transfer_byte_slave(tx_byte, &dummy_in)) {
            miso_drive_disable();
            return; // delay(1)後の2バイトが来なかった(マスター側の想定外動作)
        }
    }

    miso_drive_disable(); // 【指摘1対応】通信終了時は必ずINPUTへ戻す

    // 次回通信のために、instrument_id/frog_state/sequence_ack(前回値)を更新しておく。
    // (frog_stateはloop()側のpressure_read()結果が常に最新化されているため、
    //  次回CS LOWの直前までにここで反映させておく)
    prepare_tx_buffer_head();
}

// アクティブな（現在鳴っている）全音符にNOTE_OFFを発行して停止する
void score_stop_all() {
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        // 現在音が鳴っている（アクティブな）音符があれば
        if (note_active[i]) {
            uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
            
            // 本来はscore_stepと同様に現在のピッチオフセットを考慮すべきですが、
            // ひとまず安全に音を止めるためにNOTE_OFFを送信します。
            // ※もし不都合があれば shifted_pitch を計算して渡してください。
            serial_tx_note_off(pitch);
            
            note_active[i] = false; // 未アクティブ状態に戻す
        }
    }
    // tickの記録もリセット
    last_tick = 0xFFFF;
}


// ----------------------------------------------------------------------------
// ソフトウェアSPIスレーブの初期化。
// ----------------------------------------------------------------------------
void spi_setup() {
    pinMode(SLAVE_MOSI_PIN, INPUT);
    miso_drive_disable(); // 【指摘1対応】初期状態はINPUT(非駆動)
    pinMode(SLAVE_SCK_PIN, INPUT);
}

void init_spi_slave() {
    pinMode(SLAVE_CS_PIN, INPUT_PULLUP);

    spi_setup();
    prepare_tx_buffer_idle(); // 起動直後の送信バッファを用意

    // CSピンがLOW（通信開始）になった瞬間に割り込み関数を起動する
    attachInterrupt(digitalPinToInterrupt(SLAVE_CS_PIN), on_cs_falling, FALLING);
}
