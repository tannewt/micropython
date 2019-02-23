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
#include <string.h>

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
    // PORT->Group[1].OUTTGL.reg = 1;
    ps2io_keyboard_obj_t* self = get_eic_channel_data(channel);
    uint8_t bit = *self->data_pin_in_address & self->data_pin_mask;
    uint64_t ms;
    uint32_t us_until_ms;
    current_tick(&ms, &us_until_ms);
    // Debounce the clock signal.
    bool new_bit = ms - self->last_bit_ms > 1 ||
        (ms - self->last_bit_ms == 0 && self->last_bit_us_until_ms - us_until_ms > 40) ||
        (ms - self->last_bit_ms == 1 && 1000 - us_until_ms + self->last_bit_us_until_ms > 40);

    if (!new_bit) {
        // PORT->Group[1].OUTTGL.reg = 1;
        return;
    }
    self->last_bit_ms = ms;
    self->last_bit_us_until_ms = us_until_ms;

    if (self->command_bits % 12 != 0 || self->command_acked) {
        self->command_acked = false;
        if ((self->command & 0x1) == 0) {
            // set direction to out set 0
            *self->data_pin_dirset_address = self->data_pin_mask;
        } else {
            // set direction to in to indicate 1 via pull up
            *self->data_pin_dirclr_address = self->data_pin_mask;
        }
        self->command_bits--;
        self->command >>= 1;
        // PORT->Group[1].OUTTGL.reg = 1;
        return;
    }

    self->bitbuffer <<= 1;
    if (bit != 0) {
        self->bitbuffer |= 1;
    }
    self->bitcount++;
    if (self->bitcount == 11) {
        // This swaps bit order before adding it to the queue.
        uint8_t value = __RBIT((self->bitbuffer >> 2) & 0xff) >> 24;
        if (value == 0xfa && self->command_bits > 0) {
            common_hal_mcu_delay_us(50);
            self->command_acked = true;
            // request a host -> device
            // Bring clock low which will trigger the first zero bit.
            gpio_set_pin_function(self->clock_pin, GPIO_PIN_FUNCTION_OFF);

            // Wait 100us
            common_hal_mcu_delay_us(100);

            // Consume the start bit since we consume the first clock edge.
            *self->data_pin_dirset_address = self->data_pin_mask;
            self->command_bits--;
            self->command >>= 1;
            current_tick(&self->last_bit_ms, &self->last_bit_us_until_ms);

            // Release clock.
            gpio_set_pin_function(self->clock_pin, GPIO_PIN_FUNCTION_A);
        } else {
            ringbuf_put(&self->buf, value);
        }
        self->bitcount = 0;
        self->bitbuffer = 0;
    }
    // PORT->Group[1].OUTTGL.reg = 1;
}

