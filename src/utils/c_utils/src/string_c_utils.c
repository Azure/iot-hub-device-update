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

#include <aducpal/unistd.h> // open, read, close

// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

/**
 * @brief Maximum length for the output string of ADUC_StringFormat()
 */
#define ADUC_STRING_FORMAT_MAX_LENGTH 512

/**
 * @brief Function that sets @p strBuffers to the contents of the file at @p filePath if the contents are smaller in size than the buffer
 * @param filePath path to the file who's contents will be read
 * @param strBuffer buffer which will be loaded with the contents of @p filePath
 * @param strBuffSize the size of the buffer
 * @returns false on failure, true on success
 */
bool LoadBufferWithFileContents(const char* filePath, char* strBuffer, const size_t strBuffSize)
{
    if (filePath == NULL || strBuffer == NULL || strBuffSize == 0)
    {
        return false;
    }

    bool success = false;
    FILE* fp = NULL;

    struct stat bS;
    if (stat(filePath, &bS) != 0)
    {
        goto done;
    }

    const long fileSize = bS.st_size;
    if (fileSize == 0 || fileSize > strBuffSize)
    {
        goto done;
    }

    fp = fopen(filePath, "rt");
    if (fp == NULL)
    {
        goto done;
    }

    char* readBuff = strBuffer;
    size_t buffSize = strBuffSize;

    while (true)
    {
        const char* line = fgets(readBuff, (int)buffSize, fp);
        if (line == NULL)
        {
            if (feof(fp))
            {
                break;
            }

            // Error
            goto done;
        }

        const size_t readSize = strlen(readBuff);
        readBuff += readSize;
        buffSize -= readSize;
    }

    *readBuff = '\0';

    success = true;

done:
    if (fp != NULL)
    {
        fclose(fp);
    }

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
bool atoul(const char* str, unsigned long* converted)
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
        res = (res * 10) + (unsigned long)(*str - '0');

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
bool atoui(const char* str, unsigned int* ui)
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
        res = (res * 10) + (unsigned int)(*str - '0');

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
 * @param[out] updateTypeName - Caller must call free(), can pass NULL if not desired.
 * @param[out] updateTypeVersion
 */
bool ADUC_ParseUpdateType(const char* updateType, char** updateTypeName, unsigned int* updateTypeVersion)
{
    bool succeeded = false;
    char* name = NULL;

    if (updateTypeName != NULL)
    {
        *updateTypeName = NULL;
    }

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

    const size_t nameLength = (size_t)(delimiter - updateType);

    //name is empty
    if (nameLength == 0)
    {
        goto done;
    }

    if (updateTypeName != NULL)
    {
        name = malloc(nameLength + 1);
        if (name == NULL)
        {
            goto done;
        }

        memcpy(name, updateType, nameLength);
        name[nameLength] = '\0';
    }

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
        if (updateTypeName != NULL)
        {
            *updateTypeName = name;
        }
    }
    else
    {
        if (name != NULL)
        {
            free(name);
        }
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
bool IsNullOrEmpty(const char* str)
{
    return str == NULL || *str == '\0';
}

/**
 * @brief Allocates memory for @p target and copy @p len characters of @p source into @p target buffer.
 *
 * @param[out] target Output string
 * @param len Length of string to copy.
 * @param source Source string
 * @return bool Returns true is success.
 */
bool MallocAndSubstr(char** target, const char* source, size_t len)
{
    if (target == NULL || source == NULL)
    {
        return false;
    }
    *target = NULL;

    const size_t LenPlusNullTerm = len + 1;

    char* t = calloc(LenPlusNullTerm, sizeof(*t));
    if (t == NULL)
    {
        return false;
    }

    if (!ADUC_Safe_StrCopyN(t, LenPlusNullTerm, source, len /* srcByteLen */))
    {
        free(t);
        return false;
    }

    *target = t;
    return true;
}

/**
 * @brief Safely copies a source string to a destination buffer.
 *
 * This function is a safer alternative to strncpy. It ensures that the
 * destination buffer is always null-terminated and won't cause buffer
 * overflow if src is large enough to cause truncation.
 *
 * @param dest The destination buffer.
 * @param destByteLen The size in bytes of the destination buffer capacity that also accounts for a null-terminator. e.g. pass ARRAY_SIZE(dest), or 42, for char dest[42].
 * @param src The source string to be copied.
 * @param srcByteLen The number of source bytes to copy.
 * @return true on success.
 */
bool ADUC_Safe_StrCopyN(char* dest, size_t destByteLen, const char* src, size_t srcByteLen)
{
    if (dest == NULL || src == NULL || destByteLen == 0)
    {
        return false;
    }
    *dest = '\0';

    if (srcByteLen >= destByteLen)
    {
        return false;
    }

    if (srcByteLen > ADUC_StrNLen(src, srcByteLen))
    {
        return false;
    }

    memcpy(dest, src, srcByteLen);
    dest[srcByteLen] = '\0';

    return true;
}

/**
* @brief Allocate a new str that is a copy of the src string up to the given byte length.
* @param dest The addr of ptr that will point to newly allocated string copy.
* @param src The source string.
* @param srcByteLen The byte length of source string (not including null terminator).
* @remark The caller must take into account character encoding, e.g. utf-8 characters can have up to 4 bytes per character for some unicode codepoints.
* @return true on success
*/
bool ADUC_AllocAndStrCopyN(char** dest, const char* src, size_t srcByteLen)
{
    bool res = false;
    char* tmpDest = NULL;
    const size_t SrcByteLenNullTerm = srcByteLen + 1;

    if (dest == NULL || src == NULL || srcByteLen == 0 || SrcByteLenNullTerm == 0)
    {
        return false;
    }

    tmpDest = calloc(SrcByteLenNullTerm, sizeof(char));
    if (tmpDest == NULL)
    {
        goto done;
    }

    if (!ADUC_Safe_StrCopyN(
            tmpDest, SrcByteLenNullTerm * sizeof(char) /* destByteLen */, src, srcByteLen /* srcByteLen */))
    {
        goto done;
    }

    *dest = tmpDest;
    tmpDest = NULL;
    res = true;
done:
    free(tmpDest);

    return res;
}
