/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_PS2IO_KEYBOARD_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_PS2IO_KEYBOARD_H

#include "common-hal/ps2io/Keyboard.h"

#include "shared-bindings/microcontroller/Pin.h"

extern const mp_obj_type_t ps2io_keyboard_type;

// Construct an underlying UART object.
extern void common_hal_ps2io_keyboard_construct(ps2io_keyboard_obj_t *self,
    const mcu_pin_obj_t* clock_pin, const mcu_pin_obj_t* data_pin, size_t bufsize);
// Read characters.
extern size_t common_hal_ps2io_keyboard_read(ps2io_keyboard_obj_t *self,
    uint8_t *data, size_t len, int *errcode);

extern uint32_t common_hal_ps2io_keyboard_bytes_available(ps2io_keyboard_obj_t *self);
extern void common_hal_ps2io_keyboard_clear_buffer(ps2io_keyboard_obj_t *self);

extern void common_hal_ps2io_keyboard_move(ps2io_keyboard_obj_t *self, ps2io_keyboard_obj_t *new_location);

#endif  // MICROPY_INCLUDED_SHARED_BINDINGS_PS2IO_KEYBOARD_H
