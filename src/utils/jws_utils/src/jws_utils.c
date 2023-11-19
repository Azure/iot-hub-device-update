/**
 * @file jws_utils.c
 * @brief Provides methods for verifying Signed JSON Web Keys, JSON Web Tokens in JSON Web Signature Format, and JSON Web Signatures.
 * @brief Provides verification methods and helper functions
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "jws_utils.h"
#include "aduc/string_c_utils.h" // ADUC_Safe_StrCopyN
#include "base64_utils.h"
#include "crypto_lib.h"
#include "root_key_util.h"
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <parson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aduc/string_c_utils.h" // ADUC_Safe_StrCopyN

// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

//
// Internal Functions
//

// Find the position of the period and return the next character

/**
 * @brief Returns the string value of the fieldName iwthin the jsonString
 * @details Caller is expected to free the returned string using free()
 * @param jsonString a string containing a JSON object
 * @param fieldName the name of the field within jsonString to get the value from
 * @returns Returns NULL if the value doesn't exist, otherwise returns the string
 */
static char* GetStringValueFromJSON(const char* jsonString, const char* fieldName)
{
    char* returnStr = NULL;

    JSON_Value* root_value = json_parse_string(jsonString);

    if (json_value_get_type(root_value) != JSONObject)
    {
        goto done;
    }

    JSON_Object* jsonObj = json_value_get_object(root_value);
    const char* fieldValue = json_object_get_string(jsonObj, fieldName);

    if (fieldValue == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&returnStr, fieldValue) != 0)
    {
        returnStr = NULL;
        goto done;
    }

done:

    json_value_free(root_value);
    return returnStr;
}

/**
 * @brief Finds the character in the string, assigns a pointer to that lcoation, and returns the size of the substring from the beginning of @p string to @p foundCharPos
 * @param string string to be searched
 * @param charToFind the character to find within @p string
 * @param foundCharPos pointer will be set to the pointer to the first instance of @p charToFind within @p string
 * @returns the size of the substring from the beginning of string to the first instance of foundCharPos
 */
static size_t FindCharInString(const char* string, const char charToFind, char** foundCharPos)
{
    if (foundCharPos == NULL)
    {
        return 0;
    }

    char* found = strchr(string, charToFind);

    if (found == NULL)
    {
        *foundCharPos = NULL;
        return 0;
    }

    *foundCharPos = found;
    return (size_t)(found - string);
}

/**
 * @brief Extracts the header, payload, and signature from the Base64Url encoded JSON Web Signature within @p jws
 * @param jws a Base64URL encoded JSON Web Signature containing a header, payload, and signature delimited by '.'
 * @param header buffer to store the Base64URL encoded header in, allocated by function to be freed by Caller using free()
 * @param payload buffer to store the Base64URL encoded payload in,  allocated by function to be freed by Caller using free()
 * @param signature buffer to store the Base64URL encoded signature in,  allocated by function to be freed by Caller using free()
 * @returns on successful extraction of all sections True is returned otherwise False
 */
static bool ExtractJWSSections(const char* jws, char** header, char** payload, char** signature)
{
    bool success = false;

    *header = NULL;
    *payload = NULL;
    *signature = NULL;

    if (jws == NULL)
    {
        goto done;
    }

    const size_t jwsLen = strlen(jws);
    if (jwsLen == 0)
    {
        goto done;
    }

    char* headerEnd = NULL;
    size_t headerLen = FindCharInString(jws, '.', &headerEnd);

    if (headerLen == 0 || headerLen + 1 >= jwsLen)
    {
        goto done;
    }

    char* payloadEnd = NULL;
    size_t payloadLen = FindCharInString((headerEnd + 1), '.', &payloadEnd);

    if (payloadLen == 0 || (headerLen + payloadLen + 2 >= jwsLen))
    {
        goto done;
    }

    // From payloadEnd to the end of jws is the signature

    size_t sigLen = jwsLen - payloadLen - headerLen - 2; // 2 is for the periods

    *header = (char*)malloc(headerLen + 1);
    *payload = (char*)malloc(payloadLen + 1);
    *signature = (char*)malloc(sigLen + 1);

    if (*header == NULL || *payload == NULL || *signature == NULL)
    {
        goto done;
    }

    ADUC_Safe_StrCopyN(*header, jws, headerLen + 1, headerLen);
    ADUC_Safe_StrCopyN(*payload, (headerEnd + 1), payloadLen + 1, payloadLen);
    ADUC_Safe_StrCopyN(*signature, (payloadEnd + 1), sigLen + 1, sigLen);

    success = true;
done:
    if (!success)
    {
        if (*header != NULL)
        {
            free(*header);
            *header = NULL;
        }

        if (*payload != NULL)
        {
            free(*payload);
            *payload = NULL;
        }

        if (*signature != NULL)
        {
            free(*signature);
            *signature = NULL;
        }
    }

    return success;
}

