/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include <string.h>

#include "lib/utils/context_manager_helpers.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "py/nlr.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/captureio/Capture.h"
#include "shared-bindings/util.h"

//| .. currentmodule:: captureio
//|
//| :class:`Capture` -- capture external pin values
//| ================================================
//|
//| Usage::
//|
//|    import captureio
//|    from board import *
//|
//|    cap = captureio.Capture(CLOCK, A0, A1, A2)
//|    cap.capture()
//|

//| .. class:: Capture(clock_pin, )
//|
//|   Use the AnalogIn on the given pin. The reference voltage varies by
//|   platform so use ``reference_voltage`` to read the configured setting.
//|
//|   :param ~microcontroller.Pin pin: the pin to read from
//|
STATIC mp_obj_t captureio_capture_make_new(const mp_obj_type_t *type,
        mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    // check number of arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    captureio_capture_obj_t *self = m_new_obj(captureio_capture_obj_t);
    self->base.type = &captureio_capture_type;
    common_hal_captureio_capture_construct(self);

    return (mp_obj_t) self;
}

//|   .. method:: deinit()
//|
//|      Turn off the Capture and release the pins for other use.
//|
STATIC mp_obj_t captureio_capture_deinit(mp_obj_t self_in) {
   captureio_capture_obj_t *self = MP_OBJ_TO_PTR(self_in);
   common_hal_captureio_capture_deinit(self);
   return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(captureio_capture_deinit_obj, captureio_capture_deinit);

//|   .. method:: __enter__()
//|
//|      No-op used by Context Managers.
//|
//  Provided by context manager helper.

//|   .. method:: __exit__()
//|
//|      Automatically deinitializes the hardware when exiting a context. See
//|      :ref:`lifetime-and-contextmanagers` for more info.
//|
STATIC mp_obj_t captureio_capture___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_captureio_capture_deinit(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(captureio_capture___exit___obj, 4, 4, captureio_capture___exit__);

//|   .. method:: play(sample, *, loop=False)
//|
//|     Plays the sample once when loop=False and continuously when loop=True.
//|     Does not block. Use `playing` to block.
//|
//|     Sample must be an `audioio.WaveFile` or `audioio.RawSample`.
//|
//|     The sample itself should consist of 8 bit or 16 bit samples.
//|
STATIC mp_obj_t captureio_capture_capture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_buffer };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buffer,    MP_ARG_OBJ | MP_ARG_REQUIRED },
    };
    captureio_capture_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    raise_error_if_deinited(common_hal_captureio_capture_deinited(self));
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t bufinfo;
    if (mp_get_buffer(args[ARG_buffer].u_obj, &bufinfo, MP_BUFFER_WRITE) && bufinfo.typecode == 'L') {
        common_hal_captureio_capture_capture(self, bufinfo.buf, bufinfo.len);
    } else {
        mp_raise_ValueError("buffer must be an array of type 'L'");
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(captureio_capture_capture_obj, 1, captureio_capture_capture);

STATIC const mp_rom_map_elem_t captureio_capture_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit),             MP_ROM_PTR(&captureio_capture_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),          MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),           MP_ROM_PTR(&captureio_capture___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_capture),           MP_ROM_PTR(&captureio_capture_capture_obj) },
};

STATIC MP_DEFINE_CONST_DICT(captureio_capture_locals_dict, captureio_capture_locals_dict_table);

const mp_obj_type_t captureio_capture_type = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogIn,
    .make_new = captureio_capture_make_new,
    .locals_dict = (mp_obj_t)&captureio_capture_locals_dict,
};
