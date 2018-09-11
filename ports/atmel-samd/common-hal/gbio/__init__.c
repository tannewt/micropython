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

#include "shared-bindings/gbio/__init__.h"

#include <string.h>

#include "hal/include/hal_gpio.h"

#include "atmel_start_pins.h"

#include "audio_dma.h"
#include "common-hal/pulseio/PWMOut.h"
#include "peripherals/samd/clocks.h"
#include "peripherals/samd/events.h"
#include "peripherals/samd/timers.h"
#include "py/runtime.h"
#include "supervisor/shared/translate.h"

uint8_t dma_out_channel;
uint8_t command_cache[512];
const uint8_t gameboy_boot[] = {
                                // Adafruit
                                0x00, 0x30, 0x00, 0xC6, 0x00, 0x07, 0xCC, 0xCC, 0x00, 0xF1, 0x13, 0x3B, 0xC0, 0xD1, 0x00, 0xBD,
                                0x00, 0x66, 0x00, 0x66, 0xC1, 0xDD, 0x08, 0xE8, 0x36, 0x63, 0xE6, 0xE6, 0xCC, 0xC7, 0xCD, 0xDC,
                                0xF9, 0xBD, 0xBB, 0xBB, 0x11, 0x11, 0x88, 0x88, 0x66, 0x63, 0x66, 0xE6, 0xDD, 0xDD, 0x88, 0x8E,

                                // Nintendo logo 48 bytes
                                0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
                                0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
                                0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,

                                // Cartridge header 28 bytes
                                0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xCA, 0x31, 0x58,

                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

                                // Set the stack pointer
                                0x31, 0x00, 0xe0,

                                0x00, 0x00,

                                // Set both lines low to detect any press.
                                0xf0, 0x00, // LD A, FF00 - Poke the gpio by reading from it
                                0x3e, 0x00, // LD A, 0x30
                                0xe0, 0x00, // LD FF00, A

                                0x00, 0x00,

                                // Clear vsync and joypad interrupts
                                0x3e, 0x00, // LD A, 0x0
                                0xe0, 0x0f, // LD ff0f, A

                                // Enable vsync and joypad interrupts
                                0x3e, 0x11, // LD A, 0x11 (vblank is bit 0)
                                0xe0, 0xff, // LD ffff, A

                                0xfb, // enable interrupts

                                // Load 0x1000 into hl and repeatedly jump to it until we do
                                // something else. This prevents the program counter from
                                // exiting the cartridge address range.
                                0x21, 0x00, 0x10, 0xe9};

uint8_t gamepad_state = 0xff;
uint16_t addresses[2048];
uint16_t address_i;
uint32_t vsync_count = 0;
uint32_t gamepad_count = 0;
bool everything_going = false;
uint8_t vsync_interrupt_response[] = {0xe9, 0xe9, 0x00, // Jump out of interrupt territory
                             0x3e, 0x00, 0xe0, 0x43,  // Load a scroll x offset.
                             0x3e, 0x00, 0xe0, 0x42,  // Load a scroll y offset.

                             // Load new gamepad state
                              0x16, 0x30, // Load 0x30 into D
                              0x0e, 0x00,  // Load 0x00 into C
                              0x3e, 0x20, // Turn on only one column
                              0xe2, // Load A into 0xff00
                              0xf2, // read register 0xff + C into A
                              0x5f, // Put A into E
                              0x1a, 0x00, // Load dummy from (DE) into

                              0x14, // Increment D
                              0x3e, 0x10, // Turn on the other column
                              0xe2, // Load A into 0xff00
                              0xf2, // read register 0xff + C into A
                              0x5f, // Put A into E
                              0x1a, 0x00, // Load dummy from (DE)

                              0x3e, 0x00, // Turn on both columns so any press is detected
                              0xe2, // Load A into 0xff00
                              0x21, 0x00, 0x11, // Load 0x1100 into hl
                              0x3e, 0xee, // Clear the interrupt before it's enabled
                              0x0e, 0x0f,  // Load 0x0f into C
                              0xe2, // Load A into 0xff0f

                             0x21, 0x00, 0x11, // Load 0x1000 into hl
                             0xd9, // Return from the interrupt
                             0xe9}; // jump to 0x1100

