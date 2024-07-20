/*

MIT License

Copyright (c) 2024 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include <rtc.h>
#include <f_util.h>
#include <hw_config.h>

#include "board.h"

static char code[] = "Bad-Apple-IIgs.code";

static FIL file;

void start(void) {
    printf("Start\n");

#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, true);
#endif

    f_close(&file);

    FRESULT fr = f_open(&file, code, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        printf("f_open(%s) error: %s (%d)\n", code, FRESULT_str(fr), fr);
    }

    UINT br;
    fr = f_read(&file, bank, sizeof(bank), &br);
    if (fr != FR_OK || br != sizeof(bank)) {
        printf("f_read(%s) error: %s (%d)\n", code, FRESULT_str(fr), fr);
    }

#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif
}

void main(void) {
    multicore_launch_core1(board);

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif 

    stdio_init_all();
    printf("Bad Apple !!\n");

    time_init();

    sd_card_t *sd_card = sd_get_by_num(0);
    FRESULT fr = f_mount(&sd_card->fatfs, sd_card->pcName, 1);
    if (fr != FR_OK) {
        printf("f_mount(%s) error: %s (%d)\n", sd_card->pcName, FRESULT_str(fr), fr);
    }

    while (true) {

        if (reset) {
            reset = false;
            start();
            multicore_fifo_drain();
        }

        if (!multicore_fifo_rvalid()) {
            continue;
        }
        uint32_t next = multicore_fifo_pop_blocking();

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, true);
#endif

        UINT br;
        FRESULT fr = f_read(&file, bank[next], sizeof(bank[0]), &br);
        if (fr != FR_OK || br != sizeof(bank[0])) {
            printf("f_read(%s) error: %s (%d)\n", code, FRESULT_str(fr), fr);
        }

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif
    }
}
