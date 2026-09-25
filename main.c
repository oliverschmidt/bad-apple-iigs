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
#include <hardware/clocks.h>
#include <hardware/structs/busctrl.h>

#include <a2pico.h>
#include <hw_config.h>
#include <f_util.h>

#include "board.h"

static char code[] = "Bad-Apple-IIgs.code";

static FIL file;

void start(void) {
    printf("Start\n");

    if (a2pico_led() >= 0) {
        gpio_put(a2pico_led(), true);
    }

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

    if (a2pico_led() >= 0) {
        gpio_put(a2pico_led(), false);
    }
}

void main(void) {
    busctrl_hw->priority = BUSCTRL_BUS_PRIORITY_PROC1_BITS;
    multicore_launch_core1(board);

    set_sys_clock_khz(200000, false);

    if (a2pico_led() >= 0) {
        gpio_init(a2pico_led());
        gpio_set_dir(a2pico_led(), GPIO_OUT);
    }

    stdio_uart_init_full(uart0, PICO_DEFAULT_UART_BAUD_RATE, a2pico_tx(), a2pico_rx());
    printf("Bad Apple !!\n");

    if (!a2pico_sd()) {
        printf("No SD Card slot :-(\n");
        return;
    }

    sd_card_t *sd_card = sd_get_by_num(0);
    FRESULT fr = f_mount(&sd_card->state.fatfs, "", 1);
    if (fr != FR_OK) {
        printf("f_mount() error: %s (%d)\n", FRESULT_str(fr), fr);
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

        if (a2pico_led() >= 0) {
            gpio_put(a2pico_led(), true);
        }

        UINT br;
        FRESULT fr = f_read(&file, bank[next], sizeof(bank[0]), &br);
        if (fr != FR_OK || br != sizeof(bank[0])) {
            printf("f_read(%s) error: %s (%d)\n", code, FRESULT_str(fr), fr);
        }

        if (a2pico_led() >= 0) {
            gpio_put(a2pico_led(), false);
        }
    }
}
