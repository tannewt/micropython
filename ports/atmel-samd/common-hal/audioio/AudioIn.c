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

#include <stdint.h>
#include <string.h>

#include "extmod/vfs_fat.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "common-hal/audioio/AudioIn.h"
#include "shared-bindings/audioio/AudioIn.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/translate.h"

#include "atmel_start_pins.h"
#include "hal/include/hal_gpio.h"
#include "hpl/gclk/hpl_gclk_base.h"
#include "peripheral_clk_config.h"

#ifdef SAMD21
#include "hpl/pm/hpl_pm_base.h"
#endif

#include "audio_dma.h"

#include "samd/dma.h"
#include "samd/events.h"
#include "samd/pins.h"
#include "samd/timers.h"

void audioin_reset(void) {
}

void common_hal_audioio_audioin_construct(audioio_audioin_obj_t* self,
        const mcu_pin_obj_t* left_channel, const mcu_pin_obj_t* right_channel) {

    uint8_t left_adc_index;
    uint8_t left_adc_channel = 0xff;
    for (adc_index = 0; adc_index < NUM_ADC_PER_PIN; adc_index++) {
        // TODO(tannewt): Only use ADC0 on the SAMD51 when touch isn't being
        // used.
        if (pin->adc_input[adc_index] != 0xff) {
            adc_channel = pin->adc_input[adc_index];
            break;
        }
    }
    if (adc_channel == 0xff) {
        // No ADC function on that pin
        mp_raise_ValueError(translate("Left pin does not have ADC capabilities"));
    }
    for (adc_index = 0; adc_index < NUM_ADC_PER_PIN; adc_index++) {
        // TODO(tannewt): Only use ADC0 on the SAMD51 when touch isn't being
        // used.
        if (pin->adc_input[adc_index] != 0xff) {
            adc_channel = pin->adc_input[adc_index];
            break;
        }
    }
    if (adc_channel == 0xff) {
        // No ADC function on that pin
        mp_raise_ValueError(translate("Left pin does not have ADC capabilities"));
    }
    claim_pin(pin);

    gpio_set_pin_function(pin->number, GPIO_PIN_FUNCTION_B);

    self->left_channel = left_channel;
    gpio_set_pin_function(self->left_channel->number, GPIO_PIN_FUNCTION_B);
    audio_dma_init(&self->left_dma);

    #ifdef SAMD51
    hri_mclk_set_APBDMASK_ADC_bit(MCLK);
    #endif

    #ifdef SAMD21
    _pm_enable_bus_clock(PM_BUS_APBC, ADC);
    #endif

    // SAMD51: This clock should be <= 12 MHz, per datasheet section 47.6.3.
    // SAMD21: This clock is 48mhz despite the datasheet saying it must only be <= 350kHz, per
    // datasheet table 37-6. It's incorrect because the max output rate is 350ksps and is only
    // achieved when the GCLK is more than 8mhz.
    _gclk_enable_channel(DAC_GCLK_ID, CONF_GCLK_DAC_SRC);

    DAC->CTRLA.bit.SWRST = 1;
    while (DAC->CTRLA.bit.SWRST == 1) {}

    bool channel0_enabled = true;
    #ifdef SAMD51
    channel0_enabled = self->left_channel == &pin_PA02 || self->right_channel == &pin_PA02;
    bool channel1_enabled = self->left_channel == &pin_PA05 || self->right_channel == &pin_PA05;
    #endif

    // Use a timer to coordinate when ADC conversions occur.
    Tc *t = NULL;
    uint8_t tc_index = TC_INST_NUM;
    for (uint8_t i = TC_INST_NUM; i > 0; i--) {
        if (tc_insts[i - 1]->COUNT16.CTRLA.bit.ENABLE == 0) {
            t = tc_insts[i - 1];
            tc_index = i - 1;
            break;
        }
    }
    if (t == NULL) {
        common_hal_audioio_audioin_deinit(self);
        mp_raise_RuntimeError(translate("All timers in use"));
        return;
    }
    self->tc_index = tc_index;

    // Use the 48mhz clocks on both the SAMD21 and 51 because we will be going much slower.
    uint8_t tc_gclk = 0;
    #ifdef SAMD51
    tc_gclk = 1;
    #endif

    turn_on_clocks(true, tc_index, tc_gclk);

    // Don't bother setting the period. We set it before you playback anything.
    tc_set_enable(t, false);
    tc_reset(t);
    #ifdef SAMD51
    t->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
    #endif
    #ifdef SAMD21
    t->COUNT16.CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_MFRQ_Val;
    #endif
    t->COUNT16.EVCTRL.reg = TC_EVCTRL_OVFEO;
    tc_set_enable(t, true);
    t->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;

    // Connect the timer overflow event, which happens at the target frequency,
    // to the DAC conversion trigger(s).
    #ifdef SAMD21
    #define FIRST_TC_GEN_ID EVSYS_ID_GEN_TC3_OVF
    #endif
    #ifdef SAMD51
    #define FIRST_TC_GEN_ID EVSYS_ID_GEN_TC0_OVF
    #endif
    uint8_t tc_gen_id = FIRST_TC_GEN_ID + 3 * tc_index;

    turn_on_event_system();

    // Find a free event channel. We start at the highest channels because we only need and async
    // path.
    uint8_t channel = find_async_event_channel();
    if (channel >= EVSYS_CHANNELS) {
        mp_raise_RuntimeError(translate("All event channels in use"));
    }

    #ifdef SAMD51
    connect_event_user_to_channel(EVSYS_ID_USER_ADC1_START, channel);
    #define EVSYS_ID_USER_DAC_START EVSYS_ID_USER_ADC0_START
    #endif
    connect_event_user_to_channel(EVSYS_ID_USER_ADC_START, channel);
    init_async_event_channel(channel, tc_gen_id);

    self->tc_to_dac_event_channel = channel;

    // Leave the DMA setup to playback.
}

