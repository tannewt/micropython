/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Radomir Dopieralski
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

#include "SpriteSheet.h"
#include "__init__.h"

void shared_module__stage_sprite_sheet_construct(_stage_sprite_sheet_obj_t *self,
    uint8_t* data, uint32_t data_len,
    uint8_t sheet_width, uint8_t sheet_height,
    uint8_t bits_per_pixel,
    uint8_t sprite_width, uint8_t sprite_height) {
    self->data = data;
    self->bits_per_pixel = bits_per_pixel;
    self->sprite_width = sprite_width;
    self->sprite_height = sprite_height;
    self->sheet_width = sheet_width;
    self->sheet_height = sheet_height;
    self->pixels_per_byte = 8 / self->bits_per_pixel;
    self->pixel_mask = 0xff >> (8 - self->bits_per_pixel);
}

uint16_t get_sprite_pixel(_stage_sprite_sheet_obj_t *self, uint16_t sprite, uint16_t x, uint16_t y,
                            bool flip_x, bool flip_y) {
    // Get the position within the tile.
    x %= self->sprite_width;
    y %= self->sprite_height;

    if (flip_x) {
        x = self->sprite_width - x - 1;
    }

    if (flip_y) {
        y = self->sprite_width - y - 1;
    }

    uint32_t sprite_row = sprite / self->sheet_width;
    uint32_t sprite_col = sprite % self->sheet_width;
    uint32_t row_length_bytes = self->sheet_width * self->sprite_width / self->pixels_per_byte;
    // Start of the row.
    uint32_t sprite_start = sprite_row * row_length_bytes * self->sprite_height;
    sprite_start += sprite_col * self->sprite_width / self->pixels_per_byte;

    // Get the value of the pixel.
    uint8_t pixel = self->data[sprite_start + y * row_length_bytes + x / self->pixels_per_byte];

    uint8_t pixels_per_byte = 8 / self->bits_per_pixel;
    uint8_t pixel_mask = 0xff >> (8 - self->bits_per_pixel);

    return (pixel >> ((x % pixels_per_byte) * self->bits_per_pixel)) & pixel_mask;
}
