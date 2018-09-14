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

#include "shared-bindings/displayio/FourWire.h"

#include <stdint.h>

#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#include "tick.h"

void common_hal_displayio_fourwire_construct(displayio_fourwire_obj_t* self,
    const mcu_pin_obj_t* clock, const mcu_pin_obj_t* data, const mcu_pin_obj_t* command,
    const mcu_pin_obj_t* chip_select, const mcu_pin_obj_t* reset) {

    common_hal_busio_spi_construct(&self->bus, clock, data, mp_const_none);
    common_hal_digitalio_digitalinout_construct(&self->command, command);
    common_hal_digitalio_digitalinout_switch_to_output(&self->command, true, DRIVE_MODE_PUSH_PULL);
    common_hal_digitalio_digitalinout_construct(&self->chip_select, chip_select);
    common_hal_digitalio_digitalinout_switch_to_output(&self->chip_select, true, DRIVE_MODE_PUSH_PULL);

    common_hal_digitalio_digitalinout_construct(&self->reset, reset);
    common_hal_digitalio_digitalinout_switch_to_output(&self->reset, true, DRIVE_MODE_PUSH_PULL);
}

bool common_hal_displayio_fourwire_begin_transaction(displayio_fourwire_obj_t* self) {
    if (!common_hal_busio_spi_try_lock(&self->bus)) {
        return false;
    }
    // TODO(tannewt): Stop hardcoding SPI frequency, polarity and phase.
    common_hal_busio_spi_configure(&self->bus, 12000000, 0, 0, 8);
    common_hal_digitalio_digitalinout_set_value(&self->chip_select, false);
    return true;
}

void common_hal_displayio_fourwire_send(displayio_fourwire_obj_t* self, bool command, uint8_t *data, uint32_t data_length) {
    common_hal_digitalio_digitalinout_set_value(&self->command, !command);
    common_hal_busio_spi_write(&self->bus, data, data_length);
}

void common_hal_displayio_fourwire_end_transaction(displayio_fourwire_obj_t* self) {
    common_hal_digitalio_digitalinout_set_value(&self->chip_select, true);
    common_hal_busio_spi_unlock(&self->bus);
}
