/**
 * @file decryption_utils.h
 * @brief Header file for the decryption utilities
 *
 * @copyright Copyright (c) Microsoft Corp.
 */

typedef void* DecryptionKey;

typedef void* DecryptionCtx;

bool CryptoUtils_InitialzieDecryptionCtx(void);

void CryptoUtils_DeInitializeDecryptionCtx();

/**
 * @brief Decrypts the chunk using the specified decryption key and context and loads the result into the outputBuff up to outputBuffSize
 *
 * @param decryptionKey the decryption key to use with the context for the chunks
 * @param ctx the CryptoUtils handle for the OpenSSL context used for decrypting the chunk
 * @param chunk the chunk to be decrypted
 * @param chunkSize the size of the chunk to be decrypted
 * @param outputBuff the output buffer to put the result in
 * @param outputBuffSize the output buffer size
 * @return size_t the size of the decrypted data if successful (up to @p outputBuffSize), 0 on failure or if the decrypted data is too large for the container
 */
size_t CryptoUtils_DecryptChunk(const DecryptKey* decryptionKey, const CryptoCtx* ctx, const char* alg, const unsigned char* chunk, const size_t chunkSize, char* outputBuff, const size_t outputBuffSize);
