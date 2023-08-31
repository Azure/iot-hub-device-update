#ifndef ROOTKEYPKGTESTUTILS_HPP
#define ROOTKEYPKGTESTUTILS_HPP

#include <parson.h>
#include <string>
#include <vector>

namespace aduc
{
namespace rootkeypkgtestutils
{

class TestRSAKeyPair
{
public:
    TestRSAKeyPair()
    {
    }

    ~TestRSAKeyPair()
    {
    }

    TestRSAKeyPair(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair& operator=(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair(TestRSAKeyPair&&) = delete;
    TestRSAKeyPair& operator=(TestRSAKeyPair&&) = delete;

    std::string GetPubKeyModulus() const;
    size_t GetPubKeyExponent() const;
    std::string getSha256HashOfPublicKey() const;

    std::string SignData(const std::string& data) const;
    bool VerifySignature(const std::string& signature) const;
};

std::string get_minimal_rootkey_package_json_path();
JSON_Value* get_minimal_rootkeypkg_json_str();
bool add_test_rootkeypkg_rootkey(JSON_Value* root_value, const std::string& kid, const std::string& rsa_n_modulus_base64url);
bool add_test_rootkeypkg_disabledRootKey(JSON_Value* root_value, const std::string& kid);
bool add_test_rootkeypkg_disabledSigningKey(JSON_Value* root_value, const std::string& sha256_hash);
bool add_test_rootkeypkg_signature(JSON_Value* root_value, const std::string& rs256_signature);

} // namespace rootkeypkgtestutils
} // namespace aduc

#endif // ROOTKEYPKGTESTUTILS_HPP
