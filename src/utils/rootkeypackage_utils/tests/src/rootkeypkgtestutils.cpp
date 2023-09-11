#include "rootkeypkgtestutils.hpp"
#include <catch2/catch.hpp>
#include <parson.h>
#include <string>

namespace aduc
{
namespace rootkeypkgtestutils
{

const int KeyBitLen = 2048;

/**
 * @brief Gets the path to minimal rootkey package json with correct structure
 * @return std::string The path to the test json file.
 */
std::string get_minimal_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/rootkeypackage_minimal.json";
    return path;
}

/**
 * @brief Gets the serialized, non-formatted "protected" property value object.
 * @param rootkeyPkgJsonValue The parsed rootkey package JSON.
 * @return std::string The serialized JSON for protected property.
 */
std::string get_serialized_protectedProperties(JSON_Value* rootkeyPkgJsonValue)
{
    JSON_Object* pkgJsonObj = json_object(rootkeyPkgJsonValue);
    REQUIRE(pkgJsonObj != nullptr);

    JSON_Object* protectedProperties = json_object_get_object(pkgJsonObj, "protected");
    REQUIRE(protectedProperties != nullptr);

    auto serializedProtectedProperties_cstr = std::unique_ptr<char, void(*)(void*)>(
        json_serialize_to_string(json_object_get_wrapping_value(protectedProperties),
        json_value_free);
    REQUIRE(serializedProtectedProperties_cstr.get() != nullptr);

    return std::string{ serializedProtectedProperties_cstr.get() };
}

/**
 * @brief Gets a valid json string for a minimal rootkey package
 * with empty arrays for following properties:
 * disabledRootKeys, disabledSigningKeys, rootKeys, signatures
 * @return std::string The json string, or zero-length string on error.
 */
std::string get_minimal_rootkey_package_json_str()
{
    std::string json_str{};
    auto parsed = json_parse_file(aduc::rootkeypkgtestutils::get_minimal_rootkey_package_json_path().c_str());
    if (parsed != nullptr)
    {
        json_str = json_serialize_to_string(parsed);
    }

    return json_str;
}

/**
 * @brief Adds a root key to the test root key package
 * @param rootkey_id The rootkey identifier to use in the package.
 * @param disallowed Adds the @p rootkey_id to the disallowed rootkey list if true.
 */
void TestRootkeyPackage::AddRootkey(std::string rootkey_id, bool disallowed)
{
    aduc::testutils::KeyPair key_pair{ KeyBitLen };
    key_pair.Generate();
    root_keys.push_back(key_pair);
}

/**
 * @brief Adds a signing key to the test root key package
 * @param signingkey_id The signingkey identifier to use in the package.
 * @param allowed Adds the @p signingkey_id to the disallowed signing key list if false.
 */
void TestRootkeyPackage::AddCodeSigningKey(std::string signingkey_id, bool disallowed)
{
    aduc::testutils::KeyPair key_pair{ KeyBitLen };
    key_pair.Generate();
    signing_keys.push_back(key_pair);
}

/**
 * @brief Generate the signatures for each rootkey that's been added.
 * @details Must be called before @p GetProtectedPropertiesDataToSign if rootkey package having populated signatures array is desired.
 */
void TestRootkeyPackage::GenerateSignatures()
{
    std::string data_for_signing = GetProtectedPropertiesDataToSign();
    for(const auto& rootkey_pair : root_keys)
    {
        std::string signature = rootkey_pair.SignData(data_for_signing);
        signatures.push_back(signature);
    }
}

/**
 * @brief Get the entire rootkey package as serialized JSON string (non-formatted)
 * @return std::string The package as serialized JSON.
 */
std::string TestRootkeyPackage::GetSerialized() const
{
    json_serialize_to_string(const JSON_Value *value)
}

JSON_Value* GetPackageJson

/**
 * @brief Get the rootkey package "protected" property value as a serialized JSON string.
 * @return std::string The serialized JSON string in the rootkey package format.
 */
std::string TestRootkeyPackage::GetProtectedPropertiesDataToSign()
{
    // Start from template boiler-plate for convenience.
    std::string pkg_json_str{ aduc::testutils::FileTestUtils_slurpFile(aduc::rootkeypkgtestutils::get_minimal_rootkey_package_json_path()) };
    auto root_value = std::unique_ptr<JSON_Value, void(*)(JSON_Value*)>{ json_parse_string(pkg_json_str.c_str()), json_value_free };
    REQUIRE(root_value != nullptr);

    std::string data_for_signing = get_serialized_protectedProperties(root_value);
    json_array_append_value()

    return "";
}

/**
 * @brief Get the modulus for the rootkey package index.
 * @return std::string The modulus encoded as base64url.
 */
std::string TestRootkeyPackage::GetModulus(size_t idx) const
{
    return root_keys[idx].GetModulus();
}

/**
 * @brief Get the exponent for the rootkey package index.
 * @return int The exponent.
 */
int TestRootkeyPackage::GetExponent(size_t idx) const
{
    return root_keys[idx].GetExponent();
}

/**
 * @brief Get the i-th signature.
 * @return int The signature.
 */
std::string TestRootkeyPackage::GetSignature(size_t idx) const
{
    return signatures[idx];
}

/**
 * @brief Get the sha256 hash of the public key for the rootkey index.
 * @return std::string The hash of the public key.
 */
std::string TestRootkeyPackage::GetHashPublicKey(size_t idx) const
{
    // TODO
    return "";
}

} // namespace rootkeypkgtestutils
} // namespace aduc
