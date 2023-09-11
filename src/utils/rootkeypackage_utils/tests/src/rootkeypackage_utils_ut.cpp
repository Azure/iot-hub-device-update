/**
 * @file rootkeypackage_utils_ut.cpp
 * @brief Unit Tests for rootkeypackage_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkeypkgtestutils.hpp"
#include <aduc/crypto_test_utils.hpp>
#include <aduc/file_test_utils.hpp>
#include <aduc/vector_test_utils.hpp>
#include <aduc/hash_utils.h>
#include <aduc/result.h>
#include <aduc/rootkeypackage_types.h>
#include <aduc/rootkeypackage_utils.h>
#include <algorithm>
#include <base64_utils.h>
#include <catch2/catch.hpp>
#include <memory>
#include <parson.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

const int KeyPairBitLength = 2048;

using Catch::Matchers::Equals;

template<typename SrcElementType, typename TgtElementType>
using to_vector = aduc::testutils::Convert_VECTOR_HANDLE_to_std_vector<SrcElementType, TgtElementType>;

static std::string get_prod_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/rootkeypackage.json";
    return path;
}

TEST_CASE("RootKeyPackageUtils_Parse")
{
    SECTION("bad json")
    {
        ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };

        ADUC_RootKeyPackage pkg{};

        result = ADUC_RootKeyPackageUtils_Parse(nullptr, &pkg);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG);

        result = ADUC_RootKeyPackageUtils_Parse("", &pkg);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG);

        result = ADUC_RootKeyPackageUtils_Parse("{[}", &pkg);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON);
    }

    SECTION("bad parse - no rootkeys")
    {
        ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
        ADUC_RootKeyPackage pkg{};
        std::string json_str = aduc::rootkeypkgtestutils::get_minimal_rootkey_package_json_str();
        result = ADUC_RootKeyPackageUtils_Parse(json_str.c_str(), &pkg);
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_ROOTKEYS_EMPTY);
    }

    SECTION("valid - one disallowed rootkey, no disallowed signing keys")
    {
        aduc::rootkeypkgtestutils::TestRootkeyPackage test_rootkey_pkg;
        REQUIRE(0 == test_rootkey_pkg.AddRootkey("rootkey1_allowed", false /* disallowed */));
        REQUIRE(1 == test_rootkey_pkg.AddRootkey("rootkey2_allowed", false /* disallowed */));
        REQUIRE(2 == test_rootkey_pkg.AddRootkey("rootkey3_disallowed", true /* disallowed */));
        REQUIRE(3 == test_rootkey_pkg.GenerateSignatures());

        ADUC_RootKeyPackage pkg{};
        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(test_rootkey_pkg.GetSerialized().c_str(), &pkg);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        //
        // Verify "protected" properties
        //

        // verify "version" and "published"
        CHECK(pkg.protectedProperties.version == 2);
        CHECK(pkg.protectedProperties.publishedTime == 1667343602);

        CHECK(pkg.protectedProperties.disabledRootKeys != nullptr);

        // Verify "disabledRootKeys"
        std::vector<std::string> disallowed_root_keys_vec = to_vector<STRING_HANDLE, std::string>(
            pkg.protectedProperties.disabledRootKeys,
            [](const STRING_HANDLE* const& str_handle) -> std::string {
                return std::string{ STRING_c_str(str_handle) };
            }
        );

        REQUIRE(disallowed_root_keys_vec.size() == 1);

        const std::string& kidDisallowedRootKey = disabled_root_keys_vec[0];
        CHECK_THAT(kidDisallowedRootKey, Equals("rootkey3_disallowed"));

        // verify "disabledSigningKeys"
        REQUIRE(VECTOR_size(pkg.protectedProperties.disabledSigningKeys) == 1);
        {
            void* el = VECTOR_element(pkg.protectedProperties.disabledSigningKeys, 0);
            REQUIRE(el != nullptr);

            auto a = static_cast<ADUC_RootKeyPackage_Hash*>(el);
            ADUC_RootKeyPackage_Hash signingKeyHash{ *a };
            CHECK(signingKeyHash.alg == SHA256);

            const CONSTBUFFER* hashBuffer = CONSTBUFFER_GetContent(signingKeyHash.hash);
            REQUIRE(hashBuffer != nullptr);

            std::unique_ptr<char, void(*)(void*)> encodedHash{ Base64URLEncode(hashBuffer->buffer, hashBuffer->size), ::free };
            std::string actualHashPubKey{ encodedHash.get() };
            actualHashPubKey = std::regex_replace(actualHashPubKey, std::regex("="), "");

            std::string expected = test_rootkey_pkg.GetHashPublicKey(2);

            CHECK(actualHashPubKey == expected);

            CONSTBUFFER_DecRef(signingKeyHash.hash);
        }

        // verify "rootKeys"
        REQUIRE(VECTOR_size(pkg.protectedProperties.rootKeys) == 3);

        const ADUC_RootKey* const rootkey1 =
            static_cast<ADUC_RootKey*>(VECTOR_element(pkg.protectedProperties.rootKeys, 0));

        const ADUC_RootKey* const rootkey2 =
            static_cast<ADUC_RootKey*>(VECTOR_element(pkg.protectedProperties.rootKeys, 1));

        const ADUC_RootKey* const rootkey3 =
            static_cast<ADUC_RootKey*>(VECTOR_element(pkg.protectedProperties.rootKeys, 2));

        REQUIRE(rootkey1 != nullptr);
        REQUIRE(rootkey2 != nullptr);
        REQUIRE(rootkey3 != nullptr);

        CHECK_THAT(STRING_c_str(rootkey1->kid), Equals("rootkey1"));
        CHECK_THAT(STRING_c_str(rootkey2->kid), Equals("rootkey2"));
        CHECK_THAT(STRING_c_str(rootkey3->kid), Equals("rootkey3"));

        CHECK(rootkey1->keyType == ADUC_RootKey_KeyType_RSA);
        CHECK(rootkey2->keyType == ADUC_RootKey_KeyType_RSA);
        CHECK(rootkey3->keyType == ADUC_RootKey_KeyType_RSA);

        auto VerifyRsaParams = [](const ADUC_RSA_RootKeyParameters& rsaParams,
                                  const std::string& expected_modulus,
                                  size_t expected_exponent) {
            CHECK(rsaParams.e == expected_exponent);

            const CONSTBUFFER* actual_modulus_buf = CONSTBUFFER_GetContent(rsaParams.n);
            REQUIRE(actual_modulus_buf != nullptr);

            std::unique_ptr<char, void(*)(void*)> actual_modulus{ Base64URLEncode(actual_modulus_buf->buffer, actual_modulus_buf->size), ::free };
            REQUIRE(actual_modulus != nullptr);
            CHECK_THAT(actual_modulus, Equals(expected_modulus));
        };

        VerifyRsaParams(rootkey1->rsaParameters, test_rootkey_pkg.GetModulus(0), test_rootkey_pkg.GetExponent(0));
        VerifyRsaParams(rootkey2->rsaParameters, test_rootkey_pkg.GetModulus(1), test_rootkey_pkg.GetExponent(1));
        VerifyRsaParams(rootkey3->rsaParameters, test_rootkey_pkg.GetModulus(2), test_rootkey_pkg.GetExponent(2));

        //
        // Verify "protected" properties string
        //
        {
            std::unique_ptr<JSON_Value, void(*)(JSON_Value*)> pkgJson{ json_parse_string(rootKeyPkgJsonStr.c_str()), ::json_value_free };
            REQUIRE(pkgJson.get() != NULL);
            std::string protectedProperties = aduc::rootkeypkgtestutils::get_serialized_protectedProperties(pkgJson.get());
            CHECK_THAT(STRING_c_str(pkg.protectedPropertiesJsonString),Equals(protectedProperties.c_str()));
        }

        //
        // Verify "signatures" properties
        //
        REQUIRE(pkg.signatures != nullptr);
        REQUIRE(VECTOR_size(pkg.signatures) == 3);

        for (size_t i = 0; i < VECTOR_size(pkg.signatures); ++i)
        {
            void* el = VECTOR_element(pkg.signatures, i);
            REQUIRE(el != nullptr);

            ADUC_RootKeyPackage_Signature* sig = static_cast<ADUC_RootKeyPackage_Signature*>(el);
            REQUIRE(sig != nullptr);

            // Check algorithm
            CHECK(sig->alg == ADUC_RootKeySigningAlgorithm_RS256);

            // Check parsed signature from package matches original
            const CONSTBUFFER* signatureBuffer = CONSTBUFFER_GetContent(sig->signature);
            REQUIRE(signatureBuffer != nullptr);

            std::unique_ptr<char, void(*)(void*)> encodedHash{Base64URLEncode(signatureBuffer->buffer, signatureBuffer->size), ::free};
            std::string actual_signature{ encodedHash.get() };
            actual_signature = std::regex_replace(actual_signature, std::regex("_"), "/");
            actual_signature = std::regex_replace(actual_signature, std::regex("-"), "+");

            std::string expected_signature = test_rootkey_pkg.GetSignature(i);
            REQUIRE(expected_signature.length() > 0);
            CHECK_THAT(actual_signature, Equals(expected_signature));
        };

        //
        // Cleanup
        //
        ADUC_RootKeyPackageUtils_Destroy(&pkg);
    }

    SECTION("valid prod pkg")
    {
        std::string rootkey_pkg_json = aduc::testutils::FileTestUtils_slurpFile(get_prod_rootkey_package_json_path());

        ADUC_RootKeyPackage pkg{};
        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(rootkey_pkg_json.c_str(), &pkg);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
    }

    SECTION("single disabled rootkey")
    {
        // std::string path = get_minimal_rootkey_package_json_path();
        // JSON_Value* root_val = json_parse_file(path.c_str());
        // REQUIRE(root_val != nullptr);
        //
        // aduc::rootkeypkgtestutils::TestRootkeyPackage test_pkg{ key_pair };
        // REQUIRE(add_test_rootkeypkg_rootkey("bad_kid", bad_kid_modulus));
        // REQUIRE(add_test_rootkeypkg_rootkey("good_kid", good_kid_modulus));
        //
        // REQUIRE(add_test_rootkeypkg_disabledRootKey(root_val, "bad_kid"));
        //
        // ADUC_RootKeyPackage pkg{};
        // ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(test_pkg.GetProtectedPropertiesDataToSign().c_str(), &pkg);
        //
        // CHECK(IsAducResultCodeFailure(result.ResultCode));
        // CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_ROOTKEYS_EMPTY);
    }

    SECTION("single disabled signing key")
    {
        // ADUC_RootKeyPackage pkg{};
        // ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(aduc::rootkeypkgtestutils::get_minimal_rootkey_package_json_str().c_str(), &pkg);
        // CHECK(IsAducResultCodeFailure(result.ResultCode));
        // CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_ROOTKEYS_EMPTY);
    }

    SECTION("single disabled rootkey and disabled signing key")
    {
        // ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
        // ADUC_RootKeyPackage pkg{};
        // std::string json_str = aduc::rootkeypkgtestutils::get_minimal_rootkey_package_json_str();
        // result = ADUC_RootKeyPackageUtils_Parse(json_str.c_str(), &pkg);
        // CHECK(IsAducResultCodeFailure(result.ResultCode));
        // CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_ROOTKEYS_EMPTY);
    }
}
