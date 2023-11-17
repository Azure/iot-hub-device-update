/**
 * @file rootkeypackage_utils_ut.cpp
 * @brief Unit Tests for rootkeypackage_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkeypkgtestutils.hpp"
#include <aduc/file_test_utils.hpp>
#include <aduc/hash_utils.h>
#include <aduc/rootkeypackage_types.h>
#include <aduc/rootkeypackage_utils.h>
#include <algorithm>
#include <base64_utils.h>
#include <catch2/catch.hpp>
#include <parson.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

using Catch::Matchers::Equals;

static std::string get_rootkey_package_template_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/rootkeypackage_template.json";
    return path;
}

static std::string get_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/rootkeypackage.json";
    return path;
}

static std::string get_serialized_protectedProperties(JSON_Value* rootkeyPkgJsonValue)
{
    JSON_Object* pkgJsonObj = json_object(rootkeyPkgJsonValue);
    REQUIRE(pkgJsonObj != nullptr);

    JSON_Object* protectedProperties = json_object_get_object(pkgJsonObj, "protected");
    REQUIRE(protectedProperties != nullptr);

    char* serializedProtectedProperties_cstr =
        json_serialize_to_string(json_object_get_wrapping_value(protectedProperties));
    REQUIRE(serializedProtectedProperties_cstr != nullptr);

    std::string protectedPropertiesSerialized = serializedProtectedProperties_cstr;

    json_free_serialized_string(serializedProtectedProperties_cstr);
    serializedProtectedProperties_cstr = nullptr;

    return protectedPropertiesSerialized;
}

static std::string fillout_protected_properties_template_params(
    const std::string& templateStr,
    const char* disabledHashPublicSigningKey,
    const char* modulus_1,
    size_t exponent_1,
    const char* modulus_2,
    size_t exponent_2,
    const char* modulus_3,
    size_t exponent_3)
{
    std::string str = aduc::FileTestUtils_applyTemplateParam(
        templateStr, "disabledHashPublicSigningKey", disabledHashPublicSigningKey);

    auto conv = [](size_t exp) {
        std::stringstream ss;
        ss << exp;
        return ss.str();
    };
    std::string exp1_str = conv(exponent_1);
    std::string exp2_str = conv(exponent_2);
    std::string exp3_str = conv(exponent_3);

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_1", modulus_1);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_1", exp1_str.c_str());

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_2", modulus_2);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_2", exp2_str.c_str());

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_3", modulus_3);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_3", exp3_str.c_str());

    return str;
}

static std::string convert_hexcolon_to_URLUIntBase64String(const std::string& hexcolon)
{
    std::stringstream ss{ hexcolon };
    std::vector<std::string> hexBytes;
    while (ss.good())
    {
        std::string tok;
        getline(ss, tok, ':');
        hexBytes.push_back(tok);
    }

    auto hex2nibble = [](char c) {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        throw std::invalid_argument("bad hex char");
    };

    std::vector<std::uint8_t> bytes;
    for (const std::string& hexByte : hexBytes)
    {
        uint8_t hi_nibble = (uint8_t)hex2nibble(hexByte[0]);
        uint8_t lo_nibble = (uint8_t)hex2nibble(hexByte[1]);
        uint8_t byte = (hi_nibble << 4) | lo_nibble;
        bytes.push_back(byte);
    }

    char* encoded = Base64URLEncode(bytes.data(), bytes.size());
    std::string base64url{ encoded };
    free(encoded);
    return base64url;
}
static std::string get_valid_rootkey_package(
    const char* disabledHashPublicSigningKey,
    const char* modulus_1,
    size_t exponent_1,
    const char* modulus_2,
    size_t exponent_2,
    const char* modulus_3,
    size_t exponent_3,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey1_privateKey,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey2_privateKey,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey3_privateKey)
{
    std::string json_template = aduc::FileTestUtils_slurpFile(get_rootkey_package_template_json_path().c_str());
    REQUIRE(json_template.length() > 0);

    // Fill out the "protected" properties template parameters, but do not fill out
    // the signatures "sig" properties yet.
    std::string templateStr = fillout_protected_properties_template_params(
        json_template,
        disabledHashPublicSigningKey,
        modulus_1,
        exponent_1,
        modulus_2,
        exponent_2,
        modulus_3,
        exponent_3);

    // Generate the signatures using the rootkey private keys passed in
    // and the protected properties data to be signed.
    JSON_Value* pkgJsonValue = json_parse_string(templateStr.c_str());
    REQUIRE(pkgJsonValue != nullptr);

    std::string protectedProperties = get_serialized_protectedProperties(pkgJsonValue);

    std::string rootkeysig_1 = rootkey1_privateKey.SignData(protectedProperties);
    std::string rootkeysig_2 = rootkey2_privateKey.SignData(protectedProperties);
    std::string rootkeysig_3 = rootkey3_privateKey.SignData(protectedProperties);

    templateStr = aduc::FileTestUtils_applyTemplateParam(templateStr, "rootkeysig_1", rootkeysig_1.c_str());
    templateStr = aduc::FileTestUtils_applyTemplateParam(templateStr, "rootkeysig_2", rootkeysig_2.c_str());
    templateStr = aduc::FileTestUtils_applyTemplateParam(templateStr, "rootkeysig_3", rootkeysig_3.c_str());

    json_value_free(pkgJsonValue);
    return templateStr;
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

    SECTION("valid template")
    {
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair1{ aduc::rootkeypkgtestutils::rootkeys::rootkey1 };
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair2{ aduc::rootkeypkgtestutils::rootkeys::rootkey2 };
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair3{ aduc::rootkeypkgtestutils::rootkeys::rootkey3 };

        const auto& rootkey1_publickey = rootKeyPair1.GetPublicKey();
        const auto& rootkey2_publickey = rootKeyPair2.GetPublicKey();
        const auto& rootkey3_publickey = rootKeyPair3.GetPublicKey();

        std::string urlIntBase64EncodedHash_of_rootkey3_public_key = rootkey3_publickey.getSha256HashOfPublicKey();
        const char* PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3 =
            urlIntBase64EncodedHash_of_rootkey3_public_key.c_str();

        std::string ROOTKEY_1_MODULUS = convert_hexcolon_to_URLUIntBase64String(rootkey1_publickey.GetModulus());
        size_t ROOTKEY_1_EXPONENT = rootkey1_publickey.GetExponent();

        std::string ROOTKEY_2_MODULUS = convert_hexcolon_to_URLUIntBase64String(rootkey2_publickey.GetModulus());
        size_t ROOTKEY_2_EXPONENT = rootkey2_publickey.GetExponent();

        std::string ROOTKEY_3_MODULUS = convert_hexcolon_to_URLUIntBase64String(rootkey3_publickey.GetModulus());
        size_t ROOTKEY_3_EXPONENT = rootkey3_publickey.GetExponent();

        const std::string rootKeyPkgJsonStr = get_valid_rootkey_package(
            PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3, // disabledHashPublicSigningKey
            ROOTKEY_1_MODULUS.c_str(), // modulus_1
            ROOTKEY_1_EXPONENT, // exponent_1
            ROOTKEY_2_MODULUS.c_str(), // modulus_2
            ROOTKEY_2_EXPONENT, // exponent_2
            ROOTKEY_3_MODULUS.c_str(), // modulus_3
            ROOTKEY_3_EXPONENT, // exponent_3
            rootKeyPair1.GetPrivateKey(), // privateKey_1
            rootKeyPair2.GetPrivateKey(), // privateKey_2
            rootKeyPair3.GetPrivateKey() // privateKey_3
        );

        ADUC_RootKeyPackage pkg{};
        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(rootKeyPkgJsonStr.c_str(), &pkg);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        //
        // Verify "protected" properties
        //

        // verify "version" and "published"
        CHECK(pkg.protectedProperties.isTest == true);
        CHECK(pkg.protectedProperties.version == 2);
        CHECK(pkg.protectedProperties.publishedTime == 1667343602);

        REQUIRE(VECTOR_size(pkg.protectedProperties.disabledRootKeys) == 2);

        // verify "disabledRootKeys"
        {
            // put the KIDs from disabledRootKeys into std::vector for making it more convenient to do assertions.
            const STRING_HANDLE* const disabledRootKey1 =
                static_cast<STRING_HANDLE*>(VECTOR_element(pkg.protectedProperties.disabledRootKeys, 0));
            const STRING_HANDLE* const disabledRootKey2 =
                static_cast<STRING_HANDLE*>(VECTOR_element(pkg.protectedProperties.disabledRootKeys, 1));

            CHECK_THAT(STRING_c_str(*disabledRootKey1), Equals("rootkey1"));
            CHECK_THAT(STRING_c_str(*disabledRootKey2), Equals("rootkey2"));
        }

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

            char* encodedHash = Base64URLEncode(hashBuffer->buffer, hashBuffer->size);
            std::string encodedHashStr{ encodedHash };
            encodedHashStr = std::regex_replace(encodedHashStr, std::regex("="), "");

            std::string expected{ PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3 };
            expected = std::regex_replace(expected, std::regex("="), "");

            CHECK(encodedHashStr == expected);

            CONSTBUFFER_DecRef(signingKeyHash.hash);
            free(encodedHash);
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

            char* actual_modulus = Base64URLEncode(actual_modulus_buf->buffer, actual_modulus_buf->size);
            REQUIRE(actual_modulus != nullptr);

            CHECK_THAT(actual_modulus, Equals(expected_modulus));
            free(actual_modulus);
        };

        VerifyRsaParams(rootkey1->rsaParameters, ROOTKEY_1_MODULUS, ROOTKEY_1_EXPONENT);
        VerifyRsaParams(rootkey2->rsaParameters, ROOTKEY_2_MODULUS, ROOTKEY_2_EXPONENT);
        VerifyRsaParams(rootkey3->rsaParameters, ROOTKEY_3_MODULUS, ROOTKEY_3_EXPONENT);

        //
        // Verify "protected" properties tring
        //
        JSON_Value* pkgJson = json_parse_string(rootKeyPkgJsonStr.c_str());

        REQUIRE(pkgJson != NULL);

        std::string protectedProperties = get_serialized_protectedProperties(pkgJson);

        CHECK_THAT(STRING_c_str(pkg.protectedPropertiesJsonString), Equals(protectedProperties.c_str()));

        json_value_free(pkgJson);

        //
        // Verify "signatures" properties
        //
        REQUIRE(pkg.signatures != nullptr);
        REQUIRE(VECTOR_size(pkg.signatures) == 3);

        auto verifyPkgSig = [&](size_t sig_index, const aduc::rootkeypkgtestutils::TestRSAKeyPair& testRootKeyPair) {
            void* el = VECTOR_element(pkg.signatures, sig_index);
            REQUIRE(el != nullptr);

            ADUC_RootKeyPackage_Signature* sig = static_cast<ADUC_RootKeyPackage_Signature*>(el);
            REQUIRE(sig != nullptr);

            CHECK(sig->alg == ADUC_RootKeySigningAlgorithm_RS256);

            const CONSTBUFFER* signatureBuffer = CONSTBUFFER_GetContent(sig->signature);
            REQUIRE(signatureBuffer != nullptr);

            char* encodedHash = Base64URLEncode(signatureBuffer->buffer, signatureBuffer->size);
            std::string urlDecodedStr{ encodedHash };
            urlDecodedStr = std::regex_replace(urlDecodedStr, std::regex("_"), "/");
            urlDecodedStr = std::regex_replace(urlDecodedStr, std::regex("-"), "+");
            CHECK(testRootKeyPair.GetPublicKey().VerifySignature(urlDecodedStr));

            free(encodedHash);
        };

        verifyPkgSig(0, rootKeyPair1);
        verifyPkgSig(1, rootKeyPair2);
        verifyPkgSig(2, rootKeyPair3);

        //
        // Cleanup
        //
        ADUC_RootKeyPackageUtils_Destroy(&pkg);
    }

    SECTION("valid example")
    {
        std::string rootkey_pkg_json = aduc::FileTestUtils_slurpFile(get_example_rootkey_package_json_path());

        ADUC_RootKeyPackage pkg{};
        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(rootkey_pkg_json.c_str(), &pkg);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        CHECK_FALSE(pkg.protectedProperties.isTest);
    }
}
