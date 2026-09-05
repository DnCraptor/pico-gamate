/**
 * MIT License
 *
 * Copyright (c) 2022 Vincent Mistler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#define PWM_PIN0 (AUDIO_PWM_PIN&0xfe)
#define PWM_PIN1 (PWM_PIN0+1)

#include "audio.h"

#ifndef AUDIO_PWM_PIN
#include "hardware/irq.h"
#endif

#ifdef AUDIO_PWM_PIN
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#endif

#ifndef AUDIO_PWM_PIN
/* Keep the I2S clocks running if the producer misses a DMA boundary. */
static i2s_config_t *i2s_dma_irq_config = NULL;
static volatile bool i2s_dma_started = false;
static volatile bool i2s_dma_holding = false;
static volatile bool i2s_dma_buffer_ready = false;
static uint32_t i2s_hold_sample = 0;
static uint32_t i2s_next_hold_sample = 0;

static void __not_in_flash_func(i2s_dma_irq1_handler)(void) {
    i2s_config_t *cfg = i2s_dma_irq_config;
    if (!cfg) return;

    const uint32_t mask = 1u << cfg->dma_channel;
    if (!(dma_hw->ints1 & mask)) return;
    dma_hw->ints1 = mask;

    if (i2s_dma_buffer_ready) {
        i2s_dma_buffer_ready = false;
        i2s_dma_holding = false;
        i2s_hold_sample = i2s_next_hold_sample;
        dma_channel_set_read_addr(cfg->dma_channel, cfg->dma_buf, false);
        dma_channel_set_trans_count(cfg->dma_channel, cfg->dma_trans_count, true);
    } else {
        i2s_dma_holding = true;
        dma_channel_set_read_addr(cfg->dma_channel, &i2s_hold_sample, false);
        dma_channel_set_trans_count(cfg->dma_channel, 1, true);
    }
}
#endif

/**
 * return the default i2s context used to store information about the setup
 */
i2s_config_t i2s_get_default_config(void) {
    i2s_config_t i2s_config = {
		.sample_freq = 44100, 
		.channel_count = 2,
		.data_pin = AUDIO_DATA_PIN,
		.clock_pin_base = AUDIO_CLOCK_PIN,
		.pio = pio1,
		.sm = 0,
        .dma_channel = 0,
        .dma_buf = NULL,
        .dma_trans_count = 0,
        .volume = 0,
	};

    return i2s_config;
}

/**
 * Initialize the I2S driver. Must be called before calling i2s_write or i2s_dma_write
 * i2s_config: I2S context obtained by i2s_get_default_config()
 */


void i2s_init(i2s_config_t *i2s_config) {


#ifndef AUDIO_PWM_PIN

    uint8_t func=GPIO_FUNC_PIO1;    // TODO: GPIO_FUNC_PIO0 for pio0 or GPIO_FUNC_PIO1 for pio1
    gpio_set_function(i2s_config->data_pin, func);
    gpio_set_function(i2s_config->clock_pin_base, func);
    gpio_set_function(i2s_config->clock_pin_base+1, func);
    
    i2s_config->sm = pio_claim_unused_sm(i2s_config->pio, true);

    /* Set PIO clock */
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / i2s_config->sample_freq; // avoid arithmetic overflow

#ifdef I2S_CS4334
    uint offset = pio_add_program(i2s_config->pio, &audio_i2s_cs4334_program);
    audio_i2s_cs4334_program_init(i2s_config->pio, i2s_config->sm , offset, i2s_config->data_pin , i2s_config->clock_pin_base);
    divider >>= 3;
#else
    uint offset = pio_add_program(i2s_config->pio, &audio_i2s_program);
    audio_i2s_program_init(i2s_config->pio, i2s_config->sm , offset, i2s_config->data_pin , i2s_config->clock_pin_base);

#endif

    pio_sm_set_clkdiv_int_frac(i2s_config->pio, i2s_config->sm , divider >> 8u, divider & 0xffu);

    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm, false);
#endif
    /* Allocate memory for the DMA buffer */
    i2s_config->dma_buf=malloc(i2s_config->dma_trans_count*sizeof(uint32_t));

    /* Direct Memory Access setup */
    i2s_config->dma_channel = dma_claim_unused_channel(true);
    
    dma_channel_config dma_config = dma_channel_get_default_config(i2s_config->dma_channel);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);

    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);

    volatile uint32_t* addr_write_DMA=&(i2s_config->pio->txf[i2s_config->sm]);
#ifdef AUDIO_PWM_PIN
    gpio_set_function(PWM_PIN0, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN0);


   
    pwm_config c_pwm=pwm_get_default_config();
    pwm_config_set_clkdiv(&c_pwm,1.0);
    //pwm_config_set_wrap(&c_pwm,(1<<12)-1);//MAX PWM value
    pwm_config_set_wrap(&c_pwm,clock_get_hz(clk_sys)/(i2s_config->sample_freq));//MAX PWM value
    pwm_init(slice_num,&c_pwm,true);

    //Для синхронизации используем другой произвольный канал ШИМ


    channel_config_set_dreq(&dma_config, pwm_get_dreq(slice_num));     


    addr_write_DMA=(uint32_t*)&pwm_hw->slice[slice_num].cc;
