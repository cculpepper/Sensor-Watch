/*
 * MIT License
 *
 * Copyright (c) 2021 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "watch.h"
#include "peripheral_clk_config.h"
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////////////
// User callbacks and other definitions

ext_irq_cb_t btn_alarm_callback;
ext_irq_cb_t a2_callback;
ext_irq_cb_t d1_callback;

static void extwake_callback(uint8_t reason);


//////////////////////////////////////////////////////////////////////////////////////////
// Initialization

void _watch_init() {
    // disable the LED pin (it may have been enabled by the bootloader)
    watch_disable_digital_output(RED);

    // Use switching regulator for lower power consumption.
    SUPC->VREG.bit.SEL = 1;
    while(!SUPC->STATUS.bit.VREGRDY);

    // External wake depends on RTC; calendar is a required module.
    CALENDAR_0_init();
    calendar_enable(&CALENDAR_0);

    // Not sure if this belongs in every app -- is there a power impact?
    delay_driver_init();

    // set up state
    btn_alarm_callback = NULL;
    a2_callback = NULL;
    d1_callback = NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////
// Segmented Display

static const uint8_t Character_Set[] =
{
    0b00000000, //  
    0b00000000, // ! (unused)
    0b00100010, // "
    0b01100011, // # (degree symbol, hash mark doesn't fit)
    0b00000000, // $ (unused)
    0b00000000, // % (unused)
    0b01000100, // & ("lowercase 7" for positions 4 and 6)
    0b00100000, // '
    0b00111001, // (
    0b00001111, // )
    0b00000000, // * (unused)
    0b11000000, // + (only works in position 0)
    0b00000100, // ,
    0b01000000, // -
    0b01000000, // . (same as -, semantically most useful)
    0b00010010, // /
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000, // : (unused)
    0b00000000, // ; (unused)
    0b01011000, // <
    0b01001000, // =
    0b01001100, // >
    0b01010011, // ?
    0b11111111, // @ (all segments on)
    0b01110111, // A
    0b01111111, // B
    0b00111001, // C
    0b00111111, // D
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b10001001, // I (only works in position 0)
    0b00001110, // J
    0b01110101, // K
    0b00111000, // L
    0b10110111, // M (only works in position 0)
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b11110111, // R (only works in position 1)
    0b01101101, // S
    0b10000001, // T (only works in position 0; set (1, 12) to make it work in position 1)
    0b00111110, // U
    0b00111110, // V
    0b10111110, // W (only works in position 0)
    0b01111110, // X
    0b01101110, // Y
    0b00011011, // Z
    0b00111001, // [
    0b00100100, // backslash
    0b00001111, // ]
    0b00100011, // ^
    0b00001000, // _
    0b00000010, // `
    0b01011111, // a
    0b01111100, // b
    0b01011000, // c
    0b01011110, // d
    0b01111011, // e
    0b01110001, // f
    0b01101111, // g
    0b01110100, // h
    0b00010000, // i
    0b01000010, // j (appears as superscript to work in more positions)
    0b01110101, // k
    0b00110000, // l
    0b10110111, // m (only works in position 0)
    0b01010100, // n
    0b01011100, // o
    0b01110011, // p
    0b01100111, // q
    0b01010000, // r
    0b01101101, // s
    0b01111000, // t
    0b01100010, // u (appears as superscript to work in more positions)
    0b01100010, // v (appears as superscript to work in more positions)
    0b10111110, // w (only works in position 0)
    0b01111110, // x
    0b01101110, // y
    0b00011011, // z
    0b00111001, // {
    0b00110000, // |
    0b00001111, // }
    0b00000001, // ~
};

static const uint64_t Segment_Map[] = {
    0x4e4f0e8e8f8d4d0d, // Position 0, mode
    0xc8c4c4c8b4b4b0b,  // Position 1, mode (Segments B and C shared, as are segments E and F)
    0xc049c00a49890949, // Position 2, day of month (Segments A, D, G shared; missing segment F)
    0xc048088886874707, // Position 3, day of month
    0xc053921252139352, // Position 4, clock hours (Segments A and D shared)
    0xc054511415559594, // Position 5, clock hours
    0xc057965616179716, // Position 6, clock minutes (Segments A and D shared)
    0xc041804000018a81, // Position 7, clock minutes
    0xc043420203048382, // Position 8, clock seconds
    0xc045440506468584, // Position 9, clock seconds
};

static const uint8_t Num_Chars = 10;

static const uint32_t IndicatorSegments[6] = {
    SLCD_SEGID(0, 17), // WATCH_INDICATOR_SIGNAL
    SLCD_SEGID(0, 16), // WATCH_INDICATOR_BELL
    SLCD_SEGID(2, 17), // WATCH_INDICATOR_PM
    SLCD_SEGID(2, 16), // WATCH_INDICATOR_24H
    SLCD_SEGID(1, 10), // WATCH_INDICATOR_LAP
};

void watch_enable_display() {
    SEGMENT_LCD_0_init();
    slcd_sync_enable(&SEGMENT_LCD_0);
}

inline void watch_set_pixel(uint8_t com, uint8_t seg) {
    slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
}

inline void watch_clear_pixel(uint8_t com, uint8_t seg) {
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
}

void watch_display_character(uint8_t character, uint8_t position) {
    // handle lowercase 7 if needed
    if (character == '7' && (position == 4 || position == 6)) character = '&';

    uint64_t segmap = Segment_Map[position];
    uint64_t segdata = Character_Set[character - 0x20];

    for (int i = 0; i < 8; i++) {
        uint8_t com = (segmap & 0xFF) >> 6;
        if (com > 2) {
            // COM3 means no segment exists; skip it.
            segmap = segmap >> 8;
            segdata = segdata >> 1;
            continue;
        }
        uint8_t seg = segmap & 0x3F;
        slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
        if (segdata & 1) slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
        segmap = segmap >> 8;
        segdata = segdata >> 1;
    }
}

void watch_display_string(char *string, uint8_t position) {
    size_t i = 0;
    while(string[i] != 0) {
        watch_display_character(string[i], position + i);
        i++;
        if (i >= Num_Chars) break;
    }
}

inline void watch_set_colon() {
    slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(1, 16));
}

inline void watch_clear_colon() {
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(1, 16));
}

inline void watch_set_indicator(WatchIndicatorSegment indicator) {
    slcd_sync_seg_on(&SEGMENT_LCD_0, IndicatorSegments[indicator]);
}

inline void watch_clear_indicator(WatchIndicatorSegment indicator) {
    slcd_sync_seg_off(&SEGMENT_LCD_0, IndicatorSegments[indicator]);
}

void watch_clear_all_indicators() {
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(2, 17));
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(2, 16));
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(0, 17));
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(0, 16));
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(1, 10));
}


//////////////////////////////////////////////////////////////////////////////////////////
// Buttons

void watch_enable_buttons() {
    EXTERNAL_IRQ_0_init();
}

void watch_register_button_callback(const uint8_t pin, ext_irq_cb_t callback) {
    if (pin == BTN_ALARM) {
        gpio_set_pin_direction(BTN_ALARM, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(BTN_ALARM, GPIO_PULL_DOWN);
        gpio_set_pin_function(BTN_ALARM, PINMUX_PA02G_RTC_IN2);
        btn_alarm_callback = callback;
        _extwake_register_callback(&CALENDAR_0.device, extwake_callback);
    } else {
        ext_irq_register(pin, callback);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// LED

bool PWM_0_enabled = false;

void watch_enable_led(bool pwm) {
    if (pwm) {
        if (PWM_0_enabled) return;

        PWM_0_init();
        pwm_set_parameters(&PWM_0, 10000, 0);
        pwm_enable(&PWM_0);

        PWM_0_enabled = true;
    } else {
        watch_enable_digital_output(RED);
        watch_enable_digital_output(GREEN);
    }
    watch_set_led_off();
}

void watch_disable_led(bool pwm) {
    if (pwm) {
        if (!PWM_0_enabled) return;
        pwm_disable(&PWM_0);
        PWM_0_enabled = false;
    }

    watch_disable_digital_output(RED);
    watch_disable_digital_output(GREEN);
}

void watch_set_led_color(uint16_t red, uint16_t green) {
    if (PWM_0_enabled) {
        TC3->COUNT16.CC[0].reg = red;
        TC3->COUNT16.CC[1].reg = green;
    }
}

void watch_set_led_red() {
    if (PWM_0_enabled) {
        watch_set_led_color(65535, 0);
    } else {
        watch_set_pin_level(RED, true);
        watch_set_pin_level(GREEN, false);
    }
}

void watch_set_led_green() {
    if (PWM_0_enabled) {
        watch_set_led_color(65535, 0);
    } else {
        watch_set_pin_level(RED, false);
        watch_set_pin_level(GREEN, true);
    }
}

void watch_set_led_yellow() {
    if (PWM_0_enabled) {
        watch_set_led_color(65535, 65535);
    } else {
        watch_set_pin_level(RED, true);
        watch_set_pin_level(GREEN, true);
    }
}

void watch_set_led_off() {
    if (PWM_0_enabled) {
        watch_set_led_color(0, 0);
    } else {
        watch_set_pin_level(RED, false);
        watch_set_pin_level(GREEN, false);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Buzzer

inline void watch_enable_buzzer() {
    PWM_1_init();
}

inline void watch_set_buzzer_period(uint32_t period) {
    pwm_set_parameters(&PWM_1, period, period / 2);
}

inline void watch_set_buzzer_on() {
	pwm_enable(&PWM_1);
}

inline void watch_set_buzzer_off() {
	pwm_disable(&PWM_1);
}

const uint16_t NotePeriods[108] = {31047, 29301, 27649, 26079, 24617, 23224, 21923, 20683, 19515, 18418, 17377, 16399, 15477, 14603, 13780, 13004, 12272, 11580, 10926, 10311, 9730, 9181, 8664, 8175, 7714, 7280, 6869, 6483, 6117, 5772, 5447, 5140, 4850, 4577, 4319, 4076, 3846, 3629, 3425, 3232, 3050, 2878, 2715, 2562, 2418, 2282, 2153, 2032, 1917, 1809, 1707, 1611, 1520, 1435, 1354, 1277, 1205, 1137, 1073, 1013, 956, 902, 851, 803, 758, 715, 675, 637, 601, 567, 535, 505, 476, 450, 424, 400, 378, 357, 336, 317, 300, 283, 267, 252, 238, 224, 212, 200, 188, 178, 168, 158, 149, 141, 133, 125, 118, 112, 105, 99, 94, 89, 84, 79, 74, 70, 66, 63};

void watch_buzzer_play_note(BuzzerNote note, uint16_t duration_ms) {
    if (note == BUZZER_NOTE_REST) {
        watch_set_buzzer_off();
    } else {
        pwm_set_parameters(&PWM_1, NotePeriods[note], NotePeriods[note] / 2);
        watch_set_buzzer_on();
    }
    delay_ms(duration_ms);
    watch_set_buzzer_off();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Real-time Clock

bool _watch_rtc_is_enabled() {
    return RTC->MODE0.CTRLA.bit.ENABLE;
}

void watch_set_date_time(struct calendar_date_time date_time) {
    calendar_set_date(&CALENDAR_0, &date_time.date);
    calendar_set_time(&CALENDAR_0, &date_time.time);
}

void watch_get_date_time(struct calendar_date_time *date_time) {
    calendar_get_date_time(&CALENDAR_0, date_time);
}

void watch_register_tick_callback(ext_irq_cb_t callback) {
    _prescaler_register_callback(&CALENDAR_0.device, callback);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Analog Input

static bool ADC_0_ENABLED = false;

void watch_enable_analog(const uint8_t pin) {
    if (!ADC_0_ENABLED) ADC_0_init();
    ADC_0_ENABLED = true;

    gpio_set_pin_direction(pin, GPIO_DIRECTION_OFF);
    switch (pin) {
        case A0:
            gpio_set_pin_function(A0, PINMUX_PB04B_ADC_AIN12);
            break;
        case A1:
            gpio_set_pin_function(A1, PINMUX_PB01B_ADC_AIN9);
            break;
        case A2:
            gpio_set_pin_function(A2, PINMUX_PB02B_ADC_AIN10);
            break;
        default:
            return;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Digital IO

void watch_enable_digital_input(const uint8_t pin) {
    gpio_set_pin_direction(pin, GPIO_DIRECTION_IN);
    gpio_set_pin_function(pin, GPIO_PIN_FUNCTION_OFF);
}

void watch_enable_pull_up(const uint8_t pin) {
    gpio_set_pin_pull_mode(pin, GPIO_PULL_UP);
}

void watch_enable_pull_down(const uint8_t pin) {
    gpio_set_pin_pull_mode(pin, GPIO_PULL_DOWN);
}

bool watch_get_pin_level(const uint8_t pin) {
    return gpio_get_pin_level(pin);
}

void watch_enable_digital_output(const uint8_t pin) {
    gpio_set_pin_direction(pin, GPIO_DIRECTION_OUT);
    gpio_set_pin_function(pin, GPIO_PIN_FUNCTION_OFF);
}

void watch_disable_digital_output(const uint8_t pin) {
    gpio_set_pin_direction(pin, GPIO_DIRECTION_OFF);
}

void watch_set_pin_level(const uint8_t pin, const bool level) {
    gpio_set_pin_level(pin, level);
}

//////////////////////////////////////////////////////////////////////////////////////////
// I2C

struct io_descriptor *I2C_0_io;

void watch_enable_i2c() {
    I2C_0_init();
    i2c_m_sync_get_io_descriptor(&I2C_0, &I2C_0_io);
    i2c_m_sync_enable(&I2C_0);
}

void watch_i2c_send(int16_t addr, uint8_t *buf, uint16_t length) {
    i2c_m_sync_set_periphaddr(&I2C_0, addr, I2C_M_SEVEN);
    io_write(I2C_0_io, buf, length);
}

void watch_i2c_receive(int16_t addr, uint8_t *buf, uint16_t length) {
    i2c_m_sync_set_periphaddr(&I2C_0, addr, I2C_M_SEVEN);
    io_read(I2C_0_io, buf, length);
}

void watch_i2c_write8(int16_t addr, uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;

    watch_i2c_send(addr, (uint8_t *)&buf, 2);
}

uint8_t watch_i2c_read8(int16_t addr, uint8_t reg) {
    uint8_t data;

    watch_i2c_send(addr, (uint8_t *)&reg, 1);
    watch_i2c_receive(addr, (uint8_t *)&data, 1);

    return data;
}

uint16_t watch_i2c_read16(int16_t addr, uint8_t reg) {
    uint16_t data;

    watch_i2c_send(addr, (uint8_t *)&reg, 1);
    watch_i2c_receive(addr, (uint8_t *)&data, 2);

    return data;
}

uint32_t watch_i2c_read24(int16_t addr, uint8_t reg) {
    uint32_t data;
    data = 0;

    watch_i2c_send(addr, (uint8_t *)&reg, 1);
    watch_i2c_receive(addr, (uint8_t *)&data, 3);

    return data << 8;
}

uint32_t watch_i2c_read32(int16_t addr, uint8_t reg) {
    uint32_t data;

    watch_i2c_send(addr, (uint8_t *)&reg, 1);
    watch_i2c_receive(addr, (uint8_t *)&data, 4);

    return data;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Debug UART

/*
 * UART methods are Copyright (c) 2014-2017, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

void watch_enable_debug_uart(uint32_t baud) {
    uint64_t br = (uint64_t)65536 * (CONF_CPU_FREQUENCY - 16 * baud) / CONF_CPU_FREQUENCY;

    gpio_set_pin_direction(D1, GPIO_DIRECTION_IN);
    gpio_set_pin_function(D1, PINMUX_PB00C_SERCOM3_PAD2);

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOM3;

    GCLK->PCHCTRL[SERCOM3_GCLK_ID_CORE].reg = GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
    while (0 == (GCLK->PCHCTRL[SERCOM3_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));

    SERCOM3->USART.CTRLA.reg =
        SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE(1/*USART_INT_CLK*/) |
        SERCOM_USART_CTRLA_RXPO(0/*PAD0*/) | SERCOM_USART_CTRLA_TXPO(1/*PAD2*/);

    SERCOM3->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN |
        SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);

    SERCOM3->USART.BAUD.reg = (uint16_t)br;

    SERCOM3->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}

