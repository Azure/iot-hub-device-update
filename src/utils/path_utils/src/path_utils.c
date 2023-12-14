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

#include <aducpal/limits.h> // for PATH_MAX

#define STR_PATH_MAX PATH_MAX - 1
/**
 * @brief Replaces non-alphunumeric chars with _ (underscore) char for use in path segments of a file path.
 * @param unsanitized The string to be sanitized
 * @return STRING_HANDLE The file path string, or NULL on error.
 * @details Caller owns it and must call STRING_delete() when done with it.
 */
STRING_HANDLE PathUtils_SanitizePathSegment(const char* unsanitized)
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

/**
 * @brief Concatenates a directory and folder to form a path.
 * @param dirPath The directory path (eg /var/lib/adu)
 * @param folderName The folder name (eg 12345678-1234-1234-1234-123456789012)
 * @return char* The concatenated path, or NULL on error.
*/
char* PathUtils_ConcatenateDirAndFolderPaths(const char* dirPath, const char* folderName)
{
    char* ret = NULL;
    char* tempRet = NULL;
    const char* file_delimeter = "/";
    size_t file_delimeter_len = 1; /* can use strlen since this is a known good string */

    if (IsNullOrEmpty(dirPath) || IsNullOrEmpty(folderName))
    {
        goto done;
    }

    size_t dirPathLen = ADUC_StrNLen(dirPath, STR_PATH_MAX);

    if (dirPathLen == 0 || dirPathLen == STR_PATH_MAX)
    {
        goto done;
    }

    if (dirPath[dirPathLen - 1] == '/')
    {
        file_delimeter_len = 0;
    }

    const size_t folderPathLen = ADUC_StrNLen(folderName, STR_PATH_MAX);

    if (folderPathLen == 0 || folderPathLen == STR_PATH_MAX)
    {
        goto done;
    }

    if (dirPathLen + folderPathLen + file_delimeter_len > STR_PATH_MAX)
    {
        goto done;
    }

    if (file_delimeter_len)
    {
        tempRet = ADUC_StringFormat("%s%s%s", dirPath, file_delimeter, folderName);
    }
    else
    {
        tempRet = ADUC_StringFormat("%s%s", dirPath, folderName);
    }

done:
    ret = tempRet;

    return ret;
}
