// spi_slave.ino
#include "score_data.h"

const int SS_PIN = 10;

enum ControlCommand {
    CMD_PLAY = 0x01,
    CMD_STOP = 0x02,
    CMD_ENTRY_CUE = 0x03,
    CMD_BPM_UPDATE = 0x04
};

extern volatile bool is_playing;
extern volatile uint16_t local_tick;
extern uint8_t frog_state;
extern void score_init(uint8_t instrument_id);
extern void score_stop_all();
extern uint8_t get_instrument_id();

volatile uint8_t spi_rx_buffer[5];
volatile uint8_t spi_tx_buffer[5];
volatile uint8_t spi_index = 0;
volatile uint8_t last_sequence = 0;
volatile uint8_t ack_status = 0x01; // 0x01: OK, 0x00: NG

void prepare_tx_buffer() {
    spi_tx_buffer[0] = get_instrument_id();
    spi_tx_buffer[1] = frog_state;
    spi_tx_buffer[2] = last_sequence;
    spi_tx_buffer[3] = ack_status;
    
    // 2の補数チェックサム付与
    uint8_t sum = spi_tx_buffer[0] + spi_tx_buffer[1] + spi_tx_buffer[2] + spi_tx_buffer[3];
    spi_tx_buffer[4] = (~sum + 1) & 0xFF;
}

void init_spi_slave() {
    pinMode(MISO, OUTPUT);
    pinMode(MOSI, INPUT);
    pinMode(SCK, INPUT);
    pinMode(SS_PIN, INPUT);

    prepare_tx_buffer();
    SPDR = spi_tx_buffer[0];

    SPCR |= _BV(SPE);
    SPCR |= _BV(SPIE);
}

ISR(SPI_STC_vect) {
    spi_rx_buffer[spi_index] = SPDR;
    spi_index++;

    if (spi_index < 5) {
        SPDR = spi_tx_buffer[spi_index];
    } else {
        // 5バイト受信完了時に解析
        uint8_t sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += spi_rx_buffer[i];
        }

        if ((sum & 0xFF) == 0) { // チェックサム正常
            uint8_t command = spi_rx_buffer[0];
            last_sequence = spi_rx_buffer[3];
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
            }
        } else {
            ack_status = 0x00; // エラー報告
        }

        prepare_tx_buffer();
        spi_index = 0;
        SPDR = spi_tx_buffer[0]; // 次のトランザクションに向けて準備
    }
}