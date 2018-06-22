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

#include "Layer.h"
#include "__init__.h"


// Get the color of the pixel on the layer.
uint16_t get_layer_pixel(layer_obj_t *layer, uint16_t x, uint16_t y) {
    // Shift by the layer's position offset.
    x -= layer->x;
    y -= layer->y;

    // Bounds check.
    if ((x < 0) || (x >= layer->width * layer->graphic->sprite_width) ||
            (y < 0) || (y >= layer->height * layer->graphic->sprite_height)) {
        return TRANSPARENT;
    }

    // Get the tile from the grid location or from sprite frame.
    uint8_t frame;
    if (layer->map) {
        uint8_t tx = x / layer->graphic->sprite_width;
        uint8_t ty = y / layer->graphic->sprite_height;

        frame = layer->map[(ty * layer->width + tx) / layer->tiles_per_byte];
        uint8_t bits_per_tile = 8 / layer->tiles_per_byte;
        uint8_t position_in_tile = (ty * layer->width + tx) % layer->tiles_per_byte;
        frame = (frame >> (position_in_tile * bits_per_tile)) & ~(0xff << bits_per_tile);
    } else {
        frame = layer->frame;
    }

    // Get the value of the pixel.
    uint8_t pixel = get_sprite_pixel(layer->graphic, frame, x, y, layer->flip_x, layer->flip_y);

    // Convert to 16-bit color using the palette.
    return layer->palette[pixel];
}
