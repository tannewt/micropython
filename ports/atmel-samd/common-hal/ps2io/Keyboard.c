/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
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

#include "common-hal/ps2io/Keyboard.h"

#include <stdint.h>

#include "atmel_start_pins.h"
#include "hal/include/hal_gpio.h"

#include "background.h"
#include "external_interrupts.h"
#include "mpconfigport.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "samd/external_interrupts.h"
#include "samd/pins.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/translate.h"

#include "tick.h"

void ps2io_interrupt_handler(uint8_t channel) {
    ps2io_keyboard_obj_t* self = get_eic_channel_data(channel);
    uint8_t bit = *self->data_pin_address & self->data_pin_mask;
    uint64_t ms;
    uint32_t us_until_ms;
    current_tick(&ms, &us_until_ms);
    // Debounce the clock signal.
    bool new_bit = self->last_bit_ms - ms > 2 ||
        (self->last_bit_ms - ms == 0 && self->last_bit_us_until_ms - us_until_ms > 60) ||
        (self->last_bit_ms - ms == 1 && 1000 - us_until_ms + self->last_bit_us_until_ms > 60);
    self->last_bit_ms = ms;
    self->last_bit_us_until_ms = us_until_ms;
    if (!new_bit) {
        return;
    }
    PORT->Group[1].OUTTGL.reg = 1;

    self->bitbuffer <<= 1;
    if (bit != 0) {
        self->bitbuffer |= 1;
    }
    self->bitcount++;
    if (self->bitcount == 11) {
        ringbuf_put(&self->buf, (self->bitbuffer >> 2) & 0xff);
        self->bitcount = 0;
        self->bitbuffer = 0;
    }
    PORT->Group[1].OUTTGL.reg = 1;
}

void common_hal_ps2io_keyboard_construct(ps2io_keyboard_obj_t* self,
        const mcu_pin_obj_t* clock_pin, const mcu_pin_obj_t* data_pin, size_t bufsize) {
    if (!clock_pin->has_extint) {
        mp_raise_RuntimeError(translate("No hardware support on clock pin"));
    }
    if (eic_get_enable() && !eic_handler_free(clock_pin->extint_channel)) {
        mp_raise_RuntimeError(translate("EXTINT channel already in use"));
    }

    ringbuf_alloc(&self->buf, bufsize, false);

    self->channel = clock_pin->extint_channel;
    self->clock_pin = clock_pin->number;

    PortGroup *const port = &PORT->Group[(enum gpio_port)GPIO_PORT(data_pin->number)];
    port->DIRSET.reg = 1; // Enable PB00 as output for debugging.
    uint8_t pin_index = GPIO_PIN(data_pin->number);
    volatile PORT_PINCFG_Type *state = &port->PINCFG[pin_index];
    state->bit.INEN = true;
    self->data_pin_address = (uint8_t*) (&port->IN.reg) + (pin_index / 8);
    self->data_pin_mask = 1 << (pin_index % 8);
    self->data_pin = data_pin->number;

    set_eic_channel_data(clock_pin->extint_channel, (void*) self);

    // Check to see if the EIC is enabled and start it up if its not.'
    if (eic_get_enable() == 0) {
        turn_on_external_interrupt_controller();
    }

    gpio_set_pin_function(clock_pin->number, GPIO_PIN_FUNCTION_A);

    turn_on_cpu_interrupt(self->channel);

    claim_pin(clock_pin);

    // Set config will enable the EIC.
    enable_eic_channel_handler(self->channel, EIC_CONFIG_SENSE0_FALL_Val, EIC_HANDLER_PS2IO);
}

bool common_hal_ps2io_keyboard_deinited(ps2io_keyboard_obj_t* self) {
    return self->clock_pin == NO_PIN;
}

void common_hal_ps2io_keyboard_deinit(ps2io_keyboard_obj_t* self) {
    if (common_hal_ps2io_keyboard_deinited(self)) {
        return;
    }
    disable_eic_channel_handler(self->channel);
    reset_pin_number(self->clock_pin);
    reset_pin_number(self->data_pin);
    self->clock_pin = NO_PIN;
}

void common_hal_ps2io_keyboard_clear_buffer(ps2io_keyboard_obj_t *self) {
    common_hal_mcu_disable_interrupts();
    ringbuf_clear(&self->buf);
    common_hal_mcu_enable_interrupts();
}

