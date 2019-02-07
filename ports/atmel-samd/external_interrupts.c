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

#include "common-hal/ps2io/Keyboard.h"
#include "common-hal/pulseio/PulseIn.h"
#include "common-hal/rotaryio/IncrementalEncoder.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "samd/external_interrupts.h"
#include "external_interrupts.h"

#include "sam.h"

// This structure is used to share per-channel storage amongst all users of external interrupts.
// Without this there would be multiple arrays even though they are disjoint because each channel
// has one user.
static void *channel_data[EIC_EXTINT_NUM];
static uint8_t channel_handler[EIC_EXTINT_NUM];
static uint16_t never_reset_channels;

void reset_all_eic_handlers(void) {
    for (int i = 0; i < EIC_EXTINT_NUM; i++) {
        if ((never_reset_channels & (1 << i)) != 0) {
            continue;
        }
        turn_off_eic_channel(i);
        channel_data[i] = NULL;
        channel_handler[i] = 0;
    }
}

void never_reset_eic_handler(uint8_t eic_channel) {
    never_reset_channels |= 1 << eic_channel;
}

bool eic_handler_free(uint8_t eic_channel) {
    return channel_data[eic_channel] == NULL && eic_channel_free(eic_channel);
}

void external_interrupt_handler(uint8_t channel) {
    uint8_t handler = channel_handler[channel];
    if (handler == EIC_HANDLER_PULSEIN) {
        pulsein_interrupt_handler(channel);
    } else if (handler == EIC_HANDLER_INCREMENTAL_ENCODER) {
        incrementalencoder_interrupt_handler(channel);
    } else if (handler == EIC_HANDLER_PS2IO) {
        ps2io_interrupt_handler(channel);
    }
    clear_external_interrupt(channel);
}

void enable_eic_channel_handler(uint8_t eic_channel, uint32_t sense_setting,
                                uint8_t channel_interrupt_handler) {
    turn_on_eic_channel(eic_channel, sense_setting);
    if (channel_interrupt_handler != EIC_HANDLER_NO_INTERRUPT) {
        channel_handler[eic_channel] = channel_interrupt_handler;
        turn_on_cpu_interrupt(eic_channel);
    }
}

void disable_eic_channel_handler(uint8_t eic_channel) {
    never_reset_channels &= ~(1 << eic_channel);
    turn_off_eic_channel(eic_channel);
    channel_data[eic_channel] = NULL;
    channel_handler[eic_channel] = EIC_HANDLER_NO_INTERRUPT;
}

void* get_eic_channel_data(uint8_t eic_channel) {
    return channel_data[eic_channel];
}

void set_eic_channel_data(uint8_t eic_channel, void* data) {
    channel_data[eic_channel] = data;
}
