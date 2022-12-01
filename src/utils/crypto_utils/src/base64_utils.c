/**
 * @file base64_utils.c
 * @brief Implementation for Base64 encoding and decoding
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Note: on Base64 Encoding vs Base64URL
 * Base64 encodes byte values into specified values. A chart of these can be found in
 * RFC 4648. This chart happens to include symbols that are not URL safe (e.g. '+', '/', and '=').
 * Base64Url encoding is a reinterpratation of that table to make these URL safe. It transforms a
 * Base64 encoded string in the following ways
 * '+' is changed to '-'
 * '/' is changed to '_'
 *  and padding is removed from the output.
 *
 * More information can be found in RFC 4648 in the Base64Url section.
 */

/**
 * @brief Encodes the provided bytes into Base64URL
 * @details the string returned to the user should be freed using the free() function
 * @param bytes the buffer to be encoded
 * @param len the length of the buffer to be encoded
 * @returns NULL on failure or a pointer to a buffer of Base64URL encoded values on success.
 */
char* Base64URLEncode(const unsigned char* bytes, size_t len)
{
    char* output = NULL;

    STRING_HANDLE strHandle = Azure_Base64_Encode_Bytes(bytes, len);

    if (strHandle == NULL)
    {
        return NULL;
    }

    STRING_replace(strHandle, '+', '-');
    STRING_replace(strHandle, '/', '_');
    STRING_replace(strHandle, '=', '\0');

    size_t strLength = STRING_length(strHandle);

    output = (char*)malloc(strLength + 1);
    if (output == NULL)
    {
        goto done;
    }

    strcpy(output, STRING_c_str(strHandle)); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
    output[strLength] = '\0';

done:

    STRING_delete(strHandle);
    return output;
}

/**
 * @brief Decodes the provided blob into the provided byte buffer
 * @details the @p decoded_buffer should NOT be allocated before the decoding. The user is repsonsible for freeing the uint8_t buffer returned
 * @param base64_encoded_blob a string of base64URL encoded values
 * @param decoded_buffer the handle for the decoded data.
 * @returns the size of the @p decoded_buffer buffer on success, 0 on failure
 */
size_t Base64URLDecode(const char* base64_encoded_blob, unsigned char** decoded_buffer)
{
    size_t ret_size = 0;
    size_t buffLen = 0;
    uint8_t* tempDecodedBuffer = NULL;
    BUFFER_HANDLE buffHandle = NULL;
    char* temp_blob = NULL;

    *decoded_buffer = NULL;

    size_t blob_len = strlen(base64_encoded_blob);
    if (blob_len == 0)
    {
        goto done;
    }

    size_t padding = 0;
    if (blob_len % 4 != 0)
    {
        padding = 4 - blob_len % 4;
    }

    temp_blob = (char*)malloc(blob_len + padding + 1);
    if (temp_blob == NULL)
    {
        goto done;
    }

    unsigned int i = 0;
    for (i = 0; i < blob_len; ++i)
    {
        if (base64_encoded_blob[i] == '-')
        {
            temp_blob[i] = '+';
        }
        else if (base64_encoded_blob[i] == '_')
        {
            temp_blob[i] = '/';
        }
        else
        {
            temp_blob[i] = base64_encoded_blob[i];
        }
    }

    unsigned int padding_end = blob_len + padding;
    while (i < padding_end)
    {
        temp_blob[i] = '=';
        ++i;
    }
    temp_blob[blob_len + padding] = '\0';

    buffHandle = Azure_Base64_Decode(temp_blob);
    if (buffHandle == NULL)
    {
        goto done;
    }

    BUFFER_size(buffHandle, &buffLen);

    tempDecodedBuffer = (uint8_t*)malloc(buffLen);
    if (tempDecodedBuffer == NULL)
    {
        buffLen = 0;
        goto done;
    }
    memcpy(tempDecodedBuffer, BUFFER_u_char(buffHandle), buffLen);

done:

    BUFFER_delete(buffHandle);
    free(temp_blob);

    ret_size = buffLen;
    *decoded_buffer = tempDecodedBuffer;

    return ret_size;
}

/**
 * @brief Decodes the Base64Encoded buffers and returns the data in a null-terminated string.
 * @details This function should only be used when the caller expects the decoded data to be used in string format with
 * valid character values
 * @param base64_encoded_blob the base64 encoded data to be converted to a string
 * @returns a pointer to the newly null terminated decoded data on success, null on failure
 */
char* Base64URLDecodeToString(const char* base64_encoded_blob)
{
    bool success = false;

    char* blobStr = NULL;

    unsigned char* decodedBuffer = NULL;
    const size_t decodedSize = Base64URLDecode(base64_encoded_blob, &decodedBuffer);

    if (decodedSize == 0)
    {
        goto done;
    }

    blobStr = (char*)malloc(decodedSize + 1);

    if (blobStr == NULL)
    {
        goto done;
    }

    memcpy(blobStr, decodedBuffer, decodedSize);
    blobStr[decodedSize] = '\0';

    success = true;

done:

    if (!success)
    {
        free(blobStr);
        blobStr = NULL;
    }

    free(decodedBuffer);

    return blobStr;
}
