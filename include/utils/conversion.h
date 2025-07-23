/**
 * @file conversion.h
 * @brief String to number conversion utilities interface
 *
 * This header declares functions for converting string representations
 * of numbers to their corresponding integer and floating-point values.
 * The implementations handle:
 * - Leading/trailing whitespace
 * - Optional sign indicators (+/-)
 * - Decimal points for floating-point numbers
 * - Partial number parsing (stops at first invalid character)
 *
 * @note These functions do not handle scientific notation or hexadecimal formats
 * @note No overflow/underflow checking is performed
 *
 * @ingroup UTILS
 * @author Ashutosh Vishwakarma
 * @date 2025-07-23
 */

 #ifndef CONVERSION_H
 #define CONVERSION_H
 
 #include <stdint.h>
 
 /**
  * @brief Convert a string to a 32-bit signed integer
  *
  * @param s Null-terminated string to convert (may contain leading whitespace)
  * @return Converted 32-bit integer value
  * 
  * @warning Potential integer overflow with large numbers
  * @warning Stops parsing at first non-digit character
  */
 int32_t str_to_int(const char *s);
 
 /**
  * @brief Convert a string to a floating-point number
  *
  * @param s Null-terminated string to convert (may contain leading whitespace)
  * @return Converted floating-point value
  *
  * @warning Limited precision for fractional components
  * @warning Stops parsing at first non-digit/non-dot character
  */
 float str_to_float(const char *s);
 
 #endif // CONVERSION_H