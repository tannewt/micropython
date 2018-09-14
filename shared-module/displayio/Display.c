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

#include "shared-bindings/displayio/Display.h"

#include <stdint.h>

#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/displayio/FourWire.h"

#include "tick.h"

void common_hal_displayio_display_construct(displayio_display_obj_t* self,
        mp_obj_t bus, uint16_t width, uint16_t height, int16_t colstart, int16_t rowstart,
        uint16_t color_depth, uint8_t set_column_command, uint8_t set_row_command,
        uint8_t write_ram_command) {
    self->bus = bus;
    self->width = width;
    self->height = height;
    self->color_depth = color_depth;
    self->set_column_command = set_column_command;
    self->set_row_command = set_row_command;
    self->write_ram_command = write_ram_command;
    self->current_group = NULL;
    self->colstart = colstart;
    self->rowstart = rowstart;
}

void common_hal_displayio_display_show(displayio_display_obj_t* self, displayio_group_t* root_group) {
    self->current_group = root_group;
    common_hal_displayio_display_refresh_soon(self);
}

void common_hal_displayio_display_refresh_soon(displayio_display_obj_t* self) {
    self->refresh = true;
}

int32_t common_hal_displayio_display_wait_for_frame(displayio_display_obj_t* self) {
    uint64_t last_refresh = self->last_refresh;
    while (last_refresh == self->last_refresh) {
        MICROPY_VM_HOOK_LOOP
    }
    return 0;
}

bool displayio_display_frame_queued(displayio_display_obj_t* self) {
    // Refresh at ~30 fps.
    return (ticks_ms - self->last_refresh) > 32;
}

bool displayio_display_refresh_queued(displayio_display_obj_t* self) {
    return self->refresh || (self->current_group != NULL && displayio_group_needs_refresh(self->current_group));
}

void displayio_display_finish_refresh(displayio_display_obj_t* self) {
    if (self->current_group != NULL) {
        displayio_group_finish_refresh(self->current_group);
    }
    self->refresh = false;
    self->last_refresh = ticks_ms;
}

void displayio_display_start_region_update(displayio_display_obj_t* self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // TODO(tannewt): Handle displays with single byte bounds.
    common_hal_displayio_fourwire_begin_transaction(self->bus);
    uint16_t data[2];
    common_hal_displayio_fourwire_send(self->bus, true, &self->set_column_command, 1);
    data[0] = __builtin_bswap16(x0 + self->colstart);
    data[1] = __builtin_bswap16(x1-1 + self->colstart);
    common_hal_displayio_fourwire_send(self->bus, false, (uint8_t*) data, 4);
    common_hal_displayio_fourwire_send(self->bus, true, &self->set_row_command, 1);
    data[0] = __builtin_bswap16(y0 + 1 + self->rowstart);
    data[1] = __builtin_bswap16(y1 + self->rowstart);
    common_hal_displayio_fourwire_send(self->bus, false, (uint8_t*) data, 4);
    common_hal_displayio_fourwire_send(self->bus, true, &self->write_ram_command, 1);
}

bool displayio_display_send_pixels(displayio_display_obj_t* self, uint32_t* pixels, uint32_t length) {
    // TODO: Set this up so its async and 32 bit DMA transfers.
    common_hal_displayio_fourwire_send(self->bus, false, (uint8_t*) pixels, length*4);
    return true;
}

void displayio_display_finish_region_update(displayio_display_obj_t* self) {
    common_hal_displayio_fourwire_end_transaction(self->bus);
}

void displayio_refresh_display(void) {
    displayio_display_obj_t* display = &board_display_obj;

    if (!displayio_display_frame_queued(display)) {
        return;
    }
    if (displayio_display_refresh_queued(display)) {
        // We compute the pixels
        uint16_t x0 = 0;
        uint16_t y0 = 0;
        uint16_t x1 = display->width;
        uint16_t y1 = display->height;
        size_t index = 0;
        //size_t row_size = (x1 - x0);
        uint16_t buffer_size = 256;
        uint32_t buffer[buffer_size / 2];
        displayio_display_start_region_update(display, x0, y0, x1, y1);
        for (uint16_t y = y0; y < y1; ++y) {
            for (uint16_t x = x0; x < x1; ++x) {
                uint16_t* pixel = &(((uint16_t*)buffer)[index]);
                *pixel = 0;
                if (display->current_group != NULL) {
                    displayio_group_get_pixel(display->current_group, x, y, pixel);
                }


                index += 1;
                // The buffer is full, send it.
                if (index >= buffer_size) {
                    if (!displayio_display_send_pixels(display, buffer, buffer_size / 2)) {
                        displayio_display_finish_region_update(display);
                        return;
                    }
                    index = 0;
                }
            }
        }
        // Send the remaining data.
        if (index && !displayio_display_send_pixels(display, buffer, index * 2)) {
            displayio_display_finish_region_update(display);
            return;
        }
        displayio_display_finish_region_update(display);
    }
    displayio_display_finish_refresh(display);
}
