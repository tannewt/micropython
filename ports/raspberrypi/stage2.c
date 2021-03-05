#include "sdk/src/rp2040/hardware_structs/include/hardware/structs/ssi.h"
#include "sdk/src/rp2040/hardware_regs/include/hardware/regs/addressmap.h"
#include "sdk/src/rp2040/hardware_regs/include/hardware/regs/m0plus.h"


    // io_rw_32 ctrlr0;
    // io_rw_32 ctrlr1;
    // io_rw_32 ssienr;
    // io_rw_32 mwcr;
    // io_rw_32 ser;
    // io_rw_32 baudr;
    // io_rw_32 txftlr;
    // io_rw_32 rxftlr;
    // io_rw_32 txflr;
    // io_rw_32 rxflr;
    // io_rw_32 sr;
    // io_rw_32 imr;
    // io_rw_32 isr;
    // io_rw_32 risr;
    // io_rw_32 txoicr;
    // io_rw_32 rxoicr;
    // io_rw_32 rxuicr;
    // io_rw_32 msticr;
    // io_rw_32 icr;
    // io_rw_32 dmacr;
    // io_rw_32 dmatdlr;
    // io_rw_32 dmardlr;
    // io_rw_32 idr;
    // io_rw_32 ssi_version_id;
    // io_rw_32 dr0;
    // uint32_t _pad[(0xf0 - 0x60) / 4 - 1];
    // io_rw_32 rx_sample_dly;
    // io_rw_32 spi_ctrlr0;
    // io_rw_32 txd_drive_edge;

#define CMD_READ 0x03
#define ADDR_L 6 // * 4 = 24 bit address

// This function is use by the bootloader to enable the XIP flash. It is also
// used by the SDK to reinit XIP after doing non-read flash interactions such as
// writing or erasing. This code must compile down to position independent
// assembly because we don't know where in RAM it'll be when run.
void _stage2_boot(void) {
    uint32_t lr;
    asm volatile ("MOV %0, LR\n" : "=r" (lr) );

    // Disable the SSI so we can change the settings.
    ssi_hw->ssienr = 0;

    ssi_hw->baudr = 4; // 125 mhz / clock divider

    ssi_hw->ctrlr0 = (SSI_CTRLR0_SPI_FRF_VALUE_STD << SSI_CTRLR0_SPI_FRF_LSB) |  /* Standard 1-bit SPI serial frames */ \
                     (31 << SSI_CTRLR0_DFS_32_LSB)  |                            /* 32 clocks per data frame */ \
                     (SSI_CTRLR0_TMOD_VALUE_EEPROM_READ  << SSI_CTRLR0_TMOD_LSB); /* Send instr + addr, receive data */

    // SPI specific settings
    ssi_hw->spi_ctrlr0 = (CMD_READ << SSI_SPI_CTRLR0_XIP_CMD_LSB) |        /* Value of instruction prefix */ \
                         (ADDR_L << SSI_SPI_CTRLR0_ADDR_L_LSB) |           /* Total number of address + mode bits */ \
                         (2 << SSI_SPI_CTRLR0_INST_L_LSB) |                /* 8 bit command prefix (field value is bits divided by 4) */ \
                         (SSI_SPI_CTRLR0_TRANS_TYPE_VALUE_1C1A << SSI_SPI_CTRLR0_TRANS_TYPE_LSB); /* command and address both in serial format */

    ssi_hw->ctrlr1 = 0; // Single 32b read

    // Re-enable the SSI
    ssi_hw->ssienr = 1;

    // If lr is 0, then we came from the bootloader.
    if (lr == 0) {
        uint32_t* vector_table = (uint32_t*) (XIP_BASE + 0x100);
        // Switch the vector table to immediately after the stage 2 area.
        *((uint32_t *) (PPB_BASE + M0PLUS_VTOR_OFFSET)) = (uint32_t) vector_table;
        // Set the top of the stack according to the vector table.
        asm volatile ("MSR msp, %0" : : "r" (vector_table[0]) : );
        // The reset handler is the second entry in the vector table
        asm volatile ("bx %0" : : "r" (vector_table[1]) : );
        // Doesn't return. It jumps to the reset handler instead.
    }
    // Otherwise we return.
}
