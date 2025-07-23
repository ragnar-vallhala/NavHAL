#include "utils/conversion.h"

int32_t str_to_int(const char *s)
{
    int32_t result = 0;
    int sign = 1;

    // Skip whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // Handle sign
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    return sign * result;
}

float str_to_float(const char *s)
{
    float result = 0.0f;
    float fraction = 0.1f;
    int sign = 1;
    int seen_dot = 0;

    // Skip whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // Handle sign
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    while ((*s >= '0' && *s <= '9') || *s == '.')
    {
        if (*s == '.')
        {
            seen_dot = 1;
            s++;
            continue;
        }

        if (!seen_dot)
        {
            result = result * 10.0f + (*s - '0');
        }
        else
        {
            result = result + (*s - '0') * fraction;
            fraction *= 0.1f;
        }

        s++;
    }

    return sign * result;
}
