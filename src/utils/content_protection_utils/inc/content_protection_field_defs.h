/**
 * @file content_protection_field_defs.h
 * @brief Header file that contains the field definitions for the content protection json
 *
 * @copyright Copyright (c) Microsoft Corp.
 */

#ifndef DECRYPTION_ALG_TYPES_H
#define DECRYPTION_ALG_TYPES_H

/**
 * Example JSON:
 * {
 *  ...
 *  "updateManifestSignature": ...,
 *
 *  "updateManifest": ...,
 *
 *  "dek": "YD_jyS73mEa1c—cVXN_rRe5QjpQakAuVNmAvvA2E-U",  // base64Url encoded sha256 hash of aes key
 *
 *  "dekHashAlg": "sha128",    // the hash algorithm used in "dek" property
 *
 *  "dekCryptAlg": "rsa1024",  // the asymmetric algorithm used with DEK and KEK
 *
 *  "decryptInfo": {
 *
 *      "alg": "aes",          // symmetric decryption algorithm
 *
 *      "mode": "cbc",         // not required if alg is a stream cipher
 *
 *      "keyLen": "128",       // in bits
 *
 *      "props": <object>      // optional
 *
 *  }
 *  "props": {
 *      ...
 *   }
 *  }
 */
/**
 * @brief Field name for the Base64 encoded, encrypted dek key
 * @details Field type string
*/
#define DEK_BYTES_FIELD "dek"
/**
 * @brief Field name for the DEK decrpytion algorithm
 * @details Field type string
 */
#define DEK_CRYPT_ALG_FIELD "dekCryptAlg"
/**
 * @brief Field name for the decryption information for the content coming down
 * @details Field type json object
 */
#define DECRYPT_INFO_FIELD "decryptInfo"
/**
 * @brief Field name for the Decryption Information's algorithm type
 * @details Field type string
 */
#define DECRYPT_INFO_ALG_FIELD "alg"
/**
 * @brief Field name for the Decryption Information's decryption mode
 * @details Field type string
 */
#define DECRYPT_INFO_MODE_FIELD "mode"
/**
 * @brief Field name for the Decryption Information's key length
 * @details Field type string
 */
#define DECRYPT_INFO_KEY_LENGTH_FIELD "keyLen"

/**
 * @brief Field name for the props field
 * @details Field type json object
 */
#define PROPS_FIELD "props"

#endif
