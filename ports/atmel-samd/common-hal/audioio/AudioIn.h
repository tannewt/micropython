/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
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

#ifndef MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_AUDIOIO_AUDIOOUT_H
#define MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_AUDIOIO_AUDIOOUT_H

#include "common-hal/microcontroller/Pin.h"

#include "audio_dma.h"
#include "py/obj.h"

typedef struct {
    mp_obj_base_t base;
    const mcu_pin_obj_t *left_channel;
    audio_dma_t left_dma;
    uint8_t left_adc_instance_number;
    #ifdef SAMD51
    const mcu_pin_obj_t *right_channel;
    audio_dma_t right_dma;
    uint8_t right_adc_instance_number;
    #endif
    uint8_t tc_index;

    uint8_t tc_to_dac_event_channel;
    bool recording;
} audioio_audioin_obj_t;

void audioin_reset(void);

void audioin_background(void);

#endif // MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_AUDIOIO_AUDIOOUT_H
