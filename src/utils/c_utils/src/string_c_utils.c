/**
 * @file string_c_utils.c
 * @brief Implementation of string utilities for C code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/string_c_utils.h"
#include "aduc/c_utils.h"

#include <azure_c_shared_utility/crt_abstractions.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief Maximum length for the output string of ADUC_StringFormat()
 */
#define ADUC_STRING_FORMAT_MAX_LENGTH 512

/**
 * @brief Read a value from a delimited file and return the value found.
 * The file is in form "key=value", and keys are case sensitive.
 * Value returned has whitespace trimmed from both ends.
 *
 * @param fileName Filename of delimited file
 * @param key Key to find
 * @param value Value found for @p Key
 * @param valueLen Size of buffer for @p value
 * @return true if value found, false otherwise.
 */
_Bool ReadDelimitedValueFromFile(const char* fileName, const char* key, char* value, unsigned int valueLen)
{
    _Bool foundKey = false;
    const unsigned int bufferLen = 1024;
    char buffer[bufferLen];

    if (valueLen < 2)
    {
        // Need space for at least a character and a null terminator.
        return false;
    }

    FILE* fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        return false;
    }

    while (!foundKey && fgets(buffer, bufferLen, fp) != NULL)
    {
        char* delimiter = strchr(buffer, '=');
        if (delimiter == NULL)
        {
            // Ignore lines without delimiters.
            continue;
        }

        // Change the delimiter character to a NULL for ease of parsing.
        *delimiter = '\0';

        ADUC_StringUtils_Trim(buffer);
        foundKey = (strcmp(buffer, key) == 0);
        if (!foundKey)
        {
            continue;
        }

        char* foundValue = delimiter + 1;
        ADUC_StringUtils_Trim(foundValue);
        strncpy(value, foundValue, valueLen);
        if (value[valueLen - 1] != '\0')
        {
            // strncpy pads the buffer with NULL up to valueLen, so if
            // that position doesn't have a NULL, the buffer provided was too small.
            foundKey = false;
            break;
        }
    }

    fclose(fp);

    return foundKey;
}

/**
 * @brief Function that sets @p strBuffers to the contents of the file at @p filePath if the contents are smaller in size than the buffer
 * @param filePath path to the file who's contents will be read
 * @param strBuffer buffer which will be loaded with the contents of @p filePath
 * @param strBuffSize the size of the buffer
 * @returns false on failure, true on success
 */
_Bool LoadBufferWithFileContents(const char* filePath, char* strBuffer, const size_t strBuffSize)
{
    if (filePath == NULL || strBuffer == NULL || strBuffSize == 0)
    {
        return false;
    }

    _Bool success = false;

    // NOLINTNEXTLINE(android-cloexec-open): We are not guaranteed to have access to O_CLOEXEC on all of our builds so no need to include
    int fd = open(filePath, O_EXCL | O_RDONLY);

    if (fd == -1)
    {
        goto done;
    }

    struct stat bS;

    if (stat(filePath, &bS) != 0)
    {
        goto done;
    }

    long fileSize = bS.st_size;

    if (fileSize == 0 || fileSize > strBuffSize)
    {
        goto done;
    }

    size_t numRead = read(fd, strBuffer, fileSize);

    if (numRead != fileSize)
    {
        goto done;
    }

    strBuffer[numRead] = '\0';

    success = true;
done:

    close(fd);

    if (!success)
    {
        strBuffer[0] = '\0';
    }

    return success;
}
/**
 * @brief Trim leading and trailing white-spaces. This function modifies the input buffer.
 *
 * @param str Input string to trim.
 * @return Input pointer.
 */
char* ADUC_StringUtils_Trim(char* str)
{
    char* begin = str;
    char* current = str;

    if (IsNullOrEmpty(str))
    {
        return str;
    }

    // Find first non white-spaces.
    while (isspace((unsigned char)*current))
    {
        current++;
    }

    char* end = current;

    // Find the end of the string
    while (*end != '\0')
    {
        end++;
    }

    // back up to the last non-null character
    end--;

    // Find the last non-space character
    while (isspace((unsigned char)*end))
    {
        end--;
    }

    // Shift non-white-space character(s), if needed.
    while (current != end + 1)
    {
        *str++ = *current++;
    }

    *str = '\0';

    return begin;
}

/**
 * @brief Converts string to unsigned long
 * Returns false if an input string cannot be converted to unsigned long
 * @param[in] string that needs to be converted
 * @param[out] converted unsigned long
 */
_Bool atoul(const char* str, unsigned long* converted)
{
    if (IsNullOrEmpty(str))
    {
        return false;
    }

    unsigned long res = 0;
    while (*str != '\0')
    {
        if (*str < '0' || *str > '9')
        {
            // Not a digit.
            return false;
        }

        const unsigned long previous = res;
        res = (res * 10) + (*str - '0');

        if (res < previous)
        {
            // overflow.
            return false;
        }

        ++str;
    }

    *converted = res;
    return true;
}

