#include "watch.h"
#include <stdlib.h>

void watch_init() {
    // Use switching regulator for lower power consumption.
    SUPC->VREG.bit.SEL = 1;
    while(!SUPC->STATUS.bit.VREGRDY);

    // External wake depends on RTC; calendar is a required module.
    CALENDAR_0_init();
    calendar_enable(&CALENDAR_0);

    // Not sure if this belongs in every app -- is there a power impact?
    delay_driver_init();
}

static const uint8_t Character_Set[] =
{
    0b00000000, //  
    0b00000000, // !
    0b00100010, // "
    0b00000000, // #
    0b00000000, // $
    0b00000000, // %
    0b01000100, // &
    0b00100000, // '
    0b00000000, // (
    0b00000000, // )
    0b00000000, // *
    0b11000000, // +
    0b00010000, // ,
    0b01000000, // -
    0b00000100, // .
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
    0b00000000, // :
    0b00000000, // ;
    0b01011000, // <
    0b01001000, // =
    0b01001100, // >
    0b01010011, // ?
    0b11111111, // @
    0b01110111, // A
    0b01111111, // B
    0b00111001, // C
    0b00111111, // D
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b10001001, // I
    0b00001110, // J
    0b11101010, // K
    0b00111000, // L
    0b10110111, // M
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b11110111, // R
    0b01101101, // S
    0b10000001, // T
    0b00111110, // U
    0b00111110, // V
    0b10111110, // W
    0b01111110, // X
    0b01101110, // Y
    0b00011011, // Z
    0b00111001, // [
    0b00100100, // backslash
    0b00001111, // ]
    0b00100110, // ^
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
    0b01000010, // j
    0b11101010, // k
    0b00110000, // l
    0b10110111, // m
    0b01010100, // n
    0b01011100, // o
    0b01110011, // p
    0b01100111, // q
    0b01010000, // r
    0b01101101, // s
    0b01111000, // t
    0b01100010, // u
    0b01100010, // v
    0b10111110, // w
    0b01111110, // x
    0b01101110, // y
    0b00011011, // z
    0b00111001, // {
    0b00110000, // |
    0b00001111, // }
    0b00000001, // ~
};

static const uint64_t Segment_Map[] = {
    0x4e4f0e8e8f8d4d0d, // Position 8
    0xc8c4c4c8b4b4b0b,  // Position 9
    0xc049c00a49890949, // Position 6
    0xc048088886874707, // Position 7
    0xc053921252139352, // Position 0
    0xc054511415559594, // Position 1
    0xc057965616179716, // Position 2
    0xc041804000018a81, // Position 3
    0xc043420203048382, // Position 4
    0xc045440506468584, // Position 5
};

static const uint8_t Num_Chars = 10;

void watch_enable_display() {
    SEGMENT_LCD_0_init();
    slcd_sync_enable(&SEGMENT_LCD_0);
}

void watch_display_pixel(uint8_t com, uint8_t seg) {
    slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
}

void watch_clear_pixel(uint8_t com, uint8_t seg) {
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
}

void watch_display_character(uint8_t character, uint8_t position) {
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

void watch_enable_buttons() {
    EXTERNAL_IRQ_0_init();
}

void watch_register_button_callback(const uint32_t pin, ext_irq_cb_t callback) {
    ext_irq_register(pin, callback);
}

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

bool watch_rtc_is_enabled() {
    return RTC->MODE0.CTRLA.bit.ENABLE;
}

void watch_set_date_time(struct calendar_date_time date_time) {
    calendar_set_date(&CALENDAR_0, &date_time.date);
    calendar_set_time(&CALENDAR_0, &date_time.time);
}

void watch_get_date_time(struct calendar_date_time *date_time) {
    calendar_get_date_time(&CALENDAR_0, date_time);
}

static ext_irq_cb_t tick_user_callback;

static void tick_callback(struct calendar_dev *const dev) {
    tick_user_callback();
}

void watch_enable_tick_callback(ext_irq_cb_t callback) {
    tick_user_callback = callback;
    // TODO: rename this method to reflect that it now sets the PER7 interrupt.
    _tamper_register_callback(&CALENDAR_0.device, &tick_callback);
}

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

struct io_descriptor *I2C_0_io;

void watch_enable_i2c() {
    I2C_0_init();
    i2c_m_sync_get_io_descriptor(&I2C_0, &I2C_0_io);
    i2c_m_sync_enable(&I2C_0);
}

void watch_i2c_send(int16_t addr, uint8_t *buf, uint16_t length) {
    i2c_m_sync_set_slaveaddr(&I2C_0, addr, I2C_M_SEVEN);
    io_write(I2C_0_io, buf, length);
}

void watch_i2c_receive(int16_t addr, uint8_t *buf, uint16_t length) {
    i2c_m_sync_set_slaveaddr(&I2C_0, addr, I2C_M_SEVEN);
    io_read(I2C_0_io, buf, length);
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

void watch_enter_deep_sleep(){
    // Not yet implemented.
    // TODO: enable tamper interrupt on ALARM pin.
    // sleep(5);
}
