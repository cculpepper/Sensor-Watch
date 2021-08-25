/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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