uint16_t x_position = 0;
uint16_t y_position = 0;
static void vsync_interrupt(void) {
    vsync_count++;
    if ((gamepad_state & 0x02) == 0) {
        x_position++;
    } else if ((gamepad_state & 0x01) == 0) {
        x_position--;
    }
    if ((gamepad_state & 0x04) == 0) {
        y_position++;
    } else if ((gamepad_state & 0x08) == 0) {
        y_position--;
    }
    gamepad_state = 0xff;
    while (dma_transfer_status(dma_out_channel) == 0) {}
    if (dma_transfer_status(dma_out_channel) != 0) {
        // DMA free, return immediately.
        DmacDescriptor* descriptor_out = dma_descriptor(dma_out_channel);
        descriptor_out->BTCTRL.reg |= DMAC_BTCTRL_VALID;
        size_t len = sizeof(vsync_interrupt_response);
        vsync_interrupt_response[4] = x_position;
        vsync_interrupt_response[8] = y_position;
        descriptor_out->BTCNT.reg = sizeof(vsync_interrupt_response);
        descriptor_out->SRCADDR.reg = ((uint32_t) vsync_interrupt_response) + len;

        dma_enable_channel(dma_out_channel);
    } else {
        asm("bkpt"); // need to queue interrupt return
    }
}


uint8_t gamepad_interrupt_response[] = {0x00, 0x00,
                             0x16, 0x30, // Load 0x30 into D
                             0x0e, 0x00,  // Load 0x00 into C

                             0x3e, 0x20, // Turn on only one column
                             0xe2, // Load A into 0xff00
                             0xf2, // read register 0xff + C into A
                             0x5f, // Put A into E
                             0x1a, 0x00, // Load dummy from (DE) into

                             0x14, // Increment D
                             0x3e, 0x10, // Turn on the other column
                             0xe2, // Load A into 0xff00
                             0xf2, // read register 0xff + C into A
                             0x5f, // Put A into E
                             0x1a, 0x00, // Load dummy from (DE)

                             0x3e, 0x00, // Turn on both columns so any press is detected
                             0xe2, // Load A into 0xff00
                             0x21, 0x00, 0x11, // Load 0x1100 into hl
                             0x3e, 0xef, // Clear the interrupt before it's enabled
                             0x0e, 0x0f,  // Load 0x0f into C
                             0xe2, // Load A into 0xff0f

                             0xd9, // Return from the interrupt
                             0xe9}; //  and jump to hl.

static void gamepad_interrupt(void) {
    PORT->Group[1].OUTTGL.reg = 1 << 17;
    gamepad_count++;
    while (dma_transfer_status(dma_out_channel) == 0) {}
    if (dma_transfer_status(dma_out_channel) != 0) {
        // DMA free, return immediately.
        DmacDescriptor* descriptor_out = dma_descriptor(dma_out_channel);
        descriptor_out->BTCTRL.reg |= DMAC_BTCTRL_VALID;
        size_t len = sizeof(gamepad_interrupt_response);
        descriptor_out->BTCNT.reg = sizeof(gamepad_interrupt_response);
        descriptor_out->SRCADDR.reg = ((uint32_t) gamepad_interrupt_response) + len;

        dma_enable_channel(dma_out_channel);
    } else {
        asm("bkpt"); // need to queue interrupt return
    }
    PORT->Group[1].OUTTGL.reg = 1 << 17;
}

void EVSYS_0_Handler() {
    uint16_t address = *((volatile uint16_t*) &PORT->Group[1].IN.reg);
    EVSYS->Channel[0].CHINTFLAG.reg = 3;
        addresses[address_i] = address;
        address_i = (address_i + 1) % 2048;
    if (!everything_going) return;
    if (address == 0x0040) {
        //PORT->Group[1].OUTTGL.reg = 1 << 17;
        if (vsync_count == 1) {
            TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_READSYNC;
            while (TCC0->SYNCBUSY.reg != 0) {}
            uint16_t count = TCC0->COUNT.reg;
            TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_STOP;
            while (TCC0->SYNCBUSY.bit.CTRLB == 1) {}
            TCC0->PER.reg = count + 2;
            while (TCC0->SYNCBUSY.reg != 0) {}

            if (count == 0) {
                asm("bkpt");
            }
        }
        TCC0->COUNT.reg = 0;
        TCC0->INTFLAG.reg = TCC_INTFLAG_OVF;
        TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;
        while (TCC0->SYNCBUSY.reg != 0) {}
        TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;
        while (TCC0->STATUS.bit.STOP == 1) {}
        vsync_interrupt();
        //PORT->Group[1].OUTTGL.reg = 1 << 17;
        if ((TCC0->STATUS.bit.STOP == 1)) {
            asm("bkpt");
        }
    } else if (address == 0x0060) {
        PORT->Group[1].OUTTGL.reg = 1 << 17;
        gamepad_interrupt();
        PORT->Group[1].OUTTGL.reg = 1 << 17;
    } else if (address < 0x0100 && address != 0) {
        PORT->Group[1].OUTTGL.reg = 1 << 17;
        //asm("bkpt");
    } else if ((address & 0xf000) == 0x3000) {
        uint8_t nibble = (address & 0xf);
        uint8_t position = (address & 0x0100) >> 8;
        uint8_t data = 0xf0 | nibble;
        if (position == 1) {
            data = 0x0f | (nibble << 4);
        }
        gamepad_state &= data;
    }


    // TODO: reenable interrupts after one occurs.
}

