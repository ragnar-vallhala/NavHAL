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
 * @file rtc.c
 * @brief RTC calendar driver for STM32F4.
 *
 * @details
 * The calendar counts in BCD off a 1 Hz tick divided down from the low-speed
 * oscillator by a two-stage prescaler. The driver keeps it in 24-hour mode and
 * converts at the API boundary, so nothing above this file sees BCD.
 *
 * Reference: RM0368 §21.
 */

#include "common/hal_rtc.h"

#if NAVHAL_CONFIG_DRV_RTC

#include "family/exti_reg.h"
#include "family/rcc_reg.h"
#include "family/rtc_reg.h"
#include "navhal_port_clock.h"
#include "navhal_port_interrupt.h"
#include <stdint.h>

/* Waiting for the crystal is the one place a real-time bound matters, so it is
 * derived from the system clock rather than being a magic spin count. The
 * estimate assumes roughly ten core cycles per poll of an APB register; it is
 * approximate by nature, and generous enough that a healthy crystal is never
 * cut short. */
#define LSE_DEFAULT_TIMEOUT_MS 5000U
#define CYCLES_PER_POLL 10U
#define LSI_TIMEOUT_SPINS 100000UL
/* Entering initialization mode costs up to two RTC clock cycles — an eternity
 * in CPU terms at 32 kHz, hence the generous bound. */
#define RTC_TIMEOUT_SPINS 2000000UL

/* Prescalers dividing the oscillator down to 1 Hz. Async first (it runs the
 * low-power counter), then sync: (async + 1) * (sync + 1) = f_osc.
 * 128 * 256 = 32768 for the crystal; 128 * 250 = 32000 for the nominal RC. */
#define PRESCALER_ASYNC 127U
#define PRESCALER_SYNC_LSE 255U
#define PRESCALER_SYNC_LSI 249U

#define WAIT_OR_TIMEOUT(cond, spins)                                           \
  do {                                                                         \
    uint32_t _to = (spins);                                                    \
    while (cond) {                                                             \
      if (--_to == 0u)                                                         \
        return HAL_ERR_TIMEOUT;                                                \
    }                                                                          \
  } while (0)

static uint8_t rtc_ready; /* hal_rtc_init() has run and the RTC is ticking */

static uint8_t to_bcd(uint8_t v) {
  return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

static uint8_t from_bcd(uint8_t v) {
  return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU));
}

/* The calendar registers ignore writes unless this key sequence has been
 * written to WPR; any other value re-arms the protection. */
static void write_unlock(void) {
  RTC->WPR = RTC_WPR_KEY1;
  RTC->WPR = RTC_WPR_KEY2;
}

static void write_lock(void) { RTC->WPR = RTC_WPR_LOCK; }

/* Stopping the calendar to write to it. Time does not advance between here and
 * init_exit(), so callers keep it short. */
static hal_status_t init_enter(void) {
  RTC->ISR |= RTC_ISR_INIT;
  WAIT_OR_TIMEOUT(!(RTC->ISR & RTC_ISR_INITF), RTC_TIMEOUT_SPINS);
  return HAL_OK;
}

static void init_exit(void) { RTC->ISR &= ~RTC_ISR_INIT; }

/* Clear one status flag. The flags clear on a written 0 and ignore a written 1,
 * so the obvious ~FLAG looks right — but INIT lives in the same register and
 * does not ignore a 1: it stops the calendar and puts it into initialization
 * mode. Mask INIT out of every flag write; the wakeup timer keeps counting
 * either way, so a calendar frozen this way is easy to miss. */
static void clear_flag(uint32_t flag) { RTC->ISR = ~(flag | RTC_ISR_INIT); }

/* The shadow copies the CPU reads are refreshed once per RTC cycle. After
 * leaving init mode the old contents are stale, so wait for the refresh rather
 * than hand back a timestamp from before the write. */
static hal_status_t wait_sync(void) {
  RTC->ISR &= ~RTC_ISR_RSF;
  WAIT_OR_TIMEOUT(!(RTC->ISR & RTC_ISR_RSF), RTC_TIMEOUT_SPINS);
  return HAL_OK;
}

static hal_status_t start_lse(uint16_t timeout_ms) {
  uint32_t ms = timeout_ms ? timeout_ms : LSE_DEFAULT_TIMEOUT_MS;
  uint32_t sysclk = hal_clock_get_sysclk();
  if (sysclk == 0u)
    sysclk = 16000000u; /* clock driver could not tell; assume the reset HSI */
  uint32_t spins = (sysclk / 1000u) * ms / CYCLES_PER_POLL;

  RCC->BDCR |= RCC_BDCR_LSEON;
  WAIT_OR_TIMEOUT(!(RCC->BDCR & RCC_BDCR_LSERDY), spins);
  return HAL_OK;
}

static hal_status_t start_lsi(void) {
  RCC->CSR |= RCC_CSR_LSION;
  WAIT_OR_TIMEOUT(!(RCC->CSR & RCC_CSR_LSIRDY), LSI_TIMEOUT_SPINS);
  return HAL_OK;
}

hal_status_t hal_rtc_init(const hal_rtc_config_t *cfg) {
  hal_rtc_clock_t want = cfg ? cfg->clock : HAL_RTC_CLOCK_AUTO;

  /* Every write below lands in the backup domain, which is read-only until the
   * power controller unlocks it — and the power controller needs its own clock
   * first. Left unlocked afterwards so the backup registers stay writable. */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR_CR |= PWR_CR_DBP;

  /* A calendar left running by a previous boot is adopted as-is. Re-running the
   * configuration would mean resetting the backup domain, which throws away the
   * time and the backup registers — exactly what the RTC exists to protect.
   *
   * The selection survives the reset; the oscillator behind it may not. LSEON
   * lives in the backup domain and keeps the crystal running, but LSION lives in
   * RCC_CSR, which the system reset clears — so an RC-driven calendar comes back
   * configured, adopted, and quietly stopped, reading its power-on default. Get
   * the clock going again before calling it good. */
  uint32_t running = RCC->BDCR & RCC_BDCR_RTCSEL_MASK;
  /* Naming an oscillator that is not the one running is a request to
   * reconfigure, and that costs the calendar and the backup registers. AUTO
   * never asks for it — otherwise a crystal that is merely slow this boot would
   * wipe the clock it was supposed to keep. */
  uint8_t mismatch = (want == HAL_RTC_CLOCK_LSE && running != RCC_BDCR_RTCSEL_LSE) ||
                     (want == HAL_RTC_CLOCK_LSI && running != RCC_BDCR_RTCSEL_LSI);

  if ((RCC->BDCR & RCC_BDCR_RTCEN) && (RTC->ISR & RTC_ISR_INITS) && !mismatch) {
    if (running == RCC_BDCR_RTCSEL_LSI)
      HAL_OK_OR_RETURN(start_lsi());
    else if (!(RCC->BDCR & RCC_BDCR_LSERDY))
      HAL_OK_OR_RETURN(start_lse(cfg ? cfg->lse_timeout_ms : 0));
    rtc_ready = 1;
    /* The shadow copies were cleared by the reset; do not hand back a
     * power-on default as a timestamp. */
    return wait_sync();
  }

  hal_status_t st;
  uint32_t source = RCC_BDCR_RTCSEL_LSI;
  uint32_t sync = PRESCALER_SYNC_LSI;

  if (want == HAL_RTC_CLOCK_LSE || want == HAL_RTC_CLOCK_AUTO) {
    st = start_lse(cfg ? cfg->lse_timeout_ms : 0);
    if (st == HAL_OK) {
      source = RCC_BDCR_RTCSEL_LSE;
      sync = PRESCALER_SYNC_LSE;
    } else if (want == HAL_RTC_CLOCK_LSE) {
      return st; /* asked for the crystal by name and it is not there */
    } else {
      RCC->BDCR &= ~RCC_BDCR_LSEON; /* stop driving a crystal that never started */
    }
  }

  if (source == RCC_BDCR_RTCSEL_LSI)
    HAL_OK_OR_RETURN(start_lsi());

  /* RTCSEL is write-once per backup-domain power-on: the only way to change it
   * is to reset the whole domain. Doing that here is safe precisely because the
   * early return above means the RTC was not already running. */
  RCC->BDCR |= RCC_BDCR_BDRST;
  RCC->BDCR &= ~RCC_BDCR_BDRST;

  /* The domain reset cleared LSEON, so re-start whichever oscillator won. */
  if (source == RCC_BDCR_RTCSEL_LSE)
    HAL_OK_OR_RETURN(start_lse(cfg ? cfg->lse_timeout_ms : 0));

  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_MASK) | source;
  RCC->BDCR |= RCC_BDCR_RTCEN;

  write_unlock();
  st = init_enter();
  if (st != HAL_OK) {
    write_lock();
    return st;
  }
  /* Async before sync in one write: the two fields must be programmed together
   * and the async stage has to be the larger divider for the low-power path. */
  RTC->PRER = RTC_PRER_ASYNC(PRESCALER_ASYNC) | RTC_PRER_SYNC(sync);
  RTC->CR &= ~(RTC_CR_FMT | RTC_CR_BYPSHAD); /* 24-hour, read through shadows */
  init_exit();
  st = wait_sync();
  write_lock();
  if (st != HAL_OK)
    return st;

  rtc_ready = 1;
  return HAL_OK;
}

hal_status_t hal_rtc_set_datetime(const hal_rtc_datetime_t *dt) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (!dt || dt->year < 2000U || dt->year > 2099U || dt->month < 1U ||
      dt->month > 12U || dt->day < 1U || dt->day > 31U || dt->weekday < 1U ||
      dt->weekday > 7U || dt->hour > 23U || dt->minute > 59U ||
      dt->second > 59U)
    return HAL_ERR_INVALID_ARG;

  uint32_t tr = ((uint32_t)to_bcd(dt->hour) << RTC_TR_HOUR_SHIFT) |
                ((uint32_t)to_bcd(dt->minute) << RTC_TR_MINUTE_SHIFT) |
                ((uint32_t)to_bcd(dt->second) << RTC_TR_SECOND_SHIFT);
  uint32_t dr =
      ((uint32_t)to_bcd((uint8_t)(dt->year - 2000U)) << RTC_DR_YEAR_SHIFT) |
      ((uint32_t)dt->weekday << RTC_DR_WEEKDAY_SHIFT) |
      ((uint32_t)to_bcd(dt->month) << RTC_DR_MONTH_SHIFT) |
      ((uint32_t)to_bcd(dt->day) << RTC_DR_DAY_SHIFT);

  write_unlock();
  hal_status_t st = init_enter();
  if (st != HAL_OK) {
    write_lock();
    return st;
  }
  RTC->TR = tr;
  RTC->DR = dr;
  init_exit();
  st = wait_sync();
  write_lock();
  return st;
}

hal_status_t hal_rtc_get_datetime(hal_rtc_datetime_t *dt) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (!dt)
    return HAL_ERR_INVALID_ARG;

  /* TR must be read before DR: reading TR freezes the date until DR is read,
   * which is what stops a midnight rollover between the two from handing back
   * tomorrow's date with today's time. */
  uint32_t tr = RTC->TR;
  uint32_t dr = RTC->DR;

  dt->hour = from_bcd((uint8_t)((tr >> RTC_TR_HOUR_SHIFT) & 0x3FU));
  dt->minute = from_bcd((uint8_t)((tr >> RTC_TR_MINUTE_SHIFT) & 0x7FU));
  dt->second = from_bcd((uint8_t)((tr >> RTC_TR_SECOND_SHIFT) & 0x7FU));
  dt->day = from_bcd((uint8_t)((dr >> RTC_DR_DAY_SHIFT) & 0x3FU));
  dt->month = from_bcd((uint8_t)((dr >> RTC_DR_MONTH_SHIFT) & 0x1FU));
  dt->weekday = (uint8_t)((dr >> RTC_DR_WEEKDAY_SHIFT) & 0x07U);
  dt->year =
      (uint16_t)(2000U + from_bcd((uint8_t)((dr >> RTC_DR_YEAR_SHIFT) & 0xFFU)));
  return HAL_OK;
}

bool hal_rtc_is_set(void) {
  return (RCC->BDCR & RCC_BDCR_RTCEN) && (RTC->ISR & RTC_ISR_INITS);
}

hal_rtc_clock_t hal_rtc_get_clock(void) {
  if (!(RCC->BDCR & RCC_BDCR_RTCEN))
    return HAL_RTC_CLOCK_NONE;
  switch (RCC->BDCR & RCC_BDCR_RTCSEL_MASK) {
  case RCC_BDCR_RTCSEL_LSE:
    return HAL_RTC_CLOCK_LSE;
  case RCC_BDCR_RTCSEL_LSI:
    return HAL_RTC_CLOCK_LSI;
  default:
    return HAL_RTC_CLOCK_NONE;
  }
}

/* -------------------------------------------------------------------------- *
 * Alarms and the wakeup timer
 *
 * Both reach the NVIC through the EXTI, and both are wired to it as internal
 * lines that only ever pulse: a line left masked there keeps its RTC flag set
 * and never interrupts anything, which is the classic "the alarm fires but
 * nothing happens" bug.
 * -------------------------------------------------------------------------- */

static hal_rtc_callback_t alarm_cb[2];
static hal_rtc_callback_t wakeup_cb;

static void rtc_alarm_isr(void) {
  uint32_t isr = RTC->ISR;

  if (isr & RTC_ISR_ALRAF) {
    clear_flag(RTC_ISR_ALRAF);
    if (alarm_cb[HAL_RTC_ALARM_A])
      alarm_cb[HAL_RTC_ALARM_A]();
  }
  if (isr & RTC_ISR_ALRBF) {
    clear_flag(RTC_ISR_ALRBF);
    if (alarm_cb[HAL_RTC_ALARM_B])
      alarm_cb[HAL_RTC_ALARM_B]();
  }
  EXTI->PR = EXTI_LINE_RTC_ALARM;
}

static void rtc_wakeup_isr(void) {
  clear_flag(RTC_ISR_WUTF);
  EXTI->PR = EXTI_LINE_RTC_WAKEUP;
  if (wakeup_cb)
    wakeup_cb();
}

static void exti_line_enable(uint32_t line, uint8_t on) {
  if (on) {
    EXTI->RTSR |= line; /* the RTC drives these lines with a rising pulse */
    EXTI->IMR |= line;
  } else {
    EXTI->IMR &= ~line;
  }
  EXTI->PR = line;
}

