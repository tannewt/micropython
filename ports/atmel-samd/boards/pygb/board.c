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

#include "boards/board.h"
#include "mpconfigboard.h"
#include "hal/include/hal_gpio.h"

#include "atmel_start_pins.h"

#include "peripherals/samd/events.h"

void board_init(void) {
    // gpio_set_pin_function(RESET_PIN, GPIO_PIN_FUNCTION_OFF);
    // gpio_set_pin_direction(RESET_PIN, GPIO_DIRECTION_OUT);
    // gpio_set_pin_level(RESET_PIN, true);

    // gpio_set_pin_function(CLOCK_PIN, GPIO_PIN_FUNCTION_A);
    // gpio_set_pin_function(WRITE_PIN, GPIO_PIN_FUNCTION_A);

    // Pipe A15 into the EIC. We do async level detection on it to move it into event
    // channel 31.
    gpio_set_pin_function(HIGH_ADDRESS_PIN, GPIO_PIN_FUNCTION_A);

    // Turn on the EIC
    MCLK->APBAMASK.bit.EIC_ = true;
    hri_gclk_write_PCHCTRL_reg(GCLK, EIC_GCLK_ID,
                               GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos));

    uint8_t sense_setting = EIC_CONFIG_SENSE0_HIGH;

    EIC->CTRLA.bit.ENABLE = false;
    while (EIC->SYNCBUSY.bit.ENABLE != 0) {}

    uint8_t eic_channel = HIGH_ADDRESS_PIN % 16;
    uint8_t config_index = eic_channel / 8;
    uint8_t position = (eic_channel % 8) * 4;
    uint32_t masked_value = EIC->CONFIG[config_index].reg & ~(0xf << position);
    EIC->CONFIG[config_index].reg = masked_value | (sense_setting << position);

    EIC->EVCTRL.reg |= 1 << eic_channel;

    EIC->CTRLA.bit.ENABLE = true;
    while (EIC->SYNCBUSY.bit.ENABLE != 0) {}

    // Not ok

    // The address bit is then piped into CCL[0] to generate an event output that is only
    reset_event_system();
    turn_on_event_system();
    uint8_t event_channel = find_async_event_channel();
    //connect_event_user_to_channel(EVSYS_ID_USER_PORT_EV_0, event_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_CCL_LUTIN_0, event_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_CCL_LUTIN_1, event_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_CCL_LUTIN_2, event_channel);
    connect_event_user_to_channel(EVSYS_ID_USER_CCL_LUTIN_3, event_channel);
    init_async_event_channel(event_channel, EVSYS_ID_GEN_EIC_EXTINT_0 + eic_channel);

    // Set up the LUT
    MCLK->APBCMASK.bit.CCL_ = true;
    hri_gclk_write_PCHCTRL_reg(GCLK, CCL_GCLK_ID,
                               GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos));

    CCL->CTRL.bit.SWRST = 1;
    while (CCL->CTRL.bit.SWRST == 1) {}

    // Truth is 0xe which is 1 except when all three inputs, A15, RD and CLK are low.
    //                      CCL_LUTCTRL_LUTEO |
    CCL->LUTCTRL[0].reg = CCL_LUTCTRL_TRUTH(0xfe) |
                          CCL_LUTCTRL_LUTEO |
                          CCL_LUTCTRL_INSEL0_IO |    // CLK
                          CCL_LUTCTRL_INSEL1_IO | // A15 event event is 0x3
                          CCL_LUTCTRL_INSEL2_IO |    // Read
                          CCL_LUTCTRL_ENABLE;

    // LUTCTRL is ENABLE protected even though the data sheet says its not.
    CCL->CTRL.bit.ENABLE = true;
    //not ok

    gpio_set_pin_function(CLK0_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(WR0_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(RD0_PIN, GPIO_PIN_FUNCTION_N);
    gpio_set_pin_function(DATA_OE_PIN, GPIO_PIN_FUNCTION_N);

    // gpio_set_pin_function(DATA_OE_PIN, GPIO_PIN_FUNCTION_OFF);
    // gpio_set_pin_direction(DATA_OE_PIN, GPIO_DIRECTION_OUT);
    // gpio_set_pin_level(DATA_OE_PIN, true);

    // Set the data direction for output only. We'll worry about reading later.
    gpio_set_pin_function(DATA_DIR_PIN, GPIO_PIN_FUNCTION_OFF);
    gpio_set_pin_direction(DATA_DIR_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(DATA_DIR_PIN, false);

    // Output the address event line
    //PORT->Group[0].EVCTRL.reg = PORT_EVCTRL_PID0(11) | PORT_EVCTRL_PORTEI0 | PORT_EVCTRL_EVACT0_OUT;
}

bool board_requests_safe_mode(void) {
    return false;
}

void reset_board(void) {
    gpio_set_pin_level(DATA_DIR_PIN, false);
}
