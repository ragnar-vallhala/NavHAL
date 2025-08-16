/**
 * @file conversion.c
 * @brief String to number conversion implementation
 * @details
 * This file provides functions for converting string representations of numbers
 * to their corresponding integer and floating-point values. The implementation
 * handles:
 * - Leading/trailing whitespace
 * - Optional sign indicators (+/-)
 * - Decimal points for floating numbers
 * - Partial number parsing (stops at first invalid character)
 *
 * @note These are blocking implementations that parse until first invalid character
 * @note No overflow/underflow checking is performed
 * @note Does not support scientific notation or hexadecimal formats
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "utils/conversion.h"

/**
 * @brief Convert string to 32-bit signed integer
 * @param[in] s Pointer to null-terminated input string
 * @return Converted 32-bit integer value
 * @details
 * Parses the input string character by character to construct the integer value:
 * 1. Skips any leading whitespace (spaces/tabs)
 * 2. Processes optional sign character (+/-)
 * 3. Converts subsequent digit characters to integer value
 * 4. Stops at first non-digit character
 *
 * @warning Potential for overflow with large numbers
 * @warning No error reporting for invalid inputs
 * @code
 * int val = str_to_int(" -1234abc"); // Returns -1234
 * @endcode
 */
int32_t str_to_int(const char *s)
{
    int32_t result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // Handle optional sign
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++; // Explicit + sign, just skip
    }

    // Process digit sequence
    while (*s >= '0' && *s <= '9')
    {
        // Shift existing digits and add new digit
        result = result * 10 + (*s - '0');
        s++;
    }

    return sign * result;
}

/**
 * @brief Convert string to floating-point number
 * @param[in] s Pointer to null-terminated input string
 * @return Converted floating-point value
 * @details
 * Parses the input string to construct a float value:
 * 1. Skips leading whitespace
 * 2. Processes optional sign
 * 3. Handles both integer and fractional parts
 * 4. Properly tracks decimal point position
 * 5. Stops at first invalid character
 *
 * @warning Limited precision for fractional components
 * @warning No overflow/underflow protection
 * @code
 * float val = str_to_float(" 3.1415xyz"); // Returns 3.1415f
 * @endcode
 */
float str_to_float(const char *s)
{
    float result = 0.0f;
    float fraction = 0.1f;  // Tracks fractional place value
    int sign = 1;           // Positive by default
    int seen_dot = 0;       // Decimal point flag

    // Skip leading whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // Handle optional sign
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    // Process number components
    while ((*s >= '0' && *s <= '9') || *s == '.')
    {
        if (*s == '.')
        {
            if (seen_dot) break;  // Only allow one decimal point
            seen_dot = 1;
            s++;
            continue;
        }

        if (!seen_dot)
        {
            // Integer part
            result = result * 10.0f + (*s - '0');
        }
        else
        {
            // Fractional part
            result += (*s - '0') * fraction;
            fraction *= 0.1f;  // Move to next decimal place
        }
        s++;
    }

    return sign * result;
}