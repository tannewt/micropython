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

#include <stdint.h>

#include "py/binary.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/gbio/__init__.h"

//| :mod:`gbio` --- Interface with GameBoy hardware
//| =========================================================================
//|
//| .. module:: gbio
//|   :synopsis: Interface with GameBoy hardware
//|   :platform: Gameboy
//|
//| The `gbio` module contains classes to manage GameBoy hardware
//|
//| .. warning:: This will be changed before 4.0.0. Consider it very experimental.
//|
//| Libraries
//|
//| .. toctree::
//|     :maxdepth: 3
//|

//| .. method:: queue_commands(instructions)
//|
//|  These instructions are run immediately and this function will block until they finish.
//|
STATIC mp_obj_t gbio_queue_commands(mp_obj_t instructions){
    mp_buffer_info_t bufinfo;
    if (!mp_get_buffer(instructions, &bufinfo, MP_BUFFER_READ)) {
        mp_raise_TypeError(translate("buffer must be a bytes-like object"));
    } else if (bufinfo.typecode != 'B' && bufinfo.typecode != BYTEARRAY_TYPECODE) {
        mp_raise_ValueError(translate("instruction buffer must be a bytearray or array of type 'B'"));
    }
    common_hal_gbio_queue_commands(bufinfo.buf, bufinfo.len);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gbio_queue_commands_obj, gbio_queue_commands);

//| .. method:: queue_vblank_commands(instructions, *, additional_cycles)
//|
//|  These instructions are run at the next vblank. It does not wait for a vblank to occur.
//|
STATIC mp_obj_t gbio_queue_vblank_commands(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_additional_cycles };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_additional_cycles, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t instructions = pos_args[0];
    mp_buffer_info_t bufinfo;
    if (!mp_get_buffer(instructions, &bufinfo, MP_BUFFER_READ)) {
        mp_raise_TypeError(translate("buffer must be a bytes-like object"));
    } else if (bufinfo.typecode != 'B' && bufinfo.typecode != BYTEARRAY_TYPECODE) {
        mp_raise_ValueError(translate("instruction buffer must be a bytearray or array of type 'B'"));
    }
    common_hal_gbio_queue_vblank_commands(bufinfo.buf, bufinfo.len, args[ARG_additional_cycles].u_int);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(gbio_queue_vblank_commands_obj, 1, gbio_queue_vblank_commands);

//| .. method:: set_lcdc(value)
//|
//|  Sets the value of the LCD control register.
//|
STATIC mp_obj_t gbio_set_lcdc(mp_obj_t value_obj){
    common_hal_gbio_set_lcdc(MP_OBJ_SMALL_INT_VALUE(value_obj));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gbio_set_lcdc_obj, gbio_set_lcdc);

//| .. method:: get_lcdc()
//|
//|  Returns the value of the LCDC register.
//|
STATIC mp_obj_t gbio_get_lcdc(void){
    return MP_OBJ_NEW_SMALL_INT(common_hal_gbio_get_lcdc());
}
MP_DEFINE_CONST_FUN_OBJ_0(gbio_get_lcdc_obj, gbio_get_lcdc);

//| .. method:: get_pressed()
//|
//|  Returns a bitmask of buttons pressed since the last time the function was called.
//|
STATIC mp_obj_t gbio_get_pressed(void){
    return MP_OBJ_NEW_SMALL_INT(common_hal_gbio_get_pressed());
}
MP_DEFINE_CONST_FUN_OBJ_0(gbio_get_pressed_obj, gbio_get_pressed);

STATIC const mp_rom_map_elem_t gbio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gbio) },
    { MP_ROM_QSTR(MP_QSTR_queue_commands), MP_ROM_PTR(&gbio_queue_commands_obj) },
    { MP_ROM_QSTR(MP_QSTR_queue_vblank_commands), MP_ROM_PTR(&gbio_queue_vblank_commands_obj) },

    { MP_ROM_QSTR(MP_QSTR_set_lcdc), MP_ROM_PTR(&gbio_set_lcdc_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_lcdc), MP_ROM_PTR(&gbio_get_lcdc_obj) },

    { MP_ROM_QSTR(MP_QSTR_get_pressed), MP_ROM_PTR(&gbio_get_pressed_obj) },
};

STATIC MP_DEFINE_CONST_DICT(gbio_module_globals, gbio_module_globals_table);

const mp_obj_module_t gbio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&gbio_module_globals,
};
