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
 

#ifdef __cplusplus
extern "C" {
#endif
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
 

#ifdef __cplusplus
} /* extern "C" */
#endif
 #endif // CONVERSION_H