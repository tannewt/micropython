/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "atmel_start_pins.h"

static Sercom *sercom_instances[SERCOM_INST_NUM] = SERCOM_INSTS;
static bool skip_reset[SERCOM_INST_NUM];

void busio_init(void) {
    for (int i = 0; i < SERCOM_INST_NUM; i++) {
        skip_reset[i] = false;
    #ifdef SPI_FLASH_SERCOM
        if (sercom_instances[i] == SPI_FLASH_SERCOM) {
            skip_reset[i] = true;
        }
    #endif
    #ifdef MICROPY_HW_APA102_SERCOM
        if (sercom_instances[i] == MICROPY_HW_APA102_SERCOM) {
            skip_reset[i] = true;
        }
    #endif
    }
}

void busio_reset(void) {
    // Reset all SERCOMs except the ones being used by on-board devices.
    for (int i = 0; i < SERCOM_INST_NUM; i++) {
        if (skip_reset[i]) {
            continue;
        }
        // SWRST is same for all modes of SERCOMs.
        sercom_instances[i]->SPI.CTRLA.bit.SWRST = 1;
    }
}