void watch_debug_putc(char c) {
    while (!(SERCOM3->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
    SERCOM3->USART.DATA.reg = c;
}

void watch_debug_puts(char *s) {
    while (*s) watch_debug_putc(*s++);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Deep Sleep

static void extwake_callback(uint8_t reason) {
    if (reason & RTC_TAMPID_TAMPID2) {
        if (btn_alarm_callback != NULL) btn_alarm_callback();
    } else if (reason & RTC_TAMPID_TAMPID1) {
        if (a2_callback != NULL) a2_callback();
    } else if (reason & RTC_TAMPID_TAMPID0) {
        if (d1_callback != NULL) d1_callback();
    }
}

void watch_register_extwake_callback(uint8_t pin, ext_irq_cb_t callback) {
    uint32_t pinmux;
    if (pin == D1) {
        d1_callback = callback;
        pinmux = PINMUX_PB00G_RTC_IN0;
    } else if (pin == A2) {
        a2_callback = callback;
        pinmux = PINMUX_PB02G_RTC_IN1;
    } else {
        return;
    }
    gpio_set_pin_direction(pin, GPIO_DIRECTION_IN);
    gpio_set_pin_function(pin, pinmux);
    _extwake_register_callback(&CALENDAR_0.device, extwake_callback);
}

void watch_store_backup_data(uint32_t data, uint8_t reg) {
    if (reg < 8) {
        RTC->MODE0.BKUP[reg].reg = data;
    }
}

uint32_t watch_get_backup_data(uint8_t reg) {
    if (reg < 8) {
        return RTC->MODE0.BKUP[reg].reg;
    }

    return 0;
}

void watch_enter_deep_sleep() {
    // enable and configure the external wake interrupt, if not already set up.
    if (btn_alarm_callback == NULL && a2_callback == NULL && d1_callback == NULL) {
        gpio_set_pin_direction(BTN_ALARM, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(BTN_ALARM, GPIO_PULL_DOWN);
        gpio_set_pin_function(BTN_ALARM, PINMUX_PA02G_RTC_IN2);
        _extwake_register_callback(&CALENDAR_0.device, extwake_callback);
    }

    // disable SLCD
    slcd_sync_deinit(&SEGMENT_LCD_0);
    hri_mclk_clear_APBCMASK_SLCD_bit(SLCD);

    // TODO: disable other peripherals

    // go into backup sleep mode
    sleep(5);
}
