//#include <SPI.h>

// main.ino
#include "score_data.h"

volatile bool is_playing = false;
volatile uint16_t local_tick = 0;
uint8_t frog_state = 0;

extern void init_serial_tx();
extern void instrument_id_init();
extern uint8_t get_instrument_id();
extern void score_init(uint8_t instrument_id);
extern void init_spi_slave();
extern void sync_init();
extern void pressure_init();
extern uint8_t pressure_read();

extern void score_step(uint16_t local_tick);
extern void score_loop_check(volatile uint16_t &local_tick);
extern void score_stop_all();

void setup() {
    init_serial_tx();
    instrument_id_init();
    score_init(get_instrument_id());
    init_spi_slave();
    sync_init();
    pressure_init();
}

void loop() {
    // 圧力センサからfrog_state（0 or 1）を更新
    frog_state = pressure_read();

    if (is_playing) {
        noInterrupts();
        uint16_t current_tick = local_tick;
        interrupts();

        score_step(current_tick);
        score_loop_check(local_tick);
    }
}