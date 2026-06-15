// score_player.ino
#include "score_data.h"
#include <avr/pgmspace.h>

#define DRUM_KICK  36
#define DRUM_SNARE 38
#define DRUM_HH    42

//楽譜作る必要あり．
// 1拍＝2ティック、全64ティックの「カエルの歌」
const NoteEvent score_part_0[] PROGMEM = {
    // 【1〜4小節】かえるのうたが きこえてくるよ (4分音符 × 12 + 2拍休み)
    {0,  2,  60, 100}, // ド
    {2,  4,  62, 100}, // レ
    {4,  6,  64, 100}, // ミ
    {6,  8,  65, 100}, // ファ
    {8,  10, 64, 100}, // ミ
    {10, 12, 62, 100}, // レ
    {12, 14, 60, 100}, // ド
    // 14〜16は休み (2ティック分)

    {16, 18, 64, 100}, // ミ
    {18, 20, 65, 100}, // ファ
    {20, 22, 67, 100}, // ソ
    {22, 24, 69, 100}, // ラ
    {24, 26, 67, 100}, // ソ
    {26, 28, 65, 100}, // ファ
    {28, 30, 64, 100}, // ミ
    // 30〜32は休み (2ティック分)

    // 【5〜6小節】グワッ ぐわっ グワッ ぐわっ (4分音符 + 4分休符 の繰り返し)
    {32, 34, 60, 100}, // ド
    // 34〜36は休み
    {36, 38, 60, 100}, // ド
    // 38〜40は休み
    {40, 42, 60, 100}, // ド
    // 42〜44は休み
    {44, 46, 60, 100}, // ド
    // 46〜48は休み

    // 【7〜8小節】ゲゲゲゲ ゲゲゲゲ くわくわくわくわ
    // 前半：1拍に2音ずつ (8分音符 × 8連続)
    {48, 49, 60, 100}, {49, 50, 60, 100}, // ドド (あわせて2ティック)
    {50, 51, 62, 100}, {51, 52, 62, 100}, // レレ (あわせて2ティック)
    {52, 53, 64, 100}, {53, 54, 64, 100}, // ミミ (あわせて2ティック)
    {54, 55, 65, 100}, {55, 56, 65, 100}, // ファファ (あわせて2ティック)
    
    // 後半：元のテンポに戻る (4分音符 × 4)
    {56, 58, 64, 100}, // ミ
    {58, 60, 62, 100}, // レ
    {60, 62, 60, 100}, // ド
    // 62〜64は最後の余韻・休み
};

// 1拍＝2ティック、全64ティックの「カエルの歌」
const NoteEvent score_part_1[] PROGMEM = {
    // 【1〜4小節】かえるのうたが きこえてくるよ (4分音符 × 12 + 2拍休み)
    {0,  2,  60, 100}, // ド
    {2,  4,  62, 100}, // レ
    {4,  6,  64, 100}, // ミ
    {6,  8,  65, 100}, // ファ
    {8,  10, 64, 100}, // ミ
    {10, 12, 62, 100}, // レ
    {12, 14, 60, 100}, // ド
    // 14〜16は休み (2ティック分)

    {16, 18, 64, 100}, // ミ
    {18, 20, 65, 100}, // ファ
    {20, 22, 67, 100}, // ソ
    {22, 24, 69, 100}, // ラ
    {24, 26, 67, 100}, // ソ
    {26, 28, 65, 100}, // ファ
    {28, 30, 64, 100}, // ミ
    // 30〜32は休み (2ティック分)

    // 【5〜6小節】グワッ ぐわっ グワッ ぐわっ (4分音符 + 4分休符 の繰り返し)
    {32, 34, 60, 100}, // ド
    // 34〜36は休み
    {36, 38, 60, 100}, // ド
    // 38〜40は休み
    {40, 42, 60, 100}, // ド
    // 42〜44は休み
    {44, 46, 60, 100}, // ド
    // 46〜48は休み

    // 【7〜8小節】ゲゲゲゲ ゲゲゲゲ くわくわくわくわ
    // 前半：1拍に2音ずつ (8分音符 × 8連続)
    {48, 49, 60, 100}, {49, 50, 60, 100}, // ドド (あわせて2ティック)
    {50, 51, 62, 100}, {51, 52, 62, 100}, // レレ (あわせて2ティック)
    {52, 53, 64, 100}, {53, 54, 64, 100}, // ミミ (あわせて2ティック)
    {54, 55, 65, 100}, {55, 56, 65, 100}, // ファファ (あわせて2ティック)
    
    // 後半：元のテンポに戻る (4分音符 × 4)
    {56, 58, 64, 100}, // ミ
    {58, 60, 62, 100}, // レ
    {60, 62, 60, 100}, // ド
    // 62〜64は最後の余韻・休み
};