void tcc_wait_for_sync(Tcc* tcc) {
    while (tcc->SYNCBUSY.reg != 0) {}
}

// Everything but MC interrupts
void TCC0_0_Handler(void) {
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
    if (TCC0->INTFLAG.bit.OVF == 1) {
        TCC0->INTFLAG.reg = TCC_INTFLAG_OVF;
        vsync_interrupt();
    }
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
}

// MC0
void TCC0_1_Handler(void) {
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
    TCC0->INTFLAG.reg = TCC_INTFLAG_MC0;
    // for (uint8_t i = 0; i < 20; i++) {
    //     asm("nop");
    // }
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
}

// MC1
void TCC0_2_Handler(void) {
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
    TCC0->INTFLAG.reg = TCC_INTFLAG_MC1;
    // for (uint8_t i = 0; i < 20; i++) {
    //     asm("nop");
    // }
    //PORT->Group[1].OUTTGL.reg = 1 << 17;
}

void gbio_init(void) {
    address_i = 0;
    everything_going = false;

    vsync_count = 0;
    gamepad_state = 0xff;

    // Prep a vsync backup. Once in a while we miss an address so we keep a TC in sync with it.
    turn_on_clocks(false, 0, 1);

    // Make sure nothing resets our backup timer.
    never_reset_tcc(0);

    Tcc* tcc = TCC0;
    // Disable the module before resetting it.
    if (tcc->CTRLA.bit.ENABLE == 1) {
        tcc->CTRLA.bit.ENABLE = 0;
        while (tcc->SYNCBUSY.bit.ENABLE == 1) {}
    }
    tcc->CTRLA.bit.SWRST = 1;
    while (tcc->SYNCBUSY.bit.SWRST == 1) {}
    tcc_set_enable(tcc, false);
    tcc->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1024;
    tcc_wait_for_sync(tcc);
    tcc->CTRLBSET.reg = TCC_CTRLBSET_ONESHOT | TCC_CTRLBSET_LUPD;
    tcc_wait_for_sync(tcc);
    tcc->WAVE.reg = TCC_WAVE_WAVEGEN_NFRQ;
    tcc_wait_for_sync(tcc);
    tcc->PER.reg = 0xffffff;
    tcc->CC[0].reg = 0x01ff;
    tcc->CC[1].reg = 0x00ff;
    tcc_wait_for_sync(tcc);
    for (uint8_t i = 0; i < 7; i++) {
        NVIC_DisableIRQ(TCC0_0_IRQn + i);
        NVIC_ClearPendingIRQ(TCC0_0_IRQn + i);
        NVIC_EnableIRQ(TCC0_0_IRQn + i);
    }
    tcc->INTENSET.reg = TCC_INTENSET_OVF | TCC_INTENSET_MC0 | TCC_INTENSET_MC1;
    tcc_wait_for_sync(tcc);
    tcc_set_enable(tcc, true);
    tcc->CTRLBSET.reg = TCC_CTRLBSET_CMD_STOP;

    // Set up the LUT
    MCLK->APBCMASK.bit.CCL_ = true;
    hri_gclk_write_PCHCTRL_reg(GCLK, CCL_GCLK_ID,
                               GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos));

    CCL->CTRL.bit.SWRST = 1;
    while (CCL->CTRL.bit.SWRST == 1) {}

    // Truth is 0xe which is 1 except when all three inputs, A15, RD and CLK are low.
    CCL->LUTCTRL[0].reg = CCL_LUTCTRL_TRUTH(0xfe) |
                          CCL_LUTCTRL_LUTEO |
                          CCL_LUTCTRL_INSEL0_IO |    // CLK
                          CCL_LUTCTRL_INSEL1_IO | // A15 event event is 0x3
                          CCL_LUTCTRL_INSEL2_IO |    // Read
                          CCL_LUTCTRL_ENABLE;

    // LUTCTRL is ENABLE protected even though the data sheet says its not.
    CCL->CTRL.bit.ENABLE = true;

    gpio_set_pin_function(CLK0_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(A15_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(RD0_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(DATA_OE_PIN, GPIO_PIN_FUNCTION_N);

    // Set the data direction for output only. We'll worry about reading later.
    gpio_set_pin_function(DATA_DIR_PIN, GPIO_PIN_FUNCTION_OFF);
    gpio_set_pin_direction(DATA_DIR_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(DATA_DIR_PIN, false);

    gpio_set_pin_function(PIN_PB17, GPIO_PIN_FUNCTION_OFF);
    gpio_set_pin_direction(PIN_PB17, GPIO_DIRECTION_OUT);

    gpio_set_pin_level(RESET_PIN, true);
    gpio_set_pin_direction(RESET_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_function(RESET_PIN, GPIO_PIN_FUNCTION_OFF);
    PORT->Group[0].PINCFG[22].bit.DRVSTR = true;

    gpio_set_port_direction(GPIO_PORTB, 0x0000ffff, GPIO_DIRECTION_IN);

    // Set up output DMA
    gpio_set_port_direction(GPIO_PORTA, 0x00ff0000, GPIO_DIRECTION_OUT);

    dma_out_channel = find_free_audio_dma_channel();
    DmacDescriptor* descriptor_out = dma_descriptor(dma_out_channel);
    descriptor_out->BTCTRL.reg = DMAC_BTCTRL_VALID |
                             DMAC_BTCTRL_BLOCKACT_NOACT |
                             DMAC_BTCTRL_SRCINC |
                             DMAC_BTCTRL_BEATSIZE_BYTE;
                                    // Custom logo 48 bytes.

    descriptor_out->BTCNT.reg = sizeof(gameboy_boot);
    descriptor_out->DSTADDR.reg = (uint32_t)&PORT->Group[0].OUT.reg + 2;
    descriptor_out->DESCADDR.reg = 0;
    descriptor_out->SRCADDR.reg = ((uint32_t) gameboy_boot) + sizeof(gameboy_boot);
    dma_configure(dma_out_channel, 0, false);
    DmacChannel* out_channel = &DMAC->Channel[dma_out_channel];
    out_channel->CHEVCTRL.reg = DMAC_CHEVCTRL_EVIE | DMAC_CHEVCTRL_EVACT_TRIG;

    reset_event_system();
    turn_on_event_system();
    uint8_t event_out_channel = 0; // find_sync_event_channel();
    connect_gclk_to_peripheral(0, EVSYS_GCLK_ID_0 + event_out_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_DMAC_CH_0 + dma_out_channel, event_out_channel);
    EVSYS->Channel[event_out_channel].CHANNEL.reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_0) |
                                                EVSYS_CHANNEL_PATH_RESYNCHRONIZED |
                                                EVSYS_CHANNEL_EDGSEL_FALLING_EDGE;
    EVSYS->Channel[event_out_channel].CHINTFLAG.reg = 0;
    EVSYS->Channel[event_out_channel].CHINTENSET.bit.EVD = 1;

    NVIC_SetPriority(EVSYS_0_IRQn, 0);
    NVIC_EnableIRQ(EVSYS_0_IRQn);

    dma_enable_channel(dma_out_channel);
    //DMAC->SWTRIGCTRL.bit.SWTRIG0 = 1 << dma_out_channel;

    // Start the CPU
    gpio_set_pin_level(RESET_PIN, false);

    PORT->Group[1].OUTTGL.reg = 1 << 17;
    while (dma_transfer_status(dma_out_channel) == 0) {
        MICROPY_VM_HOOK_LOOP
    }
    PORT->Group[1].OUTTGL.reg = 1 << 17;
    everything_going = true;
}

void common_hal_gbio_queue_commands(const uint8_t* buf, uint32_t len) {
    if (len > 512 - 4) {
        mp_raise_ValueError(translate("Too many commands"));
    }
    // Wait for a previous sequence to finish.
    while (dma_transfer_status(dma_out_channel) == 0) {
        MICROPY_VM_HOOK_LOOP
    }
    memcpy(command_cache, buf, len);

    command_cache[len] = 0x21; // Load into hl
    command_cache[len + 1] = 0x00; // Load into hl
    command_cache[len + 2] = 0x10; // Load into hl
    command_cache[len + 3] = 0xe9; // jump to where hl points.
    len += 4;

    DmacDescriptor* descriptor_out = dma_descriptor(dma_out_channel);
    descriptor_out->BTCTRL.reg |= DMAC_BTCTRL_VALID;
    descriptor_out->BTCNT.reg = len;
    descriptor_out->SRCADDR.reg = ((uint32_t) command_cache) + len;

    dma_enable_channel(dma_out_channel);


    while (dma_transfer_status(dma_out_channel) == 0) {
        MICROPY_VM_HOOK_LOOP
    }
}
