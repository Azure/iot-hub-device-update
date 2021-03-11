/**
 * @file base64_utils.h
 * @brief Provides an implementation for Base64 encoding and decoding
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BASE64_UTILS_H
#    define BASE64_UTILS_H

EXTERN_C_BEGIN

//
// Base64 Encoding / Decoding
//

char* Base64URLEncode(const uint8_t* bytes, size_t len);

size_t Base64URLDecode(const char* base64_encoded_blob, uint8_t** decoded_buffer);

char* Base64URLDecodeToString(const char* base64_encoded_blob);

EXTERN_C_END

#endif // BASE64_UTILS_H
