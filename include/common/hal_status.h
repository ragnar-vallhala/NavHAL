/**
 * @file hal_status.h
 * @brief NavHAL standard status / error code type.
 *
 * @details
 * Defines `hal_status_t`, the single return type used by every fallible
 * NavHAL function. `HAL_OK` (0) means success; any other value is an error.
 * Functions that produce data return `hal_status_t` and write their output
 * through caller-supplied pointer parameters.
 *
 * This is the standardized replacement for the pre-standardization
 * `SUCCESS` / `FAILURE` macros, which remain as deprecated aliases (Section 6
 * of `docs/api_standardization.md`).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_STATUS_H
#define HAL_STATUS_H


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Standard return type for all fallible NavHAL functions.
 *
 * `HAL_OK` is guaranteed to be 0 and `HAL_ERR` to be 1; the remaining codes
 * are unspecified positive values. Test success with `== HAL_OK`.
 */
typedef enum {
  HAL_OK = 0,              /**< Operation completed successfully. */
  HAL_ERR = 1,             /**< Unspecified failure. */
  HAL_ERR_INVALID_ARG,     /**< Invalid parameter or out-of-range instance id. */
  HAL_ERR_TIMEOUT,         /**< Operation timed out. */
  HAL_ERR_BUSY,            /**< Peripheral busy or bus arbitration lost. */
  HAL_ERR_NOT_INITIALIZED, /**< Used before the peripheral's hal_<p>_init(). */
  HAL_ERR_NOT_SUPPORTED,   /**< Capability not available on this target. */
  HAL_ERR_IO,              /**< Bus, framing, or NAK error. */
  HAL_ERR_NO_MEM,          /**< Buffer too small or no resource available. */
} hal_status_t;

/**
 * @brief Evaluate @p expr; if its `hal_status_t` result is not `HAL_OK`,
 *        return that status from the enclosing function.
 *
 * @code
 * hal_status_t configure(void) {
 *   HAL_OK_OR_RETURN(hal_uart_init(HAL_UART_2, &cfg));
 *   HAL_OK_OR_RETURN(hal_gpio_init(&gpio_cfg));
 *   return HAL_OK;
 * }
 * @endcode
 */
#define HAL_OK_OR_RETURN(expr)                                                 \
  do {                                                                         \
    hal_status_t _navhal_status = (expr);                                      \
    if (_navhal_status != HAL_OK)                                              \
      return _navhal_status;                                                   \
  } while (0)

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization status macros.
 *
 * Kept so existing drivers keep building; both map onto the standardized enum
 * with identical values (SUCCESS == HAL_OK == 0, FAILURE == HAL_ERR == 1).
 * Retained as a backward-compat alias. New code MUST use
 * hal_status_t / HAL_OK / HAL_ERR.
 * -------------------------------------------------------------------------- */
#define SUCCESS HAL_OK  /**< @deprecated Use HAL_OK. */
#define FAILURE HAL_ERR /**< @deprecated Use HAL_ERR. */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_STATUS_H */
