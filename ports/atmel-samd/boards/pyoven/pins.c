#include "samd51_pins.h"

// This mapping only includes functional names because pins broken
// out on connectors are labeled with their MCU name available from
// microcontroller.pin.
STATIC const mp_map_elem_t board_global_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_RELAY1),  (mp_obj_t)&pin_PB03 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BUZZER),  (mp_obj_t)&pin_PB02 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RELAY2),  (mp_obj_t)&pin_PA00 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TOASTER_5V),  (mp_obj_t)&pin_PB04 },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_global_dict_table);
