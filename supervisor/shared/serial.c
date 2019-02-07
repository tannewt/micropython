/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
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

#include <string.h>

#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "supervisor/shared/display.h"
#include "shared-bindings/ps2io/Keyboard.h"
#include "shared-bindings/terminalio/Terminal.h"
#include "supervisor/serial.h"
#include "supervisor/usb.h"

#include "tusb.h"

// Cheat for now and always have a spot allocated for a keyboard.
static ps2io_keyboard_obj_t supervisor_keyboard;
static uint8_t kbd_buffer[256];

void serial_init(void) {
    usb_init();
}

bool serial_connected(void) {
    if (supervisor_keyboard.base.type != NULL && supervisor_keyboard.base.type != &mp_type_NoneType) {
        return true;
    }
    return tud_cdc_connected();
}

char serial_read(void) {
    if (supervisor_keyboard.base.type != NULL && supervisor_keyboard.base.type != &mp_type_NoneType) {
        char c;
        int errcode;
        mp_uint_t count = mp_stream_read_exactly(MP_OBJ_FROM_PTR(&supervisor_keyboard), &c, 1, &errcode);
        if (count == 1) {
            return c;
        }
    }
    return (char) tud_cdc_read_char();
}

bool serial_bytes_available(void) {
    if (supervisor_keyboard.base.type != NULL && supervisor_keyboard.base.type != &mp_type_NoneType) {
        mp_obj_t stream = MP_OBJ_FROM_PTR(&supervisor_keyboard);
        const mp_stream_p_t *stream_p = mp_get_stream(stream);
        int error;
        mp_uint_t res = stream_p->ioctl(stream, MP_STREAM_POLL, MP_STREAM_POLL_RD, &error);
        if (res == MP_STREAM_POLL_RD) {
            return true;
        }
    }
    return tud_cdc_available() > 0;
}

void serial_write_substring(const char* text, uint32_t length) {
    #if CIRCUITPY_DISPLAYIO
    int errcode;
    common_hal_terminalio_terminal_write(&supervisor_terminal, (const uint8_t*) text, length, &errcode);
    #endif
    if (!tud_cdc_connected()) {
        return;
    }
    uint32_t count = 0;
    while (count < length) {
        count += tud_cdc_write(text + count, length - count);
        usb_background();
    }
}

void serial_write(const char* text) {
    serial_write_substring(text, strlen(text));
}

void serial_connect_stream(mp_obj_t stream_obj) {
    mp_obj_t native_stream = mp_instance_cast_to_native_base(stream_obj, &ps2io_keyboard_type);
    if (native_stream == MP_OBJ_NULL) {
        mp_raise_TypeError(translate("Only ps2io.Keyboard streams supported."));
    }

    ps2io_keyboard_obj_t* keyboard = MP_OBJ_TO_PTR(native_stream);
    supervisor_keyboard.buf.buf = kbd_buffer;
    common_hal_ps2io_keyboard_move(keyboard, &supervisor_keyboard);
}