/**
 * @brief Extracts the header from the Base64URL encoded JSON Web Signature within @p jws
 * @details Copies the header from @p jws to @p header the header will be allocated by this method and should be freed by the caller using free
 * @param jws a Base4Url encoded JSON Web Signature contianing at least a header delimited by '.'
 * @param header destination buffer to store the Base64Url encoded header in
 * @returns on successful extraction of the header True, false otherwise
 *
 */
static bool ExtractJWSHeader(const char* jws, char** header)
{
    bool success = false;
    char* tempHeader = NULL;

    *header = NULL;

    if (jws == NULL)
    {
        goto done;
    }

    const size_t jwsLen = strlen(jws);
    if (jwsLen == 0)
    {
        goto done;
    }

    char* headerEnd = NULL;

    size_t headerLen = FindCharInString(jws, '.', &headerEnd);

    if (headerLen == 0 || headerLen + 1 >= jwsLen)
    {
        goto done;
    }

    tempHeader = (char*)malloc(headerLen + 1);

    if (tempHeader == NULL)
    {
        goto done;
    }

    ADUC_Safe_StrCopyN(tempHeader, jws, headerLen + 1, headerLen);

    success = true;

done:
    if (!success)
    {
        free(tempHeader);
        tempHeader = NULL;
    }

    *header = tempHeader;
    return success;
}

//
// Public Functions
//

/**
 * @brief converts JWSResult to const C string.
 *
 * @param r The jws result to convert.
 * @return const char* The mapped-to string.
 * @details NOTE: Needs to be kept in sync with JWSResult enum in jws_utils.h
 */
const char* jws_result_to_str(JWSResult r)
{
    switch (r)
    {
    case JWSResult_Failed:
        return "Failed";

    case JWSResult_Success:
        return "Success";

    case JWSResult_BadStructure:
        return "BadStructure";

    case JWSResult_InvalidSignature:
        return "InvalidSignature";

    case JWSResult_DisallowedRootKid:
        return "DisallowedRootKid";

    case JWSResult_MissingRootKid:
        return "MissingRootKid";

    case JWSResult_InvalidRootKid:
        return "InvalidRootKid";

    case JWSResult_InvalidEncodingJWSHeader:
        return "JWSResult_InvalidEncodingJWSHeader";

    case JWSResult_InvalidSJWKPayload:
        return "JWSResult_InvalidSJWKPayload";

    case JWSResult_DisallowedSigningKey:
        return "DisallowedSigningKey";

    case JWSResult_FailedGetDisabledSigningKeys:
        return "JWSResult_FailedGetDisabledSigningKeys";

    case JWSResult_FailGenPubKey:
        return "JWSResult_FailGenPubKey";

    case JWSResult_HashPubKeyFailed:
        return "JWSResult_HashPubKeyFailed";

    default:
        return "???";
    }
}

/**
 * @brief Verifies the Base64URL encoded @p sjwk in Signed JSON Web Key (SJWK) format using the KiD found within the encoded JWKs Header
 * @details A Signed JSON Web Key (SJWK) is JWK in JSON Web Signature (JWS) format. The function parses the header for the kid, builds the associated key, and then verifies the signature of the JWK
 * @param sjwk a base64URL encoded string that contains the Signed JSON Web Key
 * @returns a value of JWSResult
 */
JWSResult VerifySJWK(const char* sjwk)
{
    JWSResult retval = JWSResult_Failed;
    JWSResult jwsResultIsSigningKeyDisallowed = JWSResult_Failed;
    JWSResult jwsResultVerifyJwtSignature = JWSResult_Failed;
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    ADUC_Result resultGetDisabledSigningKeys = { ADUC_GeneralResult_Failure, 0 };
    VECTOR_HANDLE signingKeyDisallowed = NULL;

    char* header = NULL;
    char* payload = NULL;
    char* signature = NULL;

    char* jsonHeader = NULL;
    char* jsonPayload = NULL;
    char* kid = NULL;
    void* rootKey = NULL;

    if (!ExtractJWSSections(sjwk, &header, &payload, &signature))
    {
        retval = JWSResult_BadStructure;
        goto done;
    }

    jsonHeader = Base64URLDecodeToString(header);

    if (jsonHeader == NULL)
    {
        retval = JWSResult_Failed;
        goto done;
    }

    kid = GetStringValueFromJSON(jsonHeader, "kid");

    if (kid == NULL)
    {
        retval = JWSResult_Failed;
        goto done;
    }

    result = RootKeyUtility_GetKeyForKid(&rootKey, kid);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        if (result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED)
        {
            retval = JWSResult_DisallowedRootKid;
        }
        else if (result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID)
        {
            retval = JWSResult_MissingRootKid;
        }
        else
        {
            retval = JWSResult_InvalidRootKid;
        }
        goto done;
    }

    // First verify JWT structure and signature
    jwsResultVerifyJwtSignature = VerifyJWSWithKey(sjwk, rootKey);
    if (jwsResultVerifyJwtSignature != JWSResult_Success)
    {
        retval = jwsResultVerifyJwtSignature;
        goto done;
    }

    // Now, verify that signing key is not on the rootkey packages's Disallowed
    resultGetDisabledSigningKeys = RootKeyUtility_GetDisabledSigningKeys(&signingKeyDisallowed);
    if (IsAducResultCodeFailure(resultGetDisabledSigningKeys.ResultCode))
    {
        retval = JWSResult_FailedGetDisabledSigningKeys;
        goto done;
    }

    jsonPayload = Base64URLDecodeToString(payload);

    if (jsonPayload == NULL)
    {
        retval = JWSResult_Failed;
        goto done;
    }

    jwsResultIsSigningKeyDisallowed = IsSigningKeyDisallowed(jsonPayload, signingKeyDisallowed);
    if (jwsResultIsSigningKeyDisallowed != JWSResult_Success)
    {
        retval = jwsResultIsSigningKeyDisallowed;
        goto done;
    }

    retval = JWSResult_Success;

done:
    if (signingKeyDisallowed != NULL)
    {
        VECTOR_destroy(signingKeyDisallowed);
    }

    if (header != NULL)
    {
        free(header);
    }

    if (payload != NULL)
    {
        free(payload);
    }

    if (signature != NULL)
    {
        free(signature);
    }

    if (kid != NULL)
    {
        free(kid);
    }

    if (jsonHeader != NULL)
    {
        free(jsonHeader);
    }

    if (jsonPayload != NULL)
    {
        free(jsonPayload);
    }

    if (rootKey != NULL)
    {
        CryptoUtils_FreeCryptoKeyHandle(rootKey);
    }

    return retval;
}

/**
 * @brief Verifies the BASE64URL encoded @p blob JSON Web Signature (JWS) using the key held within the Signed JSON Web Key header parameter
 * @details Verifies the Signed JSON Web Key (SJWK) and uses the key from the SJWK to validate the JSON Web Signature @p blob
 * @param jws a Base64URL encoded JSON Web Token in JSON Web Signature format with a Signed JSON Web Key within the header
 * @returns a value of JWSResult
 */
JWSResult VerifyJWSWithSJWK(const char* jws)
{
    JWSResult result = JWSResult_Failed;

    char* header = NULL;
    char* jsonHeader = NULL;
    char* sjwk = NULL;
    CryptoKeyHandle key = NULL;

    if (!ExtractJWSHeader(jws, &header))
    {
        result = JWSResult_BadStructure;
        goto done;
    }

    jsonHeader = Base64URLDecodeToString(header);

    if (jsonHeader == NULL)
    {
        result = JWSResult_InvalidEncodingJWSHeader;
        goto done;
    }

    sjwk = GetStringValueFromJSON(jsonHeader, "sjwk");

    if (sjwk == NULL || *sjwk == '\0')
    {
        result = JWSResult_BadStructure;
        goto done;
    }

    result = VerifySJWK(sjwk);
    if (result != JWSResult_Success)
    {
        goto done;
    }

    key = GetKeyFromBase64EncodedJWK(sjwk);
    if (key == NULL)
    {
        result = JWSResult_BadStructure;
        goto done;
    }

    result = VerifyJWSWithKey(jws, key);

    if (result != JWSResult_Success)
    {
        goto done;
    }

done:
    if (header != NULL)
    {
        free(header);
    }

    if (jsonHeader != NULL)
    {
        free(jsonHeader);
    }

    if (sjwk != NULL)
    {
        free(sjwk);
    }

    if (key != NULL)
    {
        CryptoUtils_FreeCryptoKeyHandle(key);
    }
    return result;
}

/**
 * @brief Whether signing key in SJWK (header of JWS) is no longer trusted as per latest root key package on file.
 *
 * @param sjwkJsonStr The SJWK json string that is base64 URL decoded header section of the JWS.
 * @param disabledHashOfPubKeysList The list of sha256 hashes of public keys of signing keys that are disabled.
 * @return JWSResult Returns JWSResult_Success if not on Disallowed; JWSResult_DisallowedSigningKey if signing key was on Disallowed, or other failure JWSResult if failed whilst determining.
 */
JWSResult IsSigningKeyDisallowed(const char* sjwkJsonStr, VECTOR_HANDLE disabledHashOfPubKeysList)
{
    JWSResult result = JWSResult_Failed;
    CONSTBUFFER_HANDLE pubkey = NULL;
    CONSTBUFFER_HANDLE sha256HashPubKey = NULL;

    char* N = GetStringValueFromJSON(sjwkJsonStr, "n");
    char* e = GetStringValueFromJSON(sjwkJsonStr, "e");
    if (IsNullOrEmpty(N) || IsNullOrEmpty(e)
        || strcmp(e, "AQAB") != 0) // AQAB is 65337 or 0x010001, the ubiquitous RSA exponent.
    {
        result = JWSResult_InvalidSJWKPayload;
        goto done;
    }

    pubkey = CryptoUtils_GenerateRsaPublicKey(N, e);
    if (pubkey == NULL)
    {
        result = JWSResult_FailGenPubKey;
        goto done;
    }

    sha256HashPubKey = CryptoUtils_CreateSha256Hash(pubkey);
    if (pubkey == NULL)
    {
        result = JWSResult_HashPubKeyFailed;
        goto done;
    }

#ifdef TRACE_DISABLED_SIGNING_KEY
    char* base64urlSha256HashPubKey = Base64URLEncode(
        CONSTBUFFER_GetContent(sha256HashPubKey)->buffer, CONSTBUFFER_GetContent(sha256HashPubKey)->size);
    printf("base64url encoding of sha256 hash of public key: %s\n", base64urlSha256HashPubKey);
#endif

    // See if the hash of public key is on the Disallowed List.
    for (size_t i = 0; i < VECTOR_size(disabledHashOfPubKeysList); ++i)
    {
        const ADUC_RootKeyPackage_Hash* DisallowedEntry =
            (ADUC_RootKeyPackage_Hash*)VECTOR_element(disabledHashOfPubKeysList, i);

        if ((DisallowedEntry->alg == SHA256)
            && CONSTBUFFER_HANDLE_contain_same(sha256HashPubKey, DisallowedEntry->hash))
        {
            result = JWSResult_DisallowedSigningKey;
            goto done;
        }
    }

    result = JWSResult_Success;

done:

    if (pubkey != NULL)
    {
        CONSTBUFFER_DecRef(pubkey);
    }

    if (sha256HashPubKey != NULL)
    {
        CONSTBUFFER_DecRef(sha256HashPubKey);
    }

    return result;
}

/**
 * @brief Verifies the Base64URL encoded @p blob JSON Web Signature (JWS) using @p key
 * @details Function expects a Base64URL encoded @p blob in JSON Web Signature format and verifies its signature using @p key
 * @param blob a Base64URL encoded JSON Web Token in JSON Web Signature format
 * @param key the public key created with the crypto_lib functions that corresponds to the one used to sign the @p blob
 * @returns a value of JWSResult
 */
JWSResult VerifyJWSWithKey(const char* blob, CryptoKeyHandle key)
{
    JWSResult result = JWSResult_Failed;

    // Check for structure
    char* header = NULL;
    char* payload = NULL;
    char* signature = NULL;

    char* headerJson = NULL;
    char* alg = NULL;
    char* headerPlusPayload = NULL;
    uint8_t* decodedSignature = NULL;

    if (!ExtractJWSSections(blob, &header, &payload, &signature))
    {
        result = JWSResult_BadStructure;
        goto done;
    }

    headerJson = Base64URLDecodeToString(header);

    if (headerJson == NULL)
    {
        result = JWSResult_Failed;
        goto done;
    }

    alg = GetStringValueFromJSON(headerJson, "alg");

    if (alg == NULL)
    {
        result = JWSResult_BadStructure;
        goto done;
    }

    // Note: The +2 is for the "." between the header and the payload
    // and the null terminator at the end of the string.
    size_t headerLen = strlen(header);
    size_t payloadLen = strlen(payload);
    size_t hppSize = headerLen + payloadLen + 2;
    headerPlusPayload = (char*)calloc(1, hppSize);

    if (headerPlusPayload == NULL)
    {
        result = JWSResult_Failed;
        goto done;
    }

    memcpy(headerPlusPayload, header, headerLen);
    headerPlusPayload[headerLen] = '.';
    memcpy(headerPlusPayload + headerLen + 1, payload, payloadLen);
    headerPlusPayload[headerLen + payloadLen + 1] = '\0';

    size_t decodedSignatureLen = Base64URLDecode(signature, &decodedSignature);

    if (!CryptoUtils_IsValidSignature(
            alg, decodedSignature, decodedSignatureLen, (uint8_t*)headerPlusPayload, strlen(headerPlusPayload), key))
    {
        result = JWSResult_InvalidSignature;
    }
    else
    {
        result = JWSResult_Success;
    }

done:

    if (header != NULL)
    {
        free(header);
    }

    if (payload != NULL)
    {
        free(payload);
    }

    if (signature != NULL)
    {
        free(signature);
    }

    if (headerJson != NULL)
    {
        free(headerJson);
    }

    if (alg != NULL)
    {
        free(alg);
    }

    if (headerPlusPayload != NULL)
    {
        free(headerPlusPayload);
    }

    if (decodedSignature != NULL)
    {
        free(decodedSignature);
    }
    return result;
}

/**
 * @brief Pulls the payload out of the JWT and converts it from base64 to a string DOES NOT VALIDATE IT
 * @param blob a Base64URL encoded JSON Web Token (JWT) in JSON Web Signature format
 * @param destBuff the destination buffer for the JWT's payload, allocated within the function
 * @returns a value of JWSResult
 */
bool GetPayloadFromJWT(const char* blob, char** destBuff)
{
    bool result = false;

    *destBuff = NULL;

    char* header = NULL;
    char* payload = NULL;
    char* signature = NULL;
    char* tempStr = NULL;

    if (!ExtractJWSSections(blob, &header, &payload, &signature))
    {
        goto done;
    }

    tempStr = Base64URLDecodeToString(payload);

    if (tempStr == NULL)
    {
        goto done;
    }

    result = true;

done:

    free(header);
    free(payload);
    free(signature);

    *destBuff = tempStr;
    return result;
}

/**
 * @brief parses the key from the JWK into a usable CryptoLib key
 * @details Only supports RSA keys right now, more support will be added later. DOES VALIDATE THE JWK
 * @param blob a Base64 encoded JSON Web Key which contains the parameters for creating a key
 * @returns a pointer to the key on success, NULL on failure
 */
void* GetKeyFromBase64EncodedJWK(const char* blob)
{
    CryptoKeyHandle key = NULL;

    char* header = NULL;
    char* payload = NULL;
    char* signature = NULL;

    char* strN = NULL;
    char* stre = NULL;
    char* payloadJson = NULL;

    if (!ExtractJWSSections(blob, &header, &payload, &signature))
    {
        goto done;
    }

    payloadJson = Base64URLDecodeToString(payload);

    if (payloadJson == NULL)
    {
        goto done;
    }

    strN = GetStringValueFromJSON(payloadJson, "n");
    stre = GetStringValueFromJSON(payloadJson, "e");

    if (strN == NULL || stre == NULL)
    {
        goto done;
    }

    key = RSAKey_ObjFromB64Strings(strN, stre);

done:

    if (header != NULL)
    {
        free(header);
    }

    if (payload != NULL)
    {
        free(payload);
    }

    if (signature != NULL)
    {
        free(signature);
    }

    if (payloadJson != NULL)
    {
        free(payloadJson);
    }

    if (strN != NULL)
    {
        free(strN);
    }

    if (stre != NULL)
    {
        free(stre);
    }

    return key;
}
