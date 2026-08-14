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
 * @file hal_rtc.h
 * @brief Portable HAL interface for the real-time clock and its backup memory.
 *
 * @details
 * A calendar that keeps running while the rest of the chip is reset, plus a
 * handful of 32-bit registers in the same always-on domain. There is one RTC
 * per target, so — like ::hal_sdio — the functions take no instance id.
 *
 * The RTC counts from its own low-speed oscillator, not the system clock:
 * an external 32.768 kHz crystal (LSE) if the board has one, otherwise the
 * internal RC (LSI), which is typically ±5% and drifts with temperature.
 * ::hal_rtc_init picks the crystal when it starts and falls back on its own,
 * and ::hal_rtc_get_clock says which one is actually running — worth checking
 * before trusting a timestamp taken hours after it was set.
 *
 * Time survives a reset, a watchdog, and a reflash. It survives losing power
 * only if the board keeps VBAT alive with a coin cell or supercap; with VBAT
 * tied to the main rail, every power cycle starts the calendar cold.
 * ::hal_rtc_is_set distinguishes "running with a time somebody set" from
 * "powered up with a default", so an application knows whether to go and ask
 * the GPS what day it is.
 *
 * The whole API compiles only when @c NAVHAL_CONFIG_DRV_RTC is set.
 *
 * ### Typical usage
 * @code
 * hal_rtc_init(&(hal_rtc_config_t){.clock = HAL_RTC_CLOCK_AUTO});
 *
 * if (!hal_rtc_is_set())   // cold start — nothing has set the calendar yet
 *   hal_rtc_set_datetime(&(hal_rtc_datetime_t){
 *       .year = 2026, .month = 8, .day = 14, .weekday = HAL_RTC_FRIDAY,
 *       .hour = 17, .minute = 5, .second = 0});
 *
 * hal_rtc_datetime_t now;
 * hal_rtc_get_datetime(&now);
 * @endcode
 */

#ifndef HAL_RTC_H
#define HAL_RTC_H

/**
 * @defgroup HAL_RTC Rtc
 * @ingroup HAL_DRIVERS
 * @brief Real-time clock calendar and backup registers.
 * @{
 */

#include "common/hal_config.h"
#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_RTC

/** @brief Number of 32-bit backup registers available to ::hal_rtc_backup_write. */
#define HAL_RTC_BACKUP_COUNT 20

/** @brief Day of the week, as the calendar hardware numbers it. */
enum {
  HAL_RTC_MONDAY = 1,
  HAL_RTC_TUESDAY = 2,
  HAL_RTC_WEDNESDAY = 3,
  HAL_RTC_THURSDAY = 4,
  HAL_RTC_FRIDAY = 5,
  HAL_RTC_SATURDAY = 6,
  HAL_RTC_SUNDAY = 7,
};

/** @brief Which low-speed oscillator drives the calendar. */
typedef enum {
  HAL_RTC_CLOCK_AUTO = 0, /**< Prefer the crystal, fall back to the internal RC. */
  HAL_RTC_CLOCK_LSE = 1,  /**< External 32.768 kHz crystal; accurate. */
  HAL_RTC_CLOCK_LSI = 2,  /**< Internal RC; always available, ~±5%. */
  HAL_RTC_CLOCK_NONE = 3, /**< Not running (::hal_rtc_get_clock only). */
} hal_rtc_clock_t;

/** @brief RTC initialization configuration. Zero-initializes to AUTO. */
typedef struct {
  hal_rtc_clock_t clock; /**< Oscillator to drive the calendar. */
} hal_rtc_config_t;

/**
 * @brief Wall-clock date and time. All fields are plain binary, not BCD.
 */
typedef struct {
  uint16_t year;   /**< Full year, 2000..2099. */
  uint8_t month;   /**< 1 = January .. 12 = December. */
  uint8_t day;     /**< Day of month, 1..31. */
  uint8_t weekday; /**< 1 = Monday .. 7 = Sunday (::HAL_RTC_MONDAY etc). */
  uint8_t hour;    /**< 0..23 — the driver runs the calendar in 24-hour mode. */
  uint8_t minute;  /**< 0..59. */
  uint8_t second;  /**< 0..59. */
} hal_rtc_datetime_t;

/** @brief The two independent alarms the calendar provides. */
typedef enum {
  HAL_RTC_ALARM_A = 0,
  HAL_RTC_ALARM_B = 1,
} hal_rtc_alarm_t;

/** @name Calendar fields an alarm compares before firing
 *
 * Only the fields named here have to match, so the same alarm expresses
 * "every minute at second 30" (::HAL_RTC_MATCH_SECOND alone) or "09:30:00 on
 * the 1st" (all four). Naming no field at all fires once a second.
 *  @{ */
#define HAL_RTC_MATCH_SECOND (1U << 0)
#define HAL_RTC_MATCH_MINUTE (1U << 1)
#define HAL_RTC_MATCH_HOUR (1U << 2)
#define HAL_RTC_MATCH_DAY (1U << 3)
/** @} */

/** @brief When an alarm should fire. */
typedef struct {
  uint8_t day;    /**< Day of month 1..31, or weekday 1..7 — see @c day_is_weekday. */
  uint8_t hour;   /**< 0..23. */
  uint8_t minute; /**< 0..59. */
  uint8_t second; /**< 0..59. */
  uint8_t match;  /**< Fields that must match: @c HAL_RTC_MATCH_* , OR'd. */
  bool day_is_weekday; /**< Read @c day as a weekday rather than a date. */
} hal_rtc_alarm_config_t;

/**
 * @brief Called when an alarm or the wakeup timer fires.
 *
 * Runs in interrupt context, with the hardware flag already cleared.
 */
typedef void (*hal_rtc_callback_t)(void);

/**
 * @brief Start the calendar, or adopt one that is already running.
 *
 * A calendar left running by a previous boot is kept as it is — its time, its
 * oscillator, and its backup registers all survive. Only a cold RTC is
 * configured from scratch, so calling this at every boot is safe and is what
 * an application should do.
 *
 * Starting a crystal takes up to a second or two, and this call waits for it.
 * That happens once, on the boot that first configures the RTC.
 *
 * @param cfg Configuration; NULL selects ::HAL_RTC_CLOCK_AUTO.
 * @return ::HAL_OK, or ::HAL_ERR_TIMEOUT if the requested oscillator never
 *         started or the calendar would not enter initialization mode.
 */
hal_status_t hal_rtc_init(const hal_rtc_config_t *cfg);

/**
 * @brief Set the calendar.
 * @param dt Date and time; must not be NULL, and every field must be in range.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a NULL or out-of-range value,
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init, or
 *         ::HAL_ERR_TIMEOUT if the calendar would not enter initialization mode.
 */
hal_status_t hal_rtc_set_datetime(const hal_rtc_datetime_t *dt);

/**
 * @brief Read the calendar.
 * @param dt Destination; must not be NULL.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for NULL, or
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init.
 */
hal_status_t hal_rtc_get_datetime(hal_rtc_datetime_t *dt);

/**
 * @brief Whether the calendar holds a time somebody actually set.
 *
 * False after a cold start, true once ::hal_rtc_set_datetime has run — and
 * still true after a reset, because the RTC domain is not reset with the core.
 *
 * @return true if the calendar has been set.
 */
bool hal_rtc_is_set(void);

/**
 * @brief Which oscillator is driving the calendar.
 * @return ::HAL_RTC_CLOCK_LSE, ::HAL_RTC_CLOCK_LSI, or ::HAL_RTC_CLOCK_NONE
 *         when the RTC is not running.
 */
hal_rtc_clock_t hal_rtc_get_clock(void);

/**
 * @brief Arm an alarm on a calendar match.
 *
 * Re-arms itself: an alarm that names fewer fields than the full date repeats
 * on the next match, so "every minute at second 30" keeps firing without being
 * set again.
 *
 * @param alarm Which alarm to program.
 * @param cfg   When it should fire; must not be NULL.
 * @param cb    Called from interrupt context on each match, or NULL to leave
 *              the interrupt off and poll ::hal_rtc_alarm_fired instead.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a NULL or out-of-range value,
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init, or
 *         ::HAL_ERR_TIMEOUT if the alarm registers never became writable.
 */
hal_status_t hal_rtc_set_alarm(hal_rtc_alarm_t alarm,
                               const hal_rtc_alarm_config_t *cfg,
                               hal_rtc_callback_t cb);

/**
 * @brief Disarm an alarm and detach its callback.
 * @param alarm Which alarm to stop.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for an unknown alarm, or
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init.
 */
hal_status_t hal_rtc_cancel_alarm(hal_rtc_alarm_t alarm);

/**
 * @brief Whether an alarm has fired since this was last called.
 *
 * For polling without an interrupt; reading clears the flag. An alarm with a
 * callback clears its own flag, so this always reports false for one.
 *
 * @param alarm Which alarm to test.
 * @return true if it had fired.
 */
bool hal_rtc_alarm_fired(hal_rtc_alarm_t alarm);

/**
 * @brief Start the periodic wakeup timer.
 *
 * A countdown independent of the calendar, reloading itself every period. It
 * keeps running in the low-power modes that stop the CPU clock, which is what
 * makes it the usual way to wake a sleeping board on a schedule.
 *
 * Resolution follows the period: below ~30 seconds the timer counts the
 * oscillator directly, above that it counts whole seconds. The achievable
 * period is quantised to those ticks, so a request is rounded, and on a board
 * without a crystal the internal RC's tolerance applies on top.
 *
 * @param period_ms Period in milliseconds, 1..65536000 (about 18 hours).
 * @param cb        Called from interrupt context each period, or NULL to leave
 *                  the interrupt off.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a period out of range,
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init, or
 *         ::HAL_ERR_TIMEOUT if the timer registers never became writable.
 */
hal_status_t hal_rtc_set_wakeup(uint32_t period_ms, hal_rtc_callback_t cb);

/**
 * @brief Stop the wakeup timer and detach its callback.
 * @return ::HAL_OK, or ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init.
 */
hal_status_t hal_rtc_cancel_wakeup(void);

/**
 * @brief Whether the wakeup timer has fired since this was last called.
 *
 * For polling without an interrupt; reading clears the flag.
 * @return true if it had fired.
 */
bool hal_rtc_wakeup_fired(void);

/**
 * @brief Store a word in the backup domain, where it survives a reset.
 * @param index Register number, 0..::HAL_RTC_BACKUP_COUNT-1.
 * @param value Word to store.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for an out-of-range index, or
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init.
 */
hal_status_t hal_rtc_backup_write(uint8_t index, uint32_t value);

/**
 * @brief Read a word back from the backup domain.
 * @param index Register number, 0..::HAL_RTC_BACKUP_COUNT-1.
 * @param value Destination; must not be NULL.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a bad index or NULL pointer, or
 *         ::HAL_ERR_NOT_INITIALIZED before ::hal_rtc_init.
 */
hal_status_t hal_rtc_backup_read(uint8_t index, uint32_t *value);

#endif /* NAVHAL_CONFIG_DRV_RTC */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_RTC */
#endif /* HAL_RTC_H */
