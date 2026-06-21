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
extern int8_t pressure_get_pitch_offset();  // 【追加】

extern void score_step(uint16_t local_tick, int8_t pitch_offset); //引数追加
extern void score_loop_check(volatile uint16_t &local_tick);
extern void score_stop_all();
// 【追加】spi_slave.ino のSPI割り込み(on_cs_falling)が予約したコマンドを、
// 割り込みの外(loop側)で実際に実行するための関数。重い処理(score_init等)を
// 割り込みハンドラから追い出すための仕組み(instrument_spi_fix_notes.md 指摘4対応)。
extern void process_pending_command();

void setup() {
    init_serial_tx();
    instrument_id_init();
    score_init(get_instrument_id());
    init_spi_slave();
    sync_init();
    pressure_init();
}

void loop() {
    // 【追加】SPI割り込みが予約したコマンド(PLAY/STOP/ENTRY_CUE/BPM_UPDATE)を
    // ここで実処理する。score_init/score_stop_allなど時間のかかる処理は
    // 割り込みハンドラ内では行わず、必ずこのloop()側で実行する。
    process_pending_command();

    // 圧力センサからfrog_state（0 or 1）を更新
    frog_state = pressure_read();

    if (is_playing) {
        noInterrupts();
        uint16_t current_tick = local_tick;
        interrupts();

        int8_t pitch_offset = pressure_get_pitch_offset(); //追加
        score_step(current_tick, pitch_offset); //変更
        score_loop_check(local_tick);
    }
}