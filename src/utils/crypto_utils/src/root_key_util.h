/**
 * @file root_key_util.h
 * @brief Contains a list of Root Keys for ADU and functions for retrieving them
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "crypto_key.h"

#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

CryptoKeyHandle GetKeyForKid(const char* kid);

#endif // ROOT_KEY_UTIL_H
