/**
 * @file jws_utils.h
 * @brief Provides helper methods for parsing and verifying JSON Web Keys and JSON Web Tokens
 * @details The user should note that the following library expects the caller to check the inputs for malicious intent (e.g. non-utf-8 characters) and insure that all strings are null-terminated.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

/**
 * Note: On Definitions and Acryonyms used in this library
 *
 * There are a few acronyms within this library that must be defined in order to prevent confusion. Some of
 * these come from RFCs, some of them were defined for this project specifically, and others are a combination
 * of both which were used in order to more clearly define the methods utilized here.
 *
 * JWS = JSON Web Signature. Defined in RFC 7515. A JSON Web Signature. Used in the context of this document it
 * is a BASE64Url encoded string (string being defined as a set of utf-8 characters) with three sections delimeted
 * by a '.'(period). It consists of a header, a payload, and a signature. The signature is a Base64Url encoded, encrypted
 * hash of the header and the payload. Within the context of this document a JWS is always used within this context. While our
 * definition of a JWS does follow the RFC we have an additional header "claim" or parameter, the SJWK which is defined below.
 *
 * JWK = JSON Web Key. Defined in RFC  7517. A JSON Web Key is a crpytographic key that follows the RFC 7517 standard. There are
 * specific cryptographic keys which are supported by the JSON Web Algorithms (JWA) standard in RFC 7518. There are specific claims
 * intended for specific applications within this standard. It is left as an excercise for the reader to find what is and is not supported.
 *
 * SJWK = Signed JSON Web Key. A JSON Web Signature with a JSON Web Key as a payload. There is no specific RFC for a SJWK although the
 * JSON Web Key RFC does indicate one can be transmitted as a JWS as we are defining here.
 *
 * JWT = JSON Web Token. Defined in RFC 7519. A JSON Web Token is a set of "claims" in JWS format. It is the larger construct which contains
 * the header, the payload, and the signature format described in the JWS standard. One can think of a JWT as the raw form of a JWS without the
 * signature. That is why we do not "validate" a JWT. We validate it's signature (the JWS), but we do extract a JWT's payload.
 */

#include "crypto_key.h"
#include <aduc/c_utils.h>
#include <stdbool.h>

#ifndef JWS_UTILS_H
#    define JWS_UTILS_H

EXTERN_C_BEGIN

/**
 * @brief Return Value for JWS Verification Calls
 */
typedef enum tagJWSResult
{
    JWSResult_Failed = 0, /**< Failed*/
    JWSResult_Success, /**< Succeeded */
    JWSResult_BadStructure, /**< JWS structure is not correct*/
    JWSResult_UnsupportedAlg, /**< Algorithm used to sign the JWS is not supported */
    JWSResult_InvalidSignature, /**<Signature of the JWS is invalid */
    JWSResult_InvalidKid /**< Key Identifier invalid */
} JWSResult;

JWSResult VerifySJWK(const char* sjwk);

JWSResult VerifyJWSWithKey(const char* blob, CryptoKeyHandle key);

JWSResult VerifyJWSWithSJWK(const char* jws);

bool GetPayloadFromJWT(const char* blob, char** destBuff);

void* GetKeyFromBase64EncodedJWK(const char* blob);

EXTERN_C_END

#endif // JWS_UTILS_H
