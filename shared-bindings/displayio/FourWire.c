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
    mp_raise_NotImplementedError(translate("displayio is a work in progress"));
    return mp_const_none;
}


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
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&displayio_fourwire_send_obj) },
};
STATIC MP_DEFINE_CONST_DICT(displayio_fourwire_locals_dict, displayio_fourwire_locals_dict_table);

const mp_obj_type_t displayio_fourwire_type = {
    { &mp_type_type },
    .name = MP_QSTR_FourWire,
    .make_new = displayio_fourwire_make_new,
    .locals_dict = (mp_obj_dict_t*)&displayio_fourwire_locals_dict,
};
