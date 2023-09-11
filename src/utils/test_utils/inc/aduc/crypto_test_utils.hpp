/**
 * @file crypto_test_utils.hpp
 * @brief Function prototypes for file test utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef CRYPTO_TEST_UTILS_HPP
#define CRYPTO_TEST_UTILS_HPP

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <string>

namespace aduc
{
namespace testutils
{

class KeyPair
{
public:
    explicit KeyPair(int keyBitLen);

    KeyPair(const KeyPair&) = delete;
    KeyPair& operator=(const KeyPair&) = delete;
    KeyPair(KeyPair&&) = delete;
    KeyPair& operator=(KeyPair&&) = delete;

    ~KeyPair();
    void Generate();

    std::string GetHashOfPublicKey() const;
    std::string GetModulus() const;
    int GetExponent() const;
    std::string SignData(const std::string& data) const;
    bool VerifySignature(const std::string& signature) const;

private:
    int keyLen;
    EVP_PKEY_CTX* ctx = nullptr;
    EVP_PKEY* pkey = nullptr;
};

} // namespace testutils
} // namespace aduc

#endif // CRYPTO_TEST_UTILS_HPP
