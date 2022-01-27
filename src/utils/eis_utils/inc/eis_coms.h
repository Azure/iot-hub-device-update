/**
 * @file eis_coms.h
 * @brief Header file for HTTP communication with Edge Identity Service (EIS) over UDS
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <eis_err.h>
#include <time.h>
#include <umock_c/umock_c_prod.h>

#ifndef EIS_COMS_H
#    define EIS_COMS_H

EXTERN_C_BEGIN

/**
 * @brief Requests the identities from the EIS /identity/ URI
 * @details The identity response returns the hub hostname, device id, and key handle, Caller should de-allocate returned string using free()
 * @param timeoutMS max timeoutMS for the request in milliseconds
 * @returns A value of EISErr
 */
// clang-format off
// NOLINTNEXTLINE: clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(, EISErr, RequestIdentitiesFromEIS,
    unsigned int, timeoutMS,
    char**, responseBuffer)
// clang-format on

/**
 * @brief Requests the signed form of @p deviceUri and @p expiry using @p keyHandle with a request timeoutMS of @p timeoutMS
 * @details Caller should de-allocate @p responseBuffer using free()
 * @param keyHandle the handle for the key to be used for signing @p deviceUri and @p expiry
 * @param uri the uri to be used in the signature
 * @param expiry the expiration time in string format
 * @param timeoutMS the tiemout for the request in milliseconds
 * @returns A value of EISErr
 */
// clang-format off
// NOLINTNEXTLINE: clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(,EISErr,RequestSignatureFromEIS,
    const char*, keyHandle,
    const char*, uri,
    const char*, expiry,
    unsigned int, timeoutMS,
    char**, responseBuffer)
// clang-format on

/**
 * @brief Requests the signature related to @p certId from EIS
 * @details Caller should de-allocate @p responseBuffer using free()
 * @param[in] certId the identifier associated with the certificate being retrieved
 * @param[in] timeoutMS the timeout for the call
 * @param[out] responseBuffer ptr to the buffer which will hold the response from EIS
 * @returns a value of EISErr
 */
// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast): clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(,EISErr, RequestCertificateFromEIS,
    const char*, certId,
    unsigned int, timeoutMS,
    char**, responseBuffer)
// clang-format on

EXTERN_C_END

#endif
