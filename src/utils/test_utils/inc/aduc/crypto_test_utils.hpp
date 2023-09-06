#include <openssl/evp.h>
#include <openssl/rsa.h>

namespace aduc
{
namespace cryptotestutils
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

private:
    int keyLen;
    EVP_PKEY_CTX* ctx = nullptr;
    EVP_PKEY* pkey = nullptr;
};

} // namespace cryptotestutils
} // namespace aduc
