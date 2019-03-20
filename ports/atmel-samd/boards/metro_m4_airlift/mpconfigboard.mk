LD_FILE = boards/samd51x19-bootloader-external-flash.ld
USB_VID = 0x239A
USB_PID = 0x8037
USB_PRODUCT = "Metro M4 Airlift"
USB_MANUFACTURER = "Adafruit Industries LLC"

QSPI_FLASH_FILESYSTEM = 1

# Exact flash hasn't been decided yet. This is simply the chip on Scott's prototype.
EXTERNAL_FLASH_DEVICE_COUNT = 1
EXTERNAL_FLASH_DEVICES = "GD25Q64C"

LONGINT_IMPL = MPZ

# No touch on SAMD51 yet
CIRCUITPY_TOUCHIO = 0

CHIP_VARIANT = SAMD51J19A
CHIP_FAMILY = samd51

CIRCUITPY_NETWORK = 1
MICROPY_PY_WIZNET5K = 5500
