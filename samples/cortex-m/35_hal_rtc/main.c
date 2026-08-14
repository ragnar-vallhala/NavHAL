/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file main.c
 * @brief RTC calendar over the USB serial port.
 *
 * @details
 * Prints the date and time once a second, along with the oscillator driving
 * the calendar and a boot counter kept in a backup register.
 *
 * A wakeup timer runs every 5 seconds and an alarm fires on second 30 of every
 * minute, both through their interrupts; the line marks each one that has
 * arrived since the last print.
 *
 * The point of the sample is what happens across a reset: the calendar keeps
 * counting and the boot counter keeps incrementing, because both live in the
 * backup domain, which the reset line does not reach. Press reset (or reflash)
 * while watching, and the clock carries on from where it was.
 *
 * @code
 * screen /dev/ttyACM0
 * @endcode
 */

#include "board.h"
#include "navhal.h"

static hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSE,
    .pll_m = 8,
    .pll_n = 336,
    .pll_p = 4,
    .pll_q = 7,
};
static hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

#define BACKUP_REG_BOOTS 0

static volatile uint32_t wakeups;
static volatile uint32_t alarms;

static void on_wakeup(void) { wakeups++; }
static void on_alarm(void) { alarms++; }

static char line[96];
static uint16_t put_str(char *p, const char *s) {
  uint16_t i = 0;
  while (s[i]) {
    p[i] = s[i];
    i++;
  }
  return i;
}
static uint16_t put_uint(char *p, uint32_t v, uint8_t width) {
  char tmp[12];
  uint8_t n = 0;
  do {
    tmp[n++] = (char)('0' + (v % 10U));
    v /= 10U;
  } while (v);
  uint16_t i = 0;
  while (n < width--)
    p[i++] = '0';
  while (n)
    p[i++] = tmp[--n];
  return i;
}

int main(void) {
  hal_clock_init(&clk_cfg, &pll_cfg);
  hal_timebase_init(1000);
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);

  hal_rtc_init(&(hal_rtc_config_t){.clock = HAL_RTC_CLOCK_AUTO});

  /* Only seed the calendar on a cold start — a reset must not rewind it. */
  if (!hal_rtc_is_set()) {
    hal_rtc_set_datetime(&(hal_rtc_datetime_t){
        .year = 2026, .month = 8, .day = 14, .weekday = HAL_RTC_FRIDAY,
        .hour = 12, .minute = 0, .second = 0});
    hal_rtc_backup_write(BACKUP_REG_BOOTS, 0);
  }

  uint32_t boots = 0;
  hal_rtc_backup_read(BACKUP_REG_BOOTS, &boots);
  hal_rtc_backup_write(BACKUP_REG_BOOTS, boots + 1);

  /* Every 5 s, and on second 30 of every minute — the alarm names only the
   * second, so the hardware repeats it without being re-armed. */
  hal_rtc_set_wakeup(5000, on_wakeup);
  hal_rtc_set_alarm(HAL_RTC_ALARM_A,
                    &(hal_rtc_alarm_config_t){.second = 30,
                                              .match = HAL_RTC_MATCH_SECOND},
                    on_alarm);

  hal_usb_cdc_init();

  uint32_t last = 0;
  while (1) {
    uint32_t ms = hal_timebase_get_millis();

    if (hal_usb_cdc_connected() && (ms - last) >= 1000u) {
      last = ms;

      hal_rtc_datetime_t now;
      hal_rtc_get_datetime(&now);
      hal_rtc_clock_t src = hal_rtc_get_clock();

      uint16_t n = 0;
      n += put_uint(line + n, now.year, 4);
      n += put_str(line + n, "-");
      n += put_uint(line + n, now.month, 2);
      n += put_str(line + n, "-");
      n += put_uint(line + n, now.day, 2);
      n += put_str(line + n, " ");
      n += put_uint(line + n, now.hour, 2);
      n += put_str(line + n, ":");
      n += put_uint(line + n, now.minute, 2);
      n += put_str(line + n, ":");
      n += put_uint(line + n, now.second, 2);
      n += put_str(line + n, "  wday=");
      n += put_uint(line + n, now.weekday, 1);
      n += put_str(line + n, "  clock=");
      n += put_str(line + n, src == HAL_RTC_CLOCK_LSE   ? "LSE"
                             : src == HAL_RTC_CLOCK_LSI ? "LSI"
                                                        : "none");
      n += put_str(line + n, "  boots=");
      n += put_uint(line + n, boots + 1, 1);
      n += put_str(line + n, "  wakeups=");
      n += put_uint(line + n, wakeups, 1);
      n += put_str(line + n, "  alarms=");
      n += put_uint(line + n, alarms, 1);
      n += put_str(line + n, "\r\n");
      hal_usb_cdc_write((const uint8_t *)line, n);
    }

    hal_gpio_write(LED_BUILTIN, (ms % 1000u) < 30u ? LED_ON : LED_OFF);
  }
}