/**
 * @brief Converts string to unsigned integer.
 * @details Valid range 0 - 4294967295.
 * Returns false if string contains invalid char or out of range
 * @param[in] string that needs to be converted
 * @param[out] converted integer
 */
_Bool atoui(const char* str, unsigned int* ui)
{
    if (IsNullOrEmpty(str))
    {
        return false;
    }

    unsigned int res = 0;
    while (*str != '\0')
    {
        if (*str < '0' || *str > '9')
        {
            // Not a digit.
            return false;
        }

        const unsigned int previous = res;
        res = (res * 10) + (*str - '0');

        if (res < previous)
        {
            // overflow.
            return false;
        }

        ++str;
    }

    *ui = res;
    return true;
}

/**
 * @brief  Finds the length in bytes of the given string, not including the final null character. Only the first maxsize characters are inspected: if the null character is not found, maxsize is returned.
 * @param str  string whose length is to be computed
 * @param maxsize the limit up to which to check @p str's length
 * @return Length of the string "str", exclusive of the final null byte, or maxsize if the null character is not found, 0 if str is NULL.
 */
size_t ADUC_StrNLen(const char* str, size_t maxsize)
{
    if (str == NULL)
    {
        return 0;
    }

    size_t length;

    /* Note that we do not check if s == NULL, because we do not
    * return errno_t...
    */
    for (length = 0; length < maxsize && *str; length++, str++)
    {
    }

    return length;
}
/**
 * @brief Split updateType string by ':' to return updateTypeName and updateTypeVersion
 * @param[in] updateType - expected "Provider/Name:Version"
 * @param[out] updateTypeName - Caller must call free()
 * @param[out] updateTypeVersion
 */
_Bool ADUC_ParseUpdateType(const char* updateType, char** updateTypeName, unsigned int* updateTypeVersion)
{
    _Bool succeeded = false;
    char* name = NULL;
    *updateTypeName = NULL;
    *updateTypeVersion = 0;

    if (updateType == NULL)
    {
        goto done;
    }

    const char* delimiter = strchr(updateType, ':');

    //delimiter doesn't exist
    if (delimiter == NULL)
    {
        goto done;
    }

    const size_t nameLength = delimiter - updateType;

    //name is empty
    if (nameLength == 0)
    {
        goto done;
    }

    name = malloc(nameLength + 1);
    if (name == NULL)
    {
        goto done;
    }

    memcpy(name, updateType, nameLength);
    name[nameLength] = '\0';

    // convert version string to unsigned int
    if (!atoui(delimiter + 1, updateTypeVersion))
    {
        // conversion failed
        goto done;
    }

    succeeded = true;

done:
    if (succeeded)
    {
        *updateTypeName = name;
    }
    else
    {
        free(name);
    }

    return succeeded;
}

/**
 * @brief Returns string created by formatting a variable number of string arguments with @p fmt
 * @details Caller must free returned string, any formatted string above 512 characters in length will result in a failed call
 * @param fmt format string to be used on parameters
 * @returns in case of string > 512 characters or other failure NULL, otherwise a pointer to a null-terminated string
 */
char* ADUC_StringFormat(const char* fmt, ...)
{
    if (fmt == NULL)
    {
        return NULL;
    }

    char buffer[ADUC_STRING_FORMAT_MAX_LENGTH];

    va_list args;

    va_start(args, fmt);

    const int result = vsnprintf(buffer, ADUC_STRING_FORMAT_MAX_LENGTH, fmt, args);

    va_end(args);

    if (result <= 0 || result >= ADUC_STRING_FORMAT_MAX_LENGTH)
    {
        return NULL;
    }

    char* outputStr = NULL;

    if (mallocAndStrcpy_s(&outputStr, buffer) != 0)
    {
        return NULL;
    }

    return outputStr;
}

/**
 * @brief Check whether @p str is NULL or empty.
 *
 * @param str A string to check.
 * @return Returns true if @p str is NULL or empty.
 */
_Bool IsNullOrEmpty(const char* str)
{
    return str == NULL || *str == '\0';
}

/**
 * @brief Allocates memory for @p target and copy @p len characters of @p source into @p target buffer.
 *
 * @param[out] target Output string
 * @param source Source string
 * @param len Length of string to copy.
 * @return _Bool Returns true is success.
 */
_Bool MallocAndSubstr(char** target, char* source, size_t len)
{
    if (target == NULL || source == NULL)
    {
        return false;
    }
    *target = NULL;

    char* t = malloc((len + 1) * sizeof(*t));
    if (t == NULL)
    {
        return false;
    }
    memset(t, 0, (len + 1) * sizeof(*t));
    if (strncpy(t, source, len) != t)
    {
        free(t);
        return false;
    }

    *target = t;
    return true;
}