// 1拍＝2ティック、全64ティックの「カエルの歌」
const NoteEvent score_part_2[] PROGMEM = {
    // 【1〜4小節】かえるのうたが きこえてくるよ (4分音符 × 12 + 2拍休み)
    {0,  2,  60, 100}, // ド
    {2,  4,  62, 100}, // レ
    {4,  6,  64, 100}, // ミ
    {6,  8,  65, 100}, // ファ
    {8,  10, 64, 100}, // ミ
    {10, 12, 62, 100}, // レ
    {12, 14, 60, 100}, // ド
    // 14〜16は休み (2ティック分)

    {16, 18, 64, 100}, // ミ
    {18, 20, 65, 100}, // ファ
    {20, 22, 67, 100}, // ソ
    {22, 24, 69, 100}, // ラ
    {24, 26, 67, 100}, // ソ
    {26, 28, 65, 100}, // ファ
    {28, 30, 64, 100}, // ミ
    // 30〜32は休み (2ティック分)

    // 【5〜6小節】グワッ ぐわっ グワッ ぐわっ (4分音符 + 4分休符 の繰り返し)
    {32, 34, 60, 100}, // ド
    // 34〜36は休み
    {36, 38, 60, 100}, // ド
    // 38〜40は休み
    {40, 42, 60, 100}, // ド
    // 42〜44は休み
    {44, 46, 60, 100}, // ド
    // 46〜48は休み

    // 【7〜8小節】ゲゲゲゲ ゲゲゲゲ くわくわくわくわ
    // 前半：1拍に2音ずつ (8分音符 × 8連続)
    {48, 49, 60, 100}, {49, 50, 60, 100}, // ドド (あわせて2ティック)
    {50, 51, 62, 100}, {51, 52, 62, 100}, // レレ (あわせて2ティック)
    {52, 53, 64, 100}, {53, 54, 64, 100}, // ミミ (あわせて2ティック)
    {54, 55, 65, 100}, {55, 56, 65, 100}, // ファファ (あわせて2ティック)
    
    // 後半：元のテンポに戻る (4分音符 × 4)
    {56, 58, 64, 100}, // ミ
    {58, 60, 62, 100}, // レ
    {60, 62, 60, 100}, // ド
    // 62〜64は最後の余韻・休み
};

// リズムの譜面をここに入れる．一つはリズムに使う．
const NoteEvent score_part_3[] PROGMEM = {
    //ここに記述（書いてあるのはまだ曲に合ってないから後で調整する）
    //1〜16ティック（1小節目）
    {0,  1,  DRUM_KICK,  110}, {0,  1,  DRUM_HH,    90},
    {4,  5,  DRUM_HH,    80},
    {8,  9,  DRUM_SNARE, 100}, {8,  9,  DRUM_HH,    90},
    {12, 13, DRUM_HH,    80},
    
    //17〜32ティック（2小節目）
    {16, 17, DRUM_KICK,  110}, {16, 17, DRUM_HH,    90},
    {20, 21, DRUM_HH,    80},
    {24, 25, DRUM_SNARE, 100}, {24, 25, DRUM_HH,    90},
    {28, 29, DRUM_HH,    80},

    //33〜48ティック（3小節目の例）
    {32, 33, DRUM_KICK,  110}, {32, 33, DRUM_HH,    90},
    {36, 37, DRUM_HH,    80},
    {40, 41, DRUM_SNARE, 100}, {40, 41, DRUM_HH,    90},
    {44, 45, DRUM_HH,    80},

    //49〜64ティック（4小節目）
    {48, 49, DRUM_KICK,  110}, {48, 49, DRUM_HH,    90},
    {52, 53, DRUM_HH,    80},
    {56, 57, DRUM_SNARE, 100}, {56, 57, DRUM_HH,    90},
    {60, 61, DRUM_HH,    80}
};

const uint8_t score_lengths[4] = {29, 29, 29, 20};

const NoteEvent* my_score = NULL;
uint8_t my_score_length = 0;

static const uint8_t MAX_NOTES = 29;
bool note_active[MAX_NOTES] = {false};

extern void serial_tx_note_on(uint8_t pitch, uint8_t velocity);
extern void serial_tx_note_off(uint8_t pitch);

void score_init(uint8_t instrument_id) {
    switch (instrument_id) {
        case 0: my_score = score_part_0; break;
        case 1: my_score = score_part_1; break;
        case 2: my_score = score_part_2; break;
        case 3: my_score = score_part_3; break;
        default: my_score = score_part_0; break;
    }
    my_score_length = score_lengths[instrument_id];

    for (uint8_t i = 0; i < my_score_length; i++) {
        note_active[i] = false;
    }
}

void score_step(uint16_t local_tick, int8_t pitch_offset) {  //引数追加
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        uint16_t start_t = pgm_read_word(&(my_score[i].start_tick));
        uint16_t end_t = pgm_read_word(&(my_score[i].end_tick));
        uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
        uint8_t velocity = pgm_read_byte(&(my_score[i].velocity));

        //追加部：オフセットを加算（0〜127の範囲にクランプ）
        int16_t shifted_pitch = (int16_t)pitch + pitch_offset;
        if (shifted_pitch < 0)   shifted_pitch = 0;
        if (shifted_pitch > 127) shifted_pitch = 127;

        if (local_tick == start_t && !note_active[i]) {
            serial_tx_note_on(pitch, velocity);
            note_active[i] = true;
        }

        if (local_tick == end_t && note_active[i]) {
            serial_tx_note_off(pitch);
            note_active[i] = false;
        }
    }
}

void score_loop_check(volatile uint16_t &local_tick) {
    if (local_tick >= LOOP_MAX_TICK) {
        noInterrupts();
        local_tick = 0;
        interrupts();

        for (uint8_t i = 0; i < my_score_length; i++) {
            note_active[i] = false;
        }
    }
}

void score_stop_all() {
    if (my_score == NULL) return;

    for (uint8_t i = 0; i < my_score_length; i++) {
        if (note_active[i]) {
            uint8_t pitch = pgm_read_byte(&(my_score[i].pitch));
            serial_tx_note_off(pitch);
            note_active[i] = false;
        }
    }
}