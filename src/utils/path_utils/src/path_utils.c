/**
 * @file path_utils.c
 * @brief Utilities for working with paths implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/path_utils.h"
#include <aduc/string_c_utils.h> // for IsNullOrEmpty
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <ctype.h> // for isalnum

/**
 * @brief Replaces non-alphunumeric chars with _ (underscore) char for use in path segments of a file path.
 * @param unsanitized The string to be sanitized
 * @return STRING_HANDLE The file path string, or NULL on error.
 * @details Caller owns it and must call STRING_delete() when done with it.
 */
STRING_HANDLE SanitizePathSegment(const char* unsanitized)
{
    char* sanitized = NULL;
    STRING_HANDLE sanitizedHandle = NULL;

    if (IsNullOrEmpty(unsanitized))
    {
        return NULL;
    }

    if (mallocAndStrcpy_s(&sanitized, unsanitized) != 0)
    {
        return NULL;
    }

    for (int i = 0; i < strlen(unsanitized); ++i)
    {
        char c = unsanitized[i];

        if (isalnum(c) || (c == '-'))
        {
            sanitized[i] = c;
        }
        else
        {
            sanitized[i] = '_';
        }
    }

    sanitizedHandle = STRING_new_with_memory(sanitized);
    if (sanitizedHandle == NULL)
    {
        goto done;
    }

    sanitized = NULL; // transfer ownership

done:
    free(sanitized);

    return sanitizedHandle;
}
