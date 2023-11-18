#include "aduc/parse_num.h"

#include <errno.h> // errno
#include <stdlib.h> // strtol
#include <limits.h> // LONG_MIN, LONG_MAX

// clang-format off
#include <aduc/aduc_banned.h> // must be after other includes
// clang-format on

/**
 * @brief Parses an entire string as int32_t in base 10.
 *
 * @param str The string to parse.
 * @param out_parsed The parsed int32_t.
 * @return true on successful parse.
 */
bool ADUC_parse_int32_t(const char* str, int32_t* out_parsed)
{
    long val = 0;
    char* end = NULL;

    errno = 0; // set errno to 0 to distinguish between success/failure for value 0
    val = strtol(str, &end, 10 /* base */);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0) // error conditions
        || (end == str) // sets end to start of str on error
        || (val < INT32_MIN || val > INT32_MAX) // exceeded int32 bounds
        || (*end != '\0')) // did not parse all the chars (leftover on end that is not numeric)
    {
        return false;
    }

    *out_parsed = (int32_t)val;
    return true;
}