hal_status_t hal_rtc_set_alarm(hal_rtc_alarm_t alarm,
                               const hal_rtc_alarm_config_t *cfg,
                               hal_rtc_callback_t cb) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (!cfg || (alarm != HAL_RTC_ALARM_A && alarm != HAL_RTC_ALARM_B))
    return HAL_ERR_INVALID_ARG;
  if (cfg->second > 59U || cfg->minute > 59U || cfg->hour > 23U)
    return HAL_ERR_INVALID_ARG;
  /* A field the alarm does not compare is never read, so it does not have to
   * hold a sensible value — "every minute at second 30" leaves day at 0. */
  if ((cfg->match & HAL_RTC_MATCH_DAY) &&
      (cfg->day < 1U || cfg->day > (cfg->day_is_weekday ? 7U : 31U)))
    return HAL_ERR_INVALID_ARG;

  /* The register masks say which fields to *ignore*, so invert the request. */
  uint32_t reg = ((uint32_t)to_bcd(cfg->second) << RTC_ALRM_SECOND_SHIFT) |
                 ((uint32_t)to_bcd(cfg->minute) << RTC_ALRM_MINUTE_SHIFT) |
                 ((uint32_t)to_bcd(cfg->hour) << RTC_ALRM_HOUR_SHIFT) |
                 ((uint32_t)to_bcd(cfg->day) << RTC_ALRM_DAY_SHIFT);
  if (!(cfg->match & HAL_RTC_MATCH_SECOND))
    reg |= RTC_ALRM_MSK_SECOND;
  if (!(cfg->match & HAL_RTC_MATCH_MINUTE))
    reg |= RTC_ALRM_MSK_MINUTE;
  if (!(cfg->match & HAL_RTC_MATCH_HOUR))
    reg |= RTC_ALRM_MSK_HOUR;
  if (!(cfg->match & HAL_RTC_MATCH_DAY))
    reg |= RTC_ALRM_MSK_DAY;
  if (cfg->day_is_weekday)
    reg |= RTC_ALRM_WDSEL;

  uint32_t enable = (alarm == HAL_RTC_ALARM_A) ? RTC_CR_ALRAE : RTC_CR_ALRBE;
  uint32_t irq_en = (alarm == HAL_RTC_ALARM_A) ? RTC_CR_ALRAIE : RTC_CR_ALRBIE;
  uint32_t writable = (alarm == HAL_RTC_ALARM_A) ? RTC_ISR_ALRAWF : RTC_ISR_ALRBWF;
  uint32_t fired = (alarm == HAL_RTC_ALARM_A) ? RTC_ISR_ALRAF : RTC_ISR_ALRBF;

  alarm_cb[alarm] = cb;

  write_unlock();
  /* An armed alarm's registers are read-only; disabling it is what makes them
   * writable, and the hardware takes up to two RTC cycles to say so. */
  RTC->CR &= ~(enable | irq_en);
  uint32_t to = RTC_TIMEOUT_SPINS;
  while (!(RTC->ISR & writable)) {
    if (--to == 0u) {
      write_lock();
      return HAL_ERR_TIMEOUT;
    }
  }

  if (alarm == HAL_RTC_ALARM_A)
    RTC->ALRMAR = reg;
  else
    RTC->ALRMBR = reg;

  clear_flag(fired);
  RTC->CR |= enable | (cb ? irq_en : 0u);
  write_lock();

  exti_line_enable(EXTI_LINE_RTC_ALARM, cb != 0);
  if (cb) {
    hal_interrupt_attach_callback(RTC_Alarm_IRQn, rtc_alarm_isr);
    hal_interrupt_enable(RTC_Alarm_IRQn);
  }
  return HAL_OK;
}

hal_status_t hal_rtc_cancel_alarm(hal_rtc_alarm_t alarm) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (alarm != HAL_RTC_ALARM_A && alarm != HAL_RTC_ALARM_B)
    return HAL_ERR_INVALID_ARG;

  uint32_t enable = (alarm == HAL_RTC_ALARM_A) ? RTC_CR_ALRAE : RTC_CR_ALRBE;
  uint32_t irq_en = (alarm == HAL_RTC_ALARM_A) ? RTC_CR_ALRAIE : RTC_CR_ALRBIE;
  uint32_t fired = (alarm == HAL_RTC_ALARM_A) ? RTC_ISR_ALRAF : RTC_ISR_ALRBF;

  write_unlock();
  RTC->CR &= ~(enable | irq_en);
  clear_flag(fired);
  write_lock();

  alarm_cb[alarm] = 0;
  /* The line is shared by both alarms — only mask it once neither wants it. */
  if (!alarm_cb[HAL_RTC_ALARM_A] && !alarm_cb[HAL_RTC_ALARM_B])
    exti_line_enable(EXTI_LINE_RTC_ALARM, 0);
  return HAL_OK;
}

