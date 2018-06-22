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

#include <py/runtime.h>

#include "py/binary.h"

#include "__init__.h"
#include "SpriteSheet.h"

//| .. currentmodule:: _stage
//|
//| :class:`SpriteSheet` -- Keep information about a grid of sprites
//| ===================================================================
//|
//| .. class:: SpriteSheet(data, sheet_width, sheet_height, *, bits_per_pixel=4, sprite_width=8, sprite_width=8)
//|
//|     Keep internal information about a sheet of sprites in a format suitable
//|     for fast rendering with the ``render()`` function. This is similar to a
//|     texture atlas but it requires uniform positioning of sprites.
//|
//|     :param bytearray data: The graphic data of the sprite sheet
//|     :param int sheet_width: The width of the sheet in sprites
//|     :param int sheet_height: The width of the sheet in sprites
//|     :param int bits_per_pixel: The number of bits per pixel. Must be power of two
//|     :param int sprite_width: The width of a single sprite in pixels
//|     :param int sprite_height: The height of a single sprite in pixels
//|
//|     This class is intended for internal use in the ``stage`` library and
//|     it shouldn't be used on its own.
//|
STATIC mp_obj_t sprite_sheet_make_new(const mp_obj_type_t *type, size_t n_args,
        size_t n_kw, const mp_obj_t *pos_args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, true);
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, pos_args + n_args);
    enum { ARG_data,
           ARG_sheet_width,
           ARG_sheet_height,
           ARG_bits_per_pixel,
           ARG_sprite_width,
           ARG_sprite_height,
            };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_sheet_width, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_sheet_height, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_bits_per_pixel, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 4 } },
        { MP_QSTR_sprite_width, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 8 } },
        { MP_QSTR_sprite_height, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 8 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t bufinfo;
    if (!mp_get_buffer(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ) || bufinfo.typecode != BYTEARRAY_TYPECODE) {
        mp_raise_ValueError("Data must be bytearray");
    }

    uint8_t sheet_width = args[ARG_sheet_width].u_int;
    uint8_t sheet_height = args[ARG_sheet_height].u_int;
    uint8_t bits_per_pixel = args[ARG_bits_per_pixel].u_int;
    uint8_t sprite_width = args[ARG_sprite_width].u_int;
    uint8_t sprite_height = args[ARG_sprite_height].u_int;

    uint32_t pixel_width = (uint32_t) sheet_width * sprite_width;
    uint32_t pixel_height = (uint32_t) sheet_height * sprite_height;
    uint32_t pixel_length = pixel_width * pixel_height;
    uint32_t byte_length = (pixel_length * bits_per_pixel) / 8;

    if (bufinfo.len != byte_length) {
        mp_raise_ValueError("data length doesn't match dimensions");
    }

    _stage_sprite_sheet_obj_t *self = m_new_obj(_stage_sprite_sheet_obj_t);
    self->base.type = &mp_type__stage_sprite_sheet;
    shared_module__stage_sprite_sheet_construct(self, ((uint8_t*)bufinfo.buf), bufinfo.len,
        args[ARG_sheet_width].u_int, args[ARG_sheet_height].u_int,
        args[ARG_bits_per_pixel].u_int,
        args[ARG_sprite_width].u_int, args[ARG_sprite_height].u_int);

    return MP_OBJ_FROM_PTR(self);
}

// STATIC const mp_rom_map_elem_t sprite_sheet_locals_dict_table[] = {
// };
// STATIC MP_DEFINE_CONST_DICT(sprite_sheet_dict, sprite_sheet_dict_table);

const mp_obj_type_t mp_type__stage_sprite_sheet = {
    { &mp_type_type },
    .name = MP_QSTR_SpriteSheet,
    .make_new = sprite_sheet_make_new,
    //.locals_dict = (mp_obj_dict_t*)&sprite_sheet_locals_dict,
};
