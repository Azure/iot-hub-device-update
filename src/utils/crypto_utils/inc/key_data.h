/**
 * @file key_data.h
 * @brief Declares the KeyData structure for the encryption/decryption functions
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <azure_c_shared_utility/constbuffer.h>

#ifndef KEY_DATA_H
#define KEY_DATA_H

/***
 * @brief Opaque type for callers to transfer KeyData, has the ability to add other keys and information depending on the input
*/
typedef struct KeyData
{
    CONSTBUFFER_HANDLE keyData; //!< actual bytes of the key to be loaded into the algorithm
} KeyData;

#endif // KEY_DATA_H

