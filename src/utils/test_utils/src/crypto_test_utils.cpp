#include "aduc/crypto_test_utils.hpp"
#include <stdexcept>

namespace aduc
{
namespace testutils
{


KeyPair::KeyPair(int keyBitLen)
{
    keyLen = keyBitLen;
}

KeyPair::~KeyPair()
{
    if (pkey != nullptr)
    {
        EVP_PKEY_free(pkey);
        pkey = nullptr;
    }

    if (ctx != nullptr)
    {
        EVP_PKEY_CTX_free(ctx);
        ctx = nullptr;
    }
}

void KeyPair::Generate()
{
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (ctx == nullptr)
    {
        throw std::runtime_error("EVP_PKEY_CTX_new_id");
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        throw std::runtime_error("EVP_PKEY_keygen_init");
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
    {
        throw std::runtime_error("EVP_PKEY_CTX_set_rsa_keygen_bits");
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        throw std::runtime_error("EVP_PKEY_keygen");
    }
}

std::string KeyPair::GetHashOfPublicKey() const
{
    std::string hashPubKey;
    return hashPubKey;
}

std::string KeyPair::GetModulus() const
{
    std::string modulus;
    return modulus;
}

int KeyPair::GetExponent() const
{
    int exp = -1;
    return exp;
}

std::string KeyPair::SignData(const std::string& data) const
{
    std::string signature;
    return signature;
}

bool KeyPair::VerifySignature(const std::string& signature) const
{
    bool verified = false;
    return verified;
}

} // namespace testutils
} // namespace aduc
