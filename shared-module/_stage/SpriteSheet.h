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

#ifndef MICROPY_INCLUDED_SHARED_MODULE__STAGE_SPRITESHEET_H
#define MICROPY_INCLUDED_SHARED_MODULE__STAGE_SPRITESHEET_H

#include <stdint.h>
#include <stdbool.h>

#include "py/obj.h"

typedef struct {
    mp_obj_base_t base;
    uint8_t *data;
    // Dictates the number of colors in the palette as well.
    uint8_t bits_per_pixel;
    uint8_t sprite_width;
    uint8_t sprite_height;
    uint8_t sheet_width;
    uint8_t sheet_height;
    uint8_t pixels_per_byte;
    uint8_t pixel_mask;
} _stage_sprite_sheet_obj_t;


void shared_module__stage_sprite_sheet_construct(_stage_sprite_sheet_obj_t *sprite_sheet,
    uint8_t* data, uint32_t data_len,
    uint8_t sheet_width, uint8_t sheet_height,
    uint8_t bits_per_pixel,
    uint8_t sprite_width, uint8_t sprite_height);

uint16_t get_sprite_pixel(_stage_sprite_sheet_obj_t *sprite_sheet,
    uint16_t sprite, uint16_t x, uint16_t y, bool flip_x, bool flip_y);

#endif  // MICROPY_INCLUDED_SHARED_MODULE__STAGE_SPRITESHEET_H
