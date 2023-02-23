/**
 * @file decryption_alg_types.h
 * @brief Header file that defines the supported decryption types for the decryption utils
 *
 * @copyright Copyright (c) Microsoft Corp.
 */

#include <aduc/c_utils.h>


#ifndef DECRYPTION_ALG_TYPES_H
#    define DECRYPTION_ALG_TYPES_H

EXTERN_C_BEGIN

typedef enum _DecryptionAlg{
    UNSUPPORTED_DECRYPTION_ALG = 0,
    AES_128_CBC = 1,
    AES_192_CBC = 2,
    AES_256_CBC = 3,
    //
    // New supported types can be added here
    //
} DecryptionAlg;

EXTERN_C_END

#endif
