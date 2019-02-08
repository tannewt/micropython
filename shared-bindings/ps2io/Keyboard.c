/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include <stdint.h>

#include "shared-bindings/ps2io/Keyboard.h"
#include "shared-bindings/util.h"

#include "py/ioctl.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "supervisor/shared/translate.h"


//| .. currentmodule:: ps2io
//|
//| :class:`Keyboard` -- receives ps/2 keyboard commands
//| =====================================================
//|
//| .. class:: Keyboard(*, clock, data)
//|
//|   Construct a keyboard class.
//|

STATIC mp_obj_t ps2io_keyboard_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_clock, ARG_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_clock, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_data, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    assert_pin(args[ARG_clock].u_obj, false);
    assert_pin(args[ARG_data].u_obj, false);
    const mcu_pin_obj_t* clock = MP_OBJ_TO_PTR(args[ARG_clock].u_obj);
    assert_pin_free(clock);
    const mcu_pin_obj_t* data = MP_OBJ_TO_PTR(args[ARG_data].u_obj);
    assert_pin_free(data);

    ps2io_keyboard_obj_t *self = m_new_obj(ps2io_keyboard_obj_t);
    self->base.type = &ps2io_keyboard_type;
    common_hal_ps2io_keyboard_construct(self, clock, data, 256);
    return MP_OBJ_FROM_PTR(self);
}

// These are standard stream methods. Code is in py/stream.c.
//
//|   .. method:: read(nbytes=None)
//|
//|     Read characters.  If ``nbytes`` is specified then read at most that many
//|     bytes. Otherwise, read everything that arrives until the connection
//|     times out. Providing the number of bytes expected is highly recommended
//|     because it will be faster.
//|
//|     :return: Data read
//|     :rtype: bytes or None
//|
//|   .. method:: readinto(buf, nbytes=None)
//|
//|     Read bytes into the ``buf``.  If ``nbytes`` is specified then read at most
//|     that many bytes.  Otherwise, read at most ``len(buf)`` bytes.
//|
//|     :return: number of bytes read and stored into ``buf``
//|     :rtype: bytes or None
//|

// These three methods are used by the shared stream methods.
STATIC mp_uint_t ps2io_keyboard_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    ps2io_keyboard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    byte *buf = buf_in;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    return common_hal_ps2io_keyboard_read(self, buf, size, errcode);
}

STATIC mp_uint_t ps2io_keyboard_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    ps2io_keyboard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && common_hal_ps2io_keyboard_bytes_available(self) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_rom_map_elem_t ps2io_keyboard_locals_dict_table[] = {
    // Standard stream methods.
    { MP_OBJ_NEW_QSTR(MP_QSTR_read),     MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ps2io_keyboard_locals_dict, ps2io_keyboard_locals_dict_table);

STATIC const mp_stream_p_t ps2io_keyboard_stream_p = {
    .read = ps2io_keyboard_read,
    .write = NULL,
    .ioctl = ps2io_keyboard_ioctl,
    .is_text = true,
};

const mp_obj_type_t ps2io_keyboard_type = {
    { &mp_type_type },
    .name = MP_QSTR_Keyboard,
    .make_new = ps2io_keyboard_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &ps2io_keyboard_stream_p,
    .locals_dict = (mp_obj_dict_t*)&ps2io_keyboard_locals_dict,
};