// Updates the hid report and determines what unicode character to produce to the stream.
static void update_chars(int value, uint8_t *data, size_t len) {
    if (value == 0x14) { // Control
        uint8_t bitmask = 0;
        if (self->extended) {
            bitmask = 1 << 4;
        } else {
            bitmask = 1 << 0;
        }
        if (self->break) {
            self->usb_hid_report[0] &= ~bitmask;
        }
    }
    uint8_t keycode = 0;
    switch (value) {
        case 0x29: // space
            keycode = 0x2c;
            break;
        case 0x66: // backspace
            keycode = 0x2a;
            break;
        case 0x0E: // `
            keycode = 0x35;
            break;
        case 0x16: // 1
            keycode = 0x1e;
            break;
        case 0x1E: // 2
            keycode = 0x1f;
            break;
        case 0x26: // 3
            keycode = 0x20;
            break;
        case 0x25: // 4
            keycode = 0x21;
            break;
        case 0x2E: // 5
            keycode = 0x22;
            break;
        case 0x36: // 6
            keycode = 0x23;
            break;
        case 0x3D: // 7
            keycode = 0x24;
            break;
        case 0x3E: // 8
            keycode = 0x25;
            break;
        case 0x46: // 9
            keycode = 0x26;
            break;
        case 0x45: // 0
            keycode = 0x27;
            break;
        case 0x4E: // -
            keycode = 0x2d;
            break;
        case 0x55: // =
            keycode = 0x2e;
            break;
        case 0x15: // Q
            keycode = 0x14;
            break;
        case 0x1D: // W
            keycode = 0x1a;
            break;
        case 0x24: // E
            keycode = 0x08;
            break;
        case 0x2D: // R
            keycode = 0x15;
            break;
        case 0x2C: // T
            keycode = 0x17;
            break;
        case 0x35: // Y
            keycode = 0x1c;
            break;
        case 0x3C: // U
            keycode = 0x2a;
            break;
        case 0x43: // I
            keycode = 0x2a;
            break;
        case 0x44: // O
            keycode = 0x2a;
            break;
        case 0x4D: // P
            keycode = 0x2a;
            break;
        case 0x54: // [
            keycode = 0x2a;
            break;
        case 0x5B: // ]
            keycode = 0x2a;
            break;
        case 0x5D: // \
            keycode = 0x2a;
            break;
        case 0x1C: // A
            keycode = 0x2a;
            break;
        case 0x1B: // S
            keycode = 0x2a;
            break;
        case 0x23: // D
            keycode = 0x2a;
            break;
        case 0x2B: // F
            keycode = 0x2a;
            break;
        case 0x34: // G
            keycode = 0x2a;
            break;
        case 0x33: // H
            keycode = 0x2a;
            break;
        case 0x3B: // J
            keycode = 0x2a;
            break;
        case 0x42: // K
            keycode = 0x2a;
            break;
        case 0x4B: // L
            keycode = 0x2a;
            break;
        case 0x4C: // ;
            keycode = 0x2a;
            break;
        case 0x52: // '
            keycode = 0x2a;
            break;
        case 0x5A: // enter
            keycode = 0x2a;
            break;
        case 0x1A: // Z
            keycode = 0x2a;
            break;
        case 0x22: // X
            keycode = 0x2a;
            break;
        case 0x21: // C
            keycode = 0x2a;
            break;
        case 0x2A: // V
            keycode = 0x2a;
            break;
        case 0x32: // B
            keycode = 0x2a;
            break;
        case 0x31: // N
            keycode = 0x2a;
            break;
        case 0x3A: // M
            keycode = 0x2a;
            break;
        case 0x41: // ,
            keycode = 0x2a;
            break;
        case 0x49: // .
            keycode = 0x2a;
            break;
        case 0x4A: // /
            keycode = 0x2a;
            break;
    }
}

// Read characters.
size_t common_hal_ps2io_keyboard_read(ps2io_keyboard_obj_t *self,
uint8_t *data, size_t len, int *errcode) {
    size_t i;
    for (i = 0; i < len; i++) {
        common_hal_mcu_disable_interrupts();
        int value = ringbuf_get(&self->buf);
        common_hal_mcu_enable_interrupts();
        if (value == -1) {
            break;
        }
        if (value == 0xe0) {
            self->extended = true;
        } else if (value == 0xf0) {
            self->break = true;
        } else {
            update_chars(value, data + i, len - i);
            self->extended = false;
            self->break = false;
        }
    }
    return i;
}

uint32_t common_hal_ps2io_keyboard_bytes_available(ps2io_keyboard_obj_t *self) {
    return ringbuf_count(&self->buf);
}