#else
    channel_config_set_dreq(&dma_config, pio_get_dreq(i2s_config->pio, i2s_config->sm, true));
#endif
    
    dma_channel_configure(i2s_config->dma_channel,
                          &dma_config,
                          addr_write_DMA,    // Destination pointer
                          i2s_config->dma_buf,                        // Source pointer
                          i2s_config->dma_trans_count,                // Number of 32 bits words to transfer
                          false                                       // Start immediately
    );

#ifndef AUDIO_PWM_PIN
    i2s_dma_irq_config = i2s_config;
    i2s_dma_started = false;
    i2s_dma_holding = false;
    i2s_dma_buffer_ready = false;
    i2s_hold_sample = 0;
    i2s_next_hold_sample = 0;
    dma_hw->ints1 = 1u << i2s_config->dma_channel;
    irq_set_exclusive_handler(DMA_IRQ_1, i2s_dma_irq1_handler);
    irq_set_priority(DMA_IRQ_1, 0);
    dma_channel_set_irq1_enabled(i2s_config->dma_channel, true);
    irq_set_enabled(DMA_IRQ_1, true);
#endif

    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm , true);
}

/**
 * Write samples to I2S directly and wait for completion (blocking)
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     sample: pointer to an array of len x 32 bits samples
 *             Each 32 bits sample contains 2x16 bits samples, 
 *             one for the left channel and one for the right channel
 *        len: length of sample in 32 bits words
 */
void i2s_write(const i2s_config_t *i2s_config,const int16_t *samples,const size_t len) {
    for(size_t i=0;i<len;i++) {
            pio_sm_put_blocking(i2s_config->pio, i2s_config->sm, (uint32_t)samples[i]);
    }
}

/**
 * Write samples to DMA buffer and initiate DMA transfer (non blocking)
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     sample: pointer to an array of dma_trans_count x 32 bits samples
 */
void __not_in_flash_func(i2s_dma_write)(i2s_config_t *i2s_config,const int16_t *samples) {
#ifdef AUDIO_PWM_PIN
    /* PWM naturally holds the last output value while DMA is idle. */
    dma_channel_wait_for_finish_blocking(i2s_config->dma_channel);
#else
    /* Do not overwrite dma_buf while its normal block is still playing.
     * The completion IRQ first switches DMA to the one-sample HOLD source. */
    if (i2s_dma_started) {
        while (!i2s_dma_holding) { }
    }
#endif

    /* Copy samples into the DMA buffer */
#ifdef AUDIO_PWM_PIN
    for(uint16_t i=0;i<i2s_config->dma_trans_count*2;i++) {
           
            i2s_config->dma_buf[i] = (65536/2+(samples[i]))>>(4+i2s_config->volume);

        }
#else

    if(i2s_config->volume==0) {
        /* Keep the core1 audio hot path independent of libc/XIP.
         * One DMA word is one interleaved stereo pair. */
        uint32_t *dst = (uint32_t *)i2s_config->dma_buf;
        for (uint16_t i = 0; i < i2s_config->dma_trans_count; ++i) {
            const uint32_t left = (uint16_t)samples[i * 2];
            const uint32_t right = (uint16_t)samples[i * 2 + 1];
            dst[i] = left | (right << 16);
        }
    } else {
        for(uint16_t i=0;i<i2s_config->dma_trans_count*2;i++) {
            i2s_config->dma_buf[i] = samples[i]>>i2s_config->volume;
        }
    }

    /* Keep the current HOLD value unchanged until this block actually starts. */
    i2s_next_hold_sample = ((uint32_t *)i2s_config->dma_buf)[i2s_config->dma_trans_count - 1];
    __asm volatile ("dmb" ::: "memory");

    if (i2s_dma_started) {
        i2s_dma_buffer_ready = true;
        return;
    }
    i2s_hold_sample = i2s_next_hold_sample;
    i2s_dma_started = true;
#endif    


    /* First I2S block starts directly. Later blocks are picked up by IRQ 1
     * on the next one-sample HOLD boundary. PWM keeps the original path. */
    dma_channel_transfer_from_buffer_now(i2s_config->dma_channel,
                                         i2s_config->dma_buf,
                                         i2s_config->dma_trans_count);
}

/**
 * Adjust the output volume
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     volume: desired volume between 0 (highest. volume) and 16 (lowest volume)
 */
void i2s_volume(i2s_config_t *i2s_config,uint8_t volume) {
    if(volume>16) volume=16;
    i2s_config->volume=volume;
}

/**
 * Increases the output volume
 */
void i2s_increase_volume(i2s_config_t *i2s_config) {
    if(i2s_config->volume>0) {
        i2s_config->volume--;
    }
}

/**
 * Decreases the output volume
 */
void i2s_decrease_volume(i2s_config_t *i2s_config) {
    if(i2s_config->volume<16) {
        i2s_config->volume++;
    }
}
