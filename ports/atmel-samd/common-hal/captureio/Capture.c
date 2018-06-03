/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
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

#include "common-hal/captureio/Capture.h"

#include "hal/include/hal_gpio.h"

#include "atmel_start_pins.h"

#include "audio_dma.h"
#include "clocks.h"
#include "events.h"

#define RESET_PIN PIN_PB22
#define CLOCK_PIN PIN_PA04
#define WRITE_PIN PIN_PA05
#define HIGH_ADDRESS_PIN PIN_PB15

void common_hal_captureio_capture_construct(captureio_capture_obj_t* self) {
    // gpio_set_pin_function(CLOCK_PIN, GPIO_PIN_FUNCTION_A);
    // gpio_set_pin_function(WRITE_PIN, GPIO_PIN_FUNCTION_A);
    // gpio_set_pin_function(HIGH_ADDRESS_PIN, GPIO_PIN_FUNCTION_A);
    //
    // // Set the address bus as input
    gpio_set_port_direction(GPIO_PORTB, 0x0000ffff, GPIO_DIRECTION_IN);

}

bool common_hal_captureio_capture_deinited(captureio_capture_obj_t *self) {
    return false;
}

void common_hal_captureio_capture_deinit(captureio_capture_obj_t *self) {
    if (common_hal_captureio_capture_deinited(self)) {
        return;
    }
}


void common_hal_captureio_capture_capture(captureio_capture_obj_t* self, uint32_t* buffer,
    uint32_t buffer_length) {


    gpio_set_pin_level(RESET_PIN, true);
    gpio_set_pin_direction(RESET_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_function(RESET_PIN, GPIO_PIN_FUNCTION_OFF);
    PORT->Group[0].PINCFG[22].bit.DRVSTR = true;

    // for (uint32_t i = 0; i < buffer_length / 2; i++) {
    //     ((uint16_t*) buffer)[i] = PORT->Group[1].IN.reg;
    //     for (uint32_t j = 0; j < 2000; j++) {
    //         asm("nop");
    //     }
    // // }
    //
    uint8_t dma_channel = find_free_audio_dma_channel();

    DmacDescriptor* descriptor = dma_descriptor(dma_channel);
    descriptor->BTCTRL.reg = DMAC_BTCTRL_VALID |
                             DMAC_BTCTRL_BLOCKACT_NOACT |
                             DMAC_BTCTRL_DSTINC |
                             DMAC_BTCTRL_BEATSIZE_HWORD;

    descriptor->BTCNT.reg = buffer_length / 2;
    descriptor->DSTADDR.reg = ((uint32_t) buffer + buffer_length);
    descriptor->DESCADDR.reg = 0;
    descriptor->SRCADDR.reg = (uint32_t)&PORT->Group[1].IN.reg;

    dma_configure(dma_channel, 0, false);
    dma_enable_channel(dma_channel);

    // Set up output DMA
    gpio_set_port_direction(GPIO_PORTA, 0x00ff0000, GPIO_DIRECTION_OUT);

    uint8_t dma_out_channel = find_free_audio_dma_channel();
    DmacDescriptor* descriptor_out = dma_descriptor(dma_out_channel);
    descriptor_out->BTCTRL.reg = DMAC_BTCTRL_VALID |
                             DMAC_BTCTRL_BLOCKACT_NOACT |
                             DMAC_BTCTRL_SRCINC |
                             DMAC_BTCTRL_BEATSIZE_BYTE;
                                    // Custom logo 48 bytes
    const uint8_t initial_data[] = {0x99, 0x99, 0x00, 0x74, 0x55, 0x55, 0x00, 0x36, 0x00, 0xC6, 0x44, 0x44, 0x11, 0x99, 0x00, 0x36,
                                    0x00, 0xC6, 0x00, 0x69, 0x44, 0x44, 0x22, 0x22, 0xF9, 0x99, 0x74, 0x47, 0x55, 0x55, 0x66, 0x63,
                                    0x66, 0x6C, 0x55, 0x52, 0x55, 0x52, 0x66, 0x63, 0x66, 0x6C, 0x88, 0x88, 0x45, 0x54, 0xE2, 0x2E,

                                    // Nintendo logo 48 bytes
                                    0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
                                    0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
                                    0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,

                                    // Cartridge header 28 bytes
                                    0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xCA, 0x31, 0x58,

                                    // NOP
                                    0x00};
    descriptor_out->BTCNT.reg = 125;
    descriptor_out->DSTADDR.reg = (uint32_t)&PORT->Group[0].OUT.reg + 2;
    descriptor_out->DESCADDR.reg = 0;
    descriptor_out->SRCADDR.reg = ((uint32_t) initial_data) + 125;
    dma_configure(dma_out_channel, 0, false);

    uint8_t event_channel = find_sync_event_channel();
    connect_gclk_to_peripheral(0, EVSYS_GCLK_ID_0 + event_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_DMAC_CH_0 + dma_channel, event_channel);
    EVSYS->Channel[event_channel].CHANNEL.reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_0) |
                                                EVSYS_CHANNEL_PATH_RESYNCHRONIZED |
                                                EVSYS_CHANNEL_EDGSEL_FALLING_EDGE;
    // EVSYS->Channel[event_channel].CHANNEL.reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_0) |
    //                                             EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
    EVSYS->Channel[event_channel].CHINTFLAG.reg = 0;
    EVSYS->Channel[event_channel].CHINTENSET.bit.EVD = 1;
    //connect_event_user_to_channel(EVSYS_ID_USER_PORT_EV_0, event_channel);

    uint8_t event_out_channel = find_sync_event_channel();
    connect_gclk_to_peripheral(0, EVSYS_GCLK_ID_0 + event_out_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_DMAC_CH_0 + dma_out_channel, event_out_channel);
    EVSYS->Channel[event_out_channel].CHANNEL.reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_0) |
                                                EVSYS_CHANNEL_PATH_RESYNCHRONIZED |
                                                EVSYS_CHANNEL_EDGSEL_RISING_EDGE;
    // EVSYS->Channel[event_channel].CHANNEL.reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_0) |
    //                                             EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
    EVSYS->Channel[event_out_channel].CHINTFLAG.reg = 0;
    EVSYS->Channel[event_out_channel].CHINTENSET.bit.EVD = 1;

    dma_enable_channel(dma_out_channel);
    DMAC->SWTRIGCTRL.bit.SWTRIG0 = 1 << dma_out_channel;

    // Start the CPU
    gpio_set_pin_level(RESET_PIN, false);

    // EVSYS->SWEVT.reg = 1 << event_channel;
    // DMAC->SWTRIGCTRL.bit.SWTRIG0 = 1;

    // uint16_t current_address;
    // do {
    //     current_address = *((volatile uint16_t*) &PORT->Group[1].IN.reg);
    // } while (current_address != 0xff26);


    while (dma_transfer_status(dma_channel) == 0) {}

    // Stop the CPU
    gpio_set_pin_level(RESET_PIN, true);
    dma_disable_channel(dma_channel);

    if (buffer[0] == 0) {
        asm("nop");
    }

    asm("bkpt");
}
