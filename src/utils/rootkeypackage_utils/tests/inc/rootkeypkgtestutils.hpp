#ifndef ROOTKEYPKGTESTUTILS_HPP
#define ROOTKEYPKGTESTUTILS_HPP

#include <aduc/crypto_test_utils.hpp>
#include <parson.h>
#include <string>
#include <vector>

namespace aduc
{
namespace rootkeypkgtestutils
{

std::string get_minimal_rootkey_package_json_path();
std::string get_minimal_rootkey_package_json_str();
std::string get_serialized_protectedProperties(JSON_Value* rootkeyPkgJsonValue);

class TestRootkeyPackage
{
    std::vector<aduc::testutils::KeyPair> root_keys{};
    std::vector<aduc::testutils::KeyPair> signing_keys{};
    std::vector<std::string> signatures{};


public:
    size_t AddRootkey(std::string rootkey_id, bool allowed);
    size_t AddCodeSigningKey(std::string signingkey_id, bool allowed);
    size_t GenerateSignatures();
    std::string GetSerialized() const;
    std::string GetProtectedPropertiesDataToSign() const;
    std::string GetModulus(size_t idx) const;
    int GetExponent(size_t idx) const;
    std::string GetSignature(size_t idx) const;
    std::string GetHashPublicKey(size_t idx) const;
};

} // namespace rootkeypkgtestutils
} // namespace aduc

#endif // ROOTKEYPKGTESTUTILS_HPP
