#include "aduc/crypto_test_utils.hpp"
#include <stdexcept>

namespace aduc
{
namespace cryptotestutils
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

} // namespace cryptotestutils
} // namespace aduc
