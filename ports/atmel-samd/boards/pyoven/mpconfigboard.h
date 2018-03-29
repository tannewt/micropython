#define MICROPY_HW_BOARD_NAME "pyoven"
#define MICROPY_HW_MCU_NAME "samd51j20"

#define CIRCUITPY_MCU_FAMILY samd51

// These are pins not to reset.
#define MICROPY_PORT_A        (PORT_PA27)
#define MICROPY_PORT_B        (0)
#define MICROPY_PORT_C        (0)
#define MICROPY_PORT_D        (0)

#define AUTORESET_DELAY_MS 500

#include "internal_flash.h"

// If you change this, then make sure to update the linker scripts as well to
// make sure you don't overwrite code
// #define CIRCUITPY_INTERNAL_NVM_SIZE 256
#define CIRCUITPY_INTERNAL_NVM_SIZE 0

#define BOARD_FLASH_SIZE (FLASH_SIZE - 0x4000 - CIRCUITPY_INTERNAL_NVM_SIZE)