void common_hal_ps2io_keyboard_construct(ps2io_keyboard_obj_t* self,
        const mcu_pin_obj_t* clock_pin, const mcu_pin_obj_t* data_pin, size_t bufsize) {
    if (!clock_pin->has_extint) {
        mp_raise_RuntimeError(translate("No hardware support on clock pin"));
    }
    if (eic_get_enable() && !eic_handler_free(clock_pin->extint_channel)) {
        mp_raise_RuntimeError(translate("EXTINT channel already in use"));
    }

    ringbuf_alloc(&self->buf, bufsize / 2, false);
    ringbuf_alloc(&self->out_buf, bufsize / 2, false);

    self->channel = clock_pin->extint_channel;
    self->clock_pin = clock_pin->number;

    PortGroup *const port = &PORT->Group[(enum gpio_port)GPIO_PORT(data_pin->number)];
    // port->DIRSET.reg = 1; // Enable PB00 as output for debugging.
    uint8_t pin_index = GPIO_PIN(data_pin->number);
    volatile PORT_PINCFG_Type *state = &port->PINCFG[pin_index];
    state->bit.INEN = true;
    uint8_t byte_shift = (pin_index / 8);
    self->data_pin_in_address = (uint8_t*) (&port->IN.reg) + byte_shift;
    self->data_pin_dirclr_address = (uint8_t*) (&port->DIRCLR.reg) + byte_shift;
    self->data_pin_dirset_address = (uint8_t*) (&port->DIRSET.reg) + byte_shift;
    self->data_pin_mask = 1 << (pin_index % 8);
    self->data_pin = data_pin->number;
    port->OUTCLR.reg = 1 << pin_index;

    set_eic_channel_data(clock_pin->extint_channel, (void*) self);

    // Check to see if the EIC is enabled and start it up if its not.'
    if (eic_get_enable() == 0) {
        turn_on_external_interrupt_controller();
    }

    gpio_set_pin_function(clock_pin->number, GPIO_PIN_FUNCTION_A);

    // Set clock to output 0 from PORT for when the PMUX is not enabled.
    PortGroup *const clock_port = &PORT->Group[(enum gpio_port)GPIO_PORT(clock_pin->number)];
    pin_index = GPIO_PIN(clock_pin->number);
    clock_port->OUTCLR.reg = 1 << pin_index;
    clock_port->DIRSET.reg = 1 << pin_index;
    clock_port->PINCFG[pin_index].bit.DRVSTR = 1;

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


static void update_chars(ps2io_keyboard_obj_t *self, int usb_value) {
    uint8_t ascii = 0;
    switch (usb_value) {
        case 0x4f: // right arrow
            ringbuf_put(&self->out_buf, 0x1b);
            ringbuf_put(&self->out_buf, 0x5b);
            ringbuf_put(&self->out_buf, 0x43);
            return;
        case 0x50: // left arrow
            ringbuf_put(&self->out_buf, 0x1b);
            ringbuf_put(&self->out_buf, 0x5b);
            ringbuf_put(&self->out_buf, 0x44);
            return;
        case 0x51: // down arrow
            ringbuf_put(&self->out_buf, 0x1b);
            ringbuf_put(&self->out_buf, 0x5b);
            ringbuf_put(&self->out_buf, 0x42);
            return;
        case 0x52: // up arrow
            ringbuf_put(&self->out_buf, 0x1b);
            ringbuf_put(&self->out_buf, 0x5b);
            ringbuf_put(&self->out_buf, 0x41);
            return;
        case 0x2c: // space
            ascii = 0x20;
            break;
        case 0x2a: // backspace
            ascii = 0x08;
            break;
        case 0x2b: // tab
            ascii = 0x09;
            break;
        case 0x35: // `
            ascii = 0x60;
            break;
        case 0x1e: // 1
            ascii = 0x31;
            break;
        case 0x1f: // 2
            ascii = 0x32;
            break;
        case 0x20: // 3
            ascii = 0x33;
            break;
        case 0x21: // 4
            ascii = 0x34;
            break;
        case 0x22: // 5
            ascii = 0x35;
            break;
        case 0x23: // 6
            ascii = 0x36;
            break;
        case 0x24: // 7
            ascii = 0x37;
            break;
        case 0x25: // 8
            ascii = 0x38;
            break;
        case 0x26: // 9
            ascii = 0x39;
            break;
        case 0x27: // 0
            ascii = 0x30;
            break;
        case 0x2d: // -
            ascii = 0x2d;
            break;
        case 0x2e: // =
            ascii = 0x3d;
            break;
        case 0x14: // Q
            ascii = 0x71;
            break;
        case 0x1a: // W
            ascii = 0x77;
            break;
        case 0x08: // E
            ascii = 0x65;
            break;
        case 0x15: // R
            ascii = 0x72;
            break;
        case 0x17: // T
            ascii = 0x74;
            break;
        case 0x1c: // Y
            ascii = 0x79;
            break;
        case 0x18: // U
            ascii = 0x75;
            break;
        case 0x0c: // I
            ascii = 0x69;
            break;
        case 0x12: // O
            ascii = 0x6f;
            break;
        case 0x13: // P
            ascii = 0x70;
            break;
        case 0x2f: // [
            ascii = 0x5b;
            break;
        case 0x30: // ]
            ascii = 0x5d;
            break;
        case 0x31: // backslash
            ascii = 0x5c;
            break;
        case 0x04: // A
            ascii = 0x61;
            break;
        case 0x16: // S
            ascii = 0x73;
            break;
        case 0x07: // D
            ascii = 0x64;
            break;
        case 0x09: // F
            ascii = 0x66;
            break;
        case 0x0a: // G
            ascii = 0x67;
            break;
        case 0x0b: // H
            ascii = 0x68;
            break;
        case 0x0d: // J
            ascii = 0x6a;
            break;
        case 0x0e: // K
            ascii = 0x6b;
            break;
        case 0x0f: // L
            ascii = 0x6c;
            break;
        case 0x33: // ;
            ascii = 0x3b;
            break;
        case 0x34: // '
            ascii = 0x27;
            break;
        case 0x28: // enter
            ringbuf_put(&self->out_buf, 0x0d);
            ringbuf_put(&self->out_buf, 0x0a);
            return;
        case 0x1d: // Z
            ascii = 0x7a;
            break;
        case 0x1b: // X
            ascii = 0x78;
            break;
        case 0x06: // C
            ascii = 0x63;
            break;
        case 0x19: // V
            ascii = 0x76;
            break;
        case 0x05: // B
            ascii = 0x62;
            break;
        case 0x11: // N
            ascii = 0x6e;
            break;
        case 0x10: // M
            ascii = 0x6d;
            break;
        case 0x36: // ,
            ascii = 0x2c;
            break;
        case 0x37: // .
            ascii = 0x2e;
            break;
        case 0x38: // /
            ascii = 0x2f;
            break;
    }
    // Do colemak before handling shift because shift has the same effect.
    if (self->colemak) {
        switch (ascii) {
            case 0x65: // E -> F
                ascii = 0x66;
                break;
            case 0x72: // R -> P
                ascii = 0x70;
                break;
            case 0x74: // T -> G
                ascii = 0x67;
                break;
            case 0x79: // Y -> J
                ascii = 0x6a;
                break;
            case 0x66: // F -> T
                ascii = 0x74;
                break;
            case 0x75: // U -> L
                ascii = 0x6c;
                break;
            case 0x69: // I -> U
                ascii = 0x75;
                break;
            case 0x6f: // O -> Y
                ascii = 0x79;
                break;
            case 0x70: // P -> ;
                ascii = 0x3b;
                break;
            case 0x73: // S -> R
                ascii = 0x72;
                break;
            case 0x64: // D -> S
                ascii = 0x73;
                break;
            case 0x67: // G -> D
                ascii = 0x64;
                break;
            case 0x6a: // J -> N
                ascii = 0x6e;
                break;
            case 0x6b: // K -> E
                ascii = 0x65;
                break;
            case 0x6c: // L -> I
                ascii = 0x69;
                break;
            case 0x3b: // ; -> O
                ascii = 0x6f;
                break;
            case 0x6e: // N -> K
                ascii = 0x6b;
                break;
        }
    }

    if (self->usb_hid_report[0] & 0x22) { // Shift
        if ('a' <= ascii && ascii <= 'z') {
            ascii = ascii - 0x20;
        } else {
            switch (ascii) {
                case 0x60: // ` -> ~
                    ascii = 0x7e;
                    break;
                case 0x31: // 1 -> !
                    ascii = 0x21;
                    break;
                case 0x32: // 2 -> @
                    ascii = 0x40;
                    break;
                case 0x33: // 3 -> #
                    ascii = 0x23;
                    break;
                case 0x34: // 4 -> $
                    ascii = 0x24;
                    break;
                case 0x35: // 5 -> %
                    ascii = 0x25;
                    break;
                case 0x36: // 6 -> ^
                    ascii = 0x5e;
                    break;
                case 0x37: // 7 -> &
                    ascii = 0x26;
                    break;
                case 0x38: // 8 -> *
                    ascii = 0x2a;
                    break;
                case 0x39: // 9 -> (
                    ascii = 0x28;
                    break;
                case 0x30: // 0 -> )
                    ascii = 0x29;
                    break;
                case 0x2d: // - -> _
                    ascii = 0x5f;
                    break;
                case 0x6c: // = -> +
                    ascii = 0x69;
                    break;
                case 0x5b: // [ -> {
                    ascii = 0x7b;
                    break;
                case 0x5d: // ] -> }
                    ascii = 0x7d;
                    break;
                case 0x5c: // backslash -> |
                    ascii = 0x7c;
                    break;
                case 0x3b: // ; -> :
                    ascii = 0x3a;
                    break;
                case 0x27: // ' -> "
                    ascii = 0x22;
                    break;
                case 0x2c: // , -> <
                    ascii = 0x3c;
                    break;
                case 0x2e: // . -> >
                    ascii = 0x3e;
                    break;
                case 0x2f: // / -> ?
                    ascii = 0x3f;
                    break;
            }
        }
    }

    if (self->usb_hid_report[0] & 0x11) { // Control
        switch (ascii) {
            case 0x63: // C -> end of text
                ascii = 0x03;
                break;
            case 0x64: // D -> end of transmit
                ascii = 0x04;
                break;
        }
    }

    if (ascii == 0) {
        return;
    }
    ringbuf_put(&self->out_buf, ascii);
    return;
}

static void send_command(ps2io_keyboard_obj_t *self, uint8_t* command, uint8_t command_len) {
    self->command = 0;
    self->command_bits = 0;
    // Queue bytes backwards
    for (int8_t i = command_len - 1; i >= 0; i--) {
        // Ack bit is 0 set by the host.
        self->command <<= 1;
        self->command |= 1;
        self->command_bits += 1;

        // Stop bit is 1
        self->command <<= 1;
        self->command |= 1;
        self->command_bits += 1;

        // Parity bit is actually set later
        self->command <<= 1;
        self->command_bits += 1;

        uint8_t one_count = 0;
        for (int8_t b = 7; b >= 0; b--) {
            // Parity bit
            self->command <<= 1;
            if ((command[i] & (1 << b)) != 0) {
                self->command |= 1;
                one_count++;
            }
            self->command_bits += 1;
        }

        if (one_count % 2 == 0) {
            self->command |= 1 << 8;
        }

        // Start bit is 0
        self->command <<= 1;
        self->command_bits += 1;
    }
    self->command_acked = true;

    // Wait a little so we don't respond so fast we debounce ourselves.
    common_hal_mcu_delay_us(80);

    // request a host -> device
    // Bring clock low which will trigger the first zero bit.
    gpio_set_pin_function(self->clock_pin, GPIO_PIN_FUNCTION_OFF);

    // Wait 100us
    common_hal_mcu_delay_us(100);

    // Release clock.
    gpio_set_pin_function(self->clock_pin, GPIO_PIN_FUNCTION_A);
}

// Updates the hid report and determines what unicode character to produce to the stream.
static void update_report(ps2io_keyboard_obj_t *self, int ps2_value) {
    if (ps2_value == 0x14) { // Control
        uint8_t bitmask = 0;
        if (self->extended) {
            bitmask = 1 << 4;
        } else {
            bitmask = 1 << 0;
        }
        if (self->break_code) {
            self->usb_hid_report[0] &= ~bitmask;
        } else {
            self->usb_hid_report[0] |= bitmask;
        }
        return;
    }
    if (ps2_value == 0x12 || ps2_value == 0x59) { // Shift
        uint8_t bitmask = 0;
        if (ps2_value == 0x12) { // Left shift
            bitmask = 1 << 1;
        } else {
            bitmask = 1 << 5;
        }
        if (self->break_code) {
            self->usb_hid_report[0] &= ~bitmask;
        } else {
            self->usb_hid_report[0] |= bitmask;
        }
        return;
    }
    if (ps2_value == 0x58) { // Caps lock
        if (!self->colemak && !self->break_code) {
            self->colemak = true;
            self->colemak_new = true;
            uint8_t turn_on_capslock_led[2] = {0xed, 0x04};
            send_command(self, turn_on_capslock_led, 2);
        } else if (self->colemak && self->break_code) {
            if (self->colemak_new) {
                self->colemak_new = false;
            } else {
                self->colemak = false;
                uint8_t turn_off_capslock_led[2] = {0xed, 0x00};
                send_command(self, turn_off_capslock_led, 2);
            }
        }
        return;
    }
    if (ps2_value == 0xfa) { // Unneeded ack.
        return;
    }
    uint8_t keycode = 0;
    if (self->extended) {
            switch (ps2_value) {
                case 0x75: // up arrow
                    keycode = 0x52;
                    break;
                case 0x6b: // left arrow
                    keycode = 0x50;
                    break;
                case 0x74: // right arrow
                    keycode = 0x4f;
                    break;
                // down arrow doesn't work
            }
    } else {
        switch (ps2_value) {
            case 0x72: // down on numpad
                keycode = 0x51;
                break;
            case 0x29: // space
                keycode = 0x2c;
                break;
            case 0x66: // backspace
                keycode = 0x2a;
                break;
            case 0x0d: // Tab
                keycode = 0x2b;
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
                keycode = 0x18;
                break;
            case 0x43: // I
                keycode = 0x0c;
                break;
            case 0x44: // O
                keycode = 0x12;
                break;
            case 0x4D: // P
                keycode = 0x13;
                break;
            case 0x54: // [
                keycode = 0x2f;
                break;
            case 0x5B: // ]
                keycode = 0x30;
                break;
            case 0x5D: // backslash
                keycode = 0x31;
                break;
            case 0x1C: // A
                keycode = 0x04;
                break;
            case 0x1B: // S
                keycode = 0x16;
                break;
            case 0x23: // D
                keycode = 0x07;
                break;
            case 0x2B: // F
                keycode = 0x09;
                break;
            case 0x34: // G
                keycode = 0x0a;
                break;
            case 0x33: // H
                keycode = 0x0b;
                break;
            case 0x3B: // J
                keycode = 0x0d;
                break;
            case 0x42: // K
                keycode = 0x0e;
                break;
            case 0x4B: // L
                keycode = 0x0f;
                break;
            case 0x4C: // ;
                keycode = 0x33;
                break;
            case 0x52: // '
                keycode = 0x34;
                break;
            case 0x5A: // enter
                keycode = 0x28;
                break;
            case 0x1A: // Z
                keycode = 0x1d;
                break;
            case 0x22: // X
                keycode = 0x1b;
                break;
            case 0x21: // C
                keycode = 0x06;
                break;
            case 0x2A: // V
                keycode = 0x19;
                break;
            case 0x32: // B
                keycode = 0x05;
                break;
            case 0x31: // N
                keycode = 0x11;
                break;
            case 0x3A: // M
                keycode = 0x10;
                break;
            case 0x41: // ,
                keycode = 0x36;
                break;
            case 0x49: // .
                keycode = 0x37;
                break;
            case 0x4A: // /
                keycode = 0x38;
                break;
        }
    }
    uint8_t first_empty = 8;
    bool repeat = false;
    for (uint8_t i = 2; i < 8; i++) {
        if (first_empty == 8 && self->usb_hid_report[i] == 0) {
            first_empty = i;
        }
        if (self->usb_hid_report[i] == keycode) {
            if (self->break_code) {
                self->usb_hid_report[i] = 0;
            } else {
                repeat = true;
            }
            break;
        }
    }
    if (!self->break_code) {
        if (!repeat && first_empty < 8) {
            self->usb_hid_report[first_empty] = keycode;
        }
        update_chars(self, keycode);
    }
}

// Read characters.
size_t common_hal_ps2io_keyboard_read(ps2io_keyboard_obj_t *self,
uint8_t *data, size_t len, int *errcode) {
    size_t i;
    for (i = 0; i < len;) {
        common_hal_mcu_disable_interrupts();
        int value = ringbuf_get(&self->out_buf);
        common_hal_mcu_enable_interrupts();
        if (value > -1) {
            data[i] = value;
            i++;
            continue;
        }

        common_hal_mcu_disable_interrupts();
        value = ringbuf_get(&self->buf);
        common_hal_mcu_enable_interrupts();
        if (value == -1) {
            break;
        }
        if (value == 0xe0) {
            self->extended = true;
        } else if (value == 0xf0) {
            self->break_code = true;
        } else {
            update_report(self, value);
            self->extended = false;
            self->break_code = false;
        }
    }
    return i;
}

uint32_t common_hal_ps2io_keyboard_bytes_available(ps2io_keyboard_obj_t *self) {
    return ringbuf_count(&self->buf);
}

void common_hal_ps2io_keyboard_move(ps2io_keyboard_obj_t *self, ps2io_keyboard_obj_t *new_location) {

    turn_off_cpu_interrupt(self->channel);
    // Hold onto the new buffer and set it again below after we copy everything else.
    uint8_t* new_buffer = new_location->buf.buf;
    memcpy(new_buffer, self->buf.buf, 256);
    memcpy(new_location, self, sizeof(ps2io_keyboard_obj_t));
    new_location->buf.buf = new_buffer;
    set_eic_channel_data(self->channel, (void*) new_location);
    turn_on_cpu_interrupt(self->channel);

    never_reset_pin_number(self->clock_pin);
    never_reset_pin_number(self->data_pin);
    // never_reset_pin_number(32); // PB00
    never_reset_eic_handler(self->channel);
}
