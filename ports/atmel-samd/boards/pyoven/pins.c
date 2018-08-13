#include "shared-bindings/board/__init__.h"

// This mapping only includes functional names because pins broken
// out on connectors are labeled with their MCU name available from
// microcontroller.pin.
STATIC const mp_map_elem_t board_global_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_FAR_LEFT_LED),  (mp_obj_t)&pin_PA15 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LEFT_LED),  (mp_obj_t)&pin_PB11 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RIGHT_LED),  (mp_obj_t)&pin_PA07 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FAR_RIGHT_LED),  (mp_obj_t)&pin_PA04 },

    { MP_OBJ_NEW_QSTR(MP_QSTR_RELAY1),  (mp_obj_t)&pin_PB03 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BUZZER),  (mp_obj_t)&pin_PB02 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RELAY2),  (mp_obj_t)&pin_PA00 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TOASTER_5V),  (mp_obj_t)&pin_PB04 },

    { MP_OBJ_NEW_QSTR(MP_QSTR_DOTSTAR_CLK),  (mp_obj_t)&pin_PB13 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DOTSTAR_DATA),  (mp_obj_t)&pin_PB15 },

    { MP_OBJ_NEW_QSTR(MP_QSTR_TEMP),  (mp_obj_t)&pin_PA17 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_UNITS),  (mp_obj_t)&pin_PA16 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FROZEN),  (mp_obj_t)&pin_PA13 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TIME),  (mp_obj_t)&pin_PA06 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_START),  (mp_obj_t)&pin_PB14 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MORE),  (mp_obj_t)&pin_PB12 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_INC),  (mp_obj_t)&pin_PA09 },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DEC),  (mp_obj_t)&pin_PA10 },

    { MP_OBJ_NEW_QSTR(MP_QSTR_TEMP_MEASURE),  (mp_obj_t)&pin_PB00 },

    { MP_OBJ_NEW_QSTR(MP_QSTR_PUMP),  (mp_obj_t)&pin_PB22 },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_global_dict_table);
