/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
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

#include "common-hal/captureio/Capture.h"

#include "hal/include/hal_gpio.h"

#define RESET_PIN PIN_PB22

void common_hal_captureio_capture_construct(captureio_capture_obj_t* self) {

    // Put the CPU  in reset.
    gpio_set_pin_function(RESET_PIN, GPIO_PIN_FUNCTION_OFF);
    gpio_set_pin_direction(RESET_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(RESET_PIN, true);
}

bool common_hal_captureio_capture_deinited(captureio_capture_obj_t *self) {
    return false;
}

void common_hal_captureio_capture_deinit(captureio_capture_obj_t *self) {
    if (common_hal_captureio_capture_deinited(self)) {
        return;
    }
}


void common_hal_captureio_capture_capture(captureio_capture_obj_t* self, uint8_t* buffer,
    uint32_t buffer_length) {

    // Start the CPU
    gpio_set_pin_level(RESET_PIN, false);

    // Stop the CPU
    gpio_set_pin_level(RESET_PIN, true);
    asm("nop");
}