bool common_hal_audioio_audioin_deinited(audioio_audioin_obj_t* self) {
    return self->left_channel == mp_const_none;
}

void common_hal_audioio_audioin_deinit(audioio_audioin_obj_t* self) {
    if (common_hal_audioio_audioin_deinited(self)) {
        return;
    }

    #ifdef SAMD21
    ADC->CTRLA.bit.ENABLE = 0;
    while (ADC->STATUS.bit.SYNCBUSY == 1) {}
    #endif
    #ifdef SAMD51
    if (self->left_adc_instance_number == 0 || self->right_adc_instance_number == 0) {
        ADC0->CTRLA.bit.ENABLE = 0;
        while (ADC0->SYNCBUSY.bit.ENABLE == 1) {}
    }

    if (self->left_adc_instance_number == 1 || self->right_adc_instance_number == 1) {
        ADC1->CTRLA.bit.ENABLE = 0;
        while (ADC1->SYNCBUSY.bit.ENABLE == 1) {}
    }
    #endif

    disable_event_channel(self->tc_to_dac_event_channel);

    tc_set_enable(tc_insts[self->tc_index], false);

    reset_pin_number(self->left_channel->number);
    self->left_channel = mp_const_none;
    #ifdef SAMD51
    reset_pin_number(self->right_channel->number);
    self->right_channel = mp_const_none;
    #endif
}

static void set_timer_frequency(Tc* timer, uint32_t frequency) {
    uint32_t system_clock = 48000000;
    uint32_t new_top;
    uint8_t new_divisor;
    for (new_divisor = 0; new_divisor < 8; new_divisor++) {
        new_top = (system_clock / prescaler[new_divisor] / frequency) - 1;
        if (new_top < (1u << 16)) {
            break;
        }
    }
    uint8_t old_divisor = timer->COUNT16.CTRLA.bit.PRESCALER;
    if (new_divisor != old_divisor) {
        tc_set_enable(timer, false);
        timer->COUNT16.CTRLA.bit.PRESCALER = new_divisor;
        tc_set_enable(timer, true);
    }
    tc_wait_for_sync(timer);
    timer->COUNT16.CC[0].reg = new_top;
    tc_wait_for_sync(timer);
}

void common_hal_audioio_audioin_record(audioio_audioin_obj_t* self, mp_obj_t sample) {
    if (common_hal_audioio_audioin_get_recording(self)) {
        common_hal_audioio_audioin_stop(self);
    }
    audio_dma_result result = AUDIO_DMA_OK;
    uint32_t sample_rate = audiosample_sample_rate(sample);
    #ifdef SAMD21
    uint32_t max_sample_rate = 350000;
    #endif
    #ifdef SAMD51
    uint32_t max_sample_rate = 1000000;
    #endif
    if (sample_rate > max_sample_rate) {
        mp_raise_ValueError_varg(translate("Sample rate too high. It must be less than %d"), max_sample_rate);
    }

    #ifdef SAMD21
    result = audio_dma_setup_recording(&self->left_dma, sample, true, 0,
                                      (uint32_t) &ADC->RESULT.reg,
                                      ADC_DMAC_ID_RESRDY);
    #endif

    #ifdef SAMD51
    uint32_t left_channel_reg = (uint32_t) &ADC0->RESULT.reg + 0x400 * self->left_adc_instance_number;
    uint8_t left_channel_trigger = ADC0_DMAC_ID_RESRDY + 2 * self->left_adc_instance_number;
    uint32_t right_channel_reg = 0;
    uint8_t right_channel_trigger = 0;
    if (self->right_pin != NULL) {
        right_channel_reg = (uint32_t) &ADC0->RESULT.reg + 0x400 * self->right_adc_instance_number;
        right_channel_trigger = ADC0_DMAC_ID_RESRDY + 2 * self->right_adc_instance_number;
    }
    result = audio_dma_setup_recording(&self->left_dma, sample, true, 0,
                                      left_channel_reg,
                                      left_channel_trigger);
    if (right_channel_reg != 0 && result == AUDIO_DMA_OK) {
        result = audio_dma_setup_recording(&self->right_dma, sample, true, 1,
                                          right_channel_reg,
                                          right_channel_trigger);
    }
    #endif
    if (result != AUDIO_DMA_OK) {
        audio_dma_stop(&self->left_dma);
        #ifdef SAMD51
        audio_dma_stop(&self->right_dma);
        #endif
        if (result == AUDIO_DMA_DMA_BUSY) {
            mp_raise_RuntimeError(translate("No DMA channel found"));
        } else if (result == AUDIO_DMA_MEMORY_ERROR) {
            mp_raise_RuntimeError(translate("Unable to allocate buffers for signed conversion"));
        }
    }
    Tc* timer = tc_insts[self->tc_index];
    set_timer_frequency(timer, audiosample_sample_rate(sample));
    timer->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (timer->COUNT16.STATUS.bit.STOP == 1) {}
    self->recording = true;
}

void common_hal_audioio_audioin_stop(audioio_audioin_obj_t* self) {
    Tc* timer = tc_insts[self->tc_index];
    timer->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
    audio_dma_stop(&self->left_dma);
    #ifdef SAMD51
    audio_dma_stop(&self->right_dma);
    #endif
}

bool common_hal_audioio_audioin_get_recording(audioio_audioin_obj_t* self) {
    bool now_recording = audio_dma_get_recording(&self->left_dma);
    if (self->recording && !now_recording) {
        common_hal_audioio_audioin_stop(self);
    }
    return now_recording;
}