bool hal_rtc_alarm_fired(hal_rtc_alarm_t alarm) {
  if (!rtc_ready || (alarm != HAL_RTC_ALARM_A && alarm != HAL_RTC_ALARM_B))
    return false;
  uint32_t fired = (alarm == HAL_RTC_ALARM_A) ? RTC_ISR_ALRAF : RTC_ISR_ALRBF;
  if (!(RTC->ISR & fired))
    return false;
  clear_flag(fired);
  return true;
}

hal_status_t hal_rtc_set_wakeup(uint32_t period_ms, hal_rtc_callback_t cb) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (period_ms == 0u || period_ms > 65536000u)
    return HAL_ERR_INVALID_ARG;

  /* Two ranges, and the counter is 16 bits in both. Short periods count the
   * oscillator over 16 (2048 Hz on a 32.768 kHz crystal), which reaches 32 s;
   * longer ones count the calendar's own 1 Hz tick, out to about 18 hours. */
  uint32_t tick_hz =
      (hal_rtc_get_clock() == HAL_RTC_CLOCK_LSE) ? 32768u / 16u : 32000u / 16u;
  uint32_t select, reload;

  if (period_ms <= (65536u * 1000u) / tick_hz) {
    select = RTC_CR_WUCKSEL_DIV16;
    reload = (period_ms * tick_hz) / 1000u;
    if (reload == 0u)
      reload = 1u;
  } else {
    select = RTC_CR_WUCKSEL_SPRE;
    reload = period_ms / 1000u;
  }
  if (reload > 65536u)
    return HAL_ERR_INVALID_ARG;

  wakeup_cb = cb;

  write_unlock();
  /* Same rule as the alarms: stop it before its registers will take a write. */
  RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
  uint32_t to = RTC_TIMEOUT_SPINS;
  while (!(RTC->ISR & RTC_ISR_WUTWF)) {
    if (--to == 0u) {
      write_lock();
      return HAL_ERR_TIMEOUT;
    }
  }

  RTC->WUTR = reload - 1u; /* counts reload-1 down to 0, so one full period */
  RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL_MASK) | select;
  clear_flag(RTC_ISR_WUTF);
  RTC->CR |= RTC_CR_WUTE | (cb ? RTC_CR_WUTIE : 0u);
  write_lock();

  exti_line_enable(EXTI_LINE_RTC_WAKEUP, cb != 0);
  if (cb) {
    hal_interrupt_attach_callback(RTC_WKUP_IRQn, rtc_wakeup_isr);
    hal_interrupt_enable(RTC_WKUP_IRQn);
  }
  return HAL_OK;
}

hal_status_t hal_rtc_cancel_wakeup(void) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;

  write_unlock();
  RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
  clear_flag(RTC_ISR_WUTF);
  write_lock();

  wakeup_cb = 0;
  exti_line_enable(EXTI_LINE_RTC_WAKEUP, 0);
  return HAL_OK;
}

bool hal_rtc_wakeup_fired(void) {
  if (!rtc_ready || !(RTC->ISR & RTC_ISR_WUTF))
    return false;
  clear_flag(RTC_ISR_WUTF);
  return true;
}

hal_status_t hal_rtc_backup_write(uint8_t index, uint32_t value) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (index >= HAL_RTC_BACKUP_COUNT)
    return HAL_ERR_INVALID_ARG;
  RTC->BKPR[index] = value;
  return HAL_OK;
}

hal_status_t hal_rtc_backup_read(uint8_t index, uint32_t *value) {
  if (!rtc_ready)
    return HAL_ERR_NOT_INITIALIZED;
  if (index >= HAL_RTC_BACKUP_COUNT || !value)
    return HAL_ERR_INVALID_ARG;
  *value = RTC->BKPR[index];
  return HAL_OK;
}

#endif /* NAVHAL_CONFIG_DRV_RTC */
