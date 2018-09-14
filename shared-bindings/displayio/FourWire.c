/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include "lib/utils/context_manager_helpers.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"
#include "supervisor/shared/translate.h"

displayio_fourwire_obj_t display_bus;

//| .. currentmodule:: displayio
//|
//| :class:`FourWire` -- Manage communicating to a display over SPI four wire protocol
//| ====================================================================================
//|
//| Manages communication to a display over SPI four wire protocol in the background while Python
//| code runs. The number of created display busses is limited and may return an existing object.
//|
//| .. warning:: This will be changed before 4.0.0. Consider it very experimental.
//|
//| .. class:: FourWire(*, clock, data, command, chip_select, reset)
//|
//|   Create a FourWire object associated with the given pins.
//|
STATIC mp_obj_t displayio_fourwire_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *pos_args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, true);
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, pos_args + n_args);
    enum { ARG_clock, ARG_data, ARG_command, ARG_chip_select, ARG_reset };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_clock, MP_ARG_OBJ | MP_ARG_KW_ONLY },
        { MP_QSTR_data, MP_ARG_OBJ | MP_ARG_KW_ONLY },
        { MP_QSTR_command, MP_ARG_OBJ | MP_ARG_KW_ONLY },
        { MP_QSTR_chip_select, MP_ARG_OBJ | MP_ARG_KW_ONLY },
        { MP_QSTR_reset, MP_ARG_OBJ | MP_ARG_KW_ONLY },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    for (int32_t i = 0; i < 4; i++) {
        if (args[i].u_obj == mp_const_none) {
            mp_raise_ValueError_varg(translate("Must provide %q pin"), allowed_args[i].qst);
        }
    }
    mcu_pin_obj_t* clock = MP_OBJ_TO_PTR(args[ARG_clock].u_obj);
    mcu_pin_obj_t* data = MP_OBJ_TO_PTR(args[ARG_data].u_obj);
    mcu_pin_obj_t* command = MP_OBJ_TO_PTR(args[ARG_command].u_obj);
    mcu_pin_obj_t* chip_select = MP_OBJ_TO_PTR(args[ARG_chip_select].u_obj);
    mcu_pin_obj_t* reset = NULL;
    if (args[ARG_reset].u_obj != mp_const_none) {
        reset = MP_OBJ_TO_PTR(args[ARG_reset].u_obj);
    }

    if (display_bus.base.type != NULL) {
        common_hal_displayio_fourwire_deinit(&display_bus);
    }
    display_bus.base.type = &displayio_fourwire_type;
    common_hal_displayio_fourwire_construct(&display_bus, clock, data, command, chip_select, reset);

    return MP_OBJ_FROM_PTR(&display_bus);
}


//|   .. method:: deinit()
//|
//|      Turn off the DigitalInOut and release the pin for other use.
//|
STATIC mp_obj_t displayio_fourwire_obj_deinit(mp_obj_t self_in) {
    displayio_fourwire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_displayio_fourwire_deinit(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_fourwire_deinit_obj, displayio_fourwire_obj_deinit);

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
STATIC mp_obj_t displayio_fourwire_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_displayio_fourwire_deinit(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(displayio_fourwire_obj___exit___obj, 4, 4, displayio_fourwire_obj___exit__);

//|   .. method:: send(command, data)
//|
//|
STATIC mp_obj_t displayio_fourwire_obj_send(mp_obj_t self_in, mp_obj_t command_in, mp_obj_t data) {
    displayio_fourwire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    uint8_t command = mp_obj_int_get_checked(command_in);
    mp_buffer_info_t bufinfo;
    if (!mp_get_buffer(data, &bufinfo, MP_BUFFER_READ) ||
            !(bufinfo.typecode == 'B' || bufinfo.typecode == BYTEARRAY_TYPECODE)) {
        mp_raise_ValueError(translate("data buffer must be a bytearray or array of type 'B'"));
    }

    if (!common_hal_displayio_fourwire_begin_transaction(self)) {
        mp_raise_RuntimeError(translate("unable to begin transaction"));
    }
    common_hal_displayio_fourwire_send(self, true, &command, 1);
    common_hal_displayio_fourwire_send(self, false, (uint8_t *) bufinfo.buf, bufinfo.len);
    common_hal_displayio_fourwire_end_transaction(self);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(displayio_fourwire_send_obj, displayio_fourwire_obj_send);

STATIC const mp_rom_map_elem_t displayio_fourwire_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_deinit),             MP_ROM_PTR(&displayio_fourwire_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),          MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),           MP_ROM_PTR(&displayio_fourwire_obj___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&displayio_fourwire_send_obj) },
};
STATIC MP_DEFINE_CONST_DICT(displayio_fourwire_locals_dict, displayio_fourwire_locals_dict_table);

const mp_obj_type_t displayio_fourwire_type = {
    { &mp_type_type },
    .name = MP_QSTR_FourWire,
    .make_new = displayio_fourwire_make_new,
    .locals_dict = (mp_obj_dict_t*)&displayio_fourwire_locals_dict,
};
