/**
 * @file rootkeypackage_utils_ut.cpp
 * @brief Unit Tests for rootkeypackage_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkeypkgtestutils.hpp"
#include <aduc/file_test_utils.hpp>
#include <aduc/rootkeypackage_types.h>
#include <aduc/rootkeypackage_utils.h>
#include <algorithm>
#include <catch2/catch.hpp>
#include <parson.h>
#include <sstream>
#include <vector>

static std::string get_valid_rootkey_package_template_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/valid_test_rootkeypackage.json";
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

static std::string LoadTemplateRootKeyPackageJson(JSON_Value* rootKeyJsonValue)
{
    std::string templateStr;

    char* serializedTemplate_cstr = json_serialize_to_string(rootKeyJsonValue);
    REQUIRE(serializedTemplate_cstr != nullptr);
    templateStr = serializedTemplate_cstr;
    json_free_serialized_string(serializedTemplate_cstr);

    return templateStr;
}

static std::string fillout_protected_properties_template_params(
    const std::string& templateStr,
    const char* disabledHashPublicSigningKey,
    const char* modulus_1,
    const char* exponent_1,
    const char* modulus_2,
    const char* exponent_2,
    const char* modulus_3,
    const char* exponent_3)
{
    std::string str = aduc::FileTestUtils_applyTemplateParam(
        templateStr, "disabledHashPublicSigningKey", disabledHashPublicSigningKey);

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_1", modulus_1);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_1", exponent_1);

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_2", modulus_2);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_2", exponent_2);

    str = aduc::FileTestUtils_applyTemplateParam(str, "modulus_3", modulus_3);
    str = aduc::FileTestUtils_applyTemplateParam(str, "exponent_3", exponent_3);

    return str;
}

static std::string get_valid_rootkey_package(
    const char* disabledHashPublicSigningKey,
    const char* modulus_1,
    const char* exponent_1,
    const char* modulus_2,
    const char* exponent_2,
    const char* modulus_3,
    const char* exponent_3,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey1_privateKey,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey2_privateKey,
    const aduc::rootkeypkgtestutils::TestRSAPrivateKey& rootkey3_privateKey)
{
    std::stringstream ss;

    JSON_Value* pkgJsonValue = json_parse_file(get_valid_rootkey_package_template_json_path().c_str());
    REQUIRE(pkgJsonValue != nullptr);

    // Load template root key package JSON
    std::string templateStr = LoadTemplateRootKeyPackageJson(pkgJsonValue);

    // Fill out the "protected" properties template parameters, but do not fill out
    // the signatures "sig" properties yet.
    templateStr = fillout_protected_properties_template_params(
        templateStr,
        disabledHashPublicSigningKey,
        modulus_1,
        exponent_1,
        modulus_2,
        exponent_2,
        modulus_3,
        exponent_3);

    // Generate the signatures using the rootkey private keys passed in
    // and the protected properties data to be signed.
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
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON);

        result = ADUC_RootKeyPackageUtils_Parse("", &pkg);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON);

        result = ADUC_RootKeyPackageUtils_Parse("{[}", &pkg);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON);
    }

    SECTION("valid")
    {
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair1{};
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair2{};
        aduc::rootkeypkgtestutils::TestRSAKeyPair rootKeyPair3{};

        const auto& rootkey1_publickey = rootKeyPair1.GetPublicKey();
        const auto& rootkey2_publickey = rootKeyPair2.GetPublicKey();
        const auto& rootkey3_publickey = rootKeyPair3.GetPublicKey();

        std::string urlIntBase64EncodedHash_of_rootkey3_public_key = rootkey3_publickey.getSha256HashOfPublicKey();
        const char* PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3 =
            urlIntBase64EncodedHash_of_rootkey3_public_key.c_str();

        const char* ROOTKEY_1_MODULUS = rootkey1_publickey.GetModulus().c_str();
        const char* ROOTKEY_1_EXPONENT = rootkey1_publickey.GetExponent().c_str();

        const char* ROOTKEY_2_MODULUS = rootkey2_publickey.GetModulus().c_str();
        const char* ROOTKEY_2_EXPONENT = rootkey2_publickey.GetExponent().c_str();

        const char* ROOTKEY_3_MODULUS = rootkey3_publickey.GetModulus().c_str();
        const char* ROOTKEY_3_EXPONENT = rootkey3_publickey.GetExponent().c_str();

        const std::string rootKeyPkgJsonStr = get_valid_rootkey_package(
            PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3, // disabledHashPublicSigningKey
            ROOTKEY_1_MODULUS, // modulus_1
            ROOTKEY_1_EXPONENT, // exponent_1
            ROOTKEY_2_MODULUS, // modulus_2
            ROOTKEY_2_EXPONENT, // exponent_2
            ROOTKEY_3_MODULUS, // modulus_3
            ROOTKEY_1_MODULUS, // modulus_1
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
        CHECK(pkg.protectedProperties.version == 2);
        CHECK(pkg.protectedProperties.publishedTime == 1667343602);

        REQUIRE(VECTOR_size(pkg.protectedProperties.disabledRootKeys) == 2);

        // verify "disabledRootKeys"
        {
            std::vector<std::string> disabledRootKeys;
            disabledRootKeys.push_back(
                STRING_c_str((STRING_HANDLE)VECTOR_element(pkg.protectedProperties.disabledRootKeys, 0)));
            disabledRootKeys.push_back(
                STRING_c_str((STRING_HANDLE)VECTOR_element(pkg.protectedProperties.disabledRootKeys, 1)));

            CHECK(
                std::end(disabledRootKeys)
                != std::find(std::begin(disabledRootKeys), std::end(disabledRootKeys), std::string{ "rootkey1" }));
            CHECK(
                std::end(disabledRootKeys)
                != std::find(std::begin(disabledRootKeys), std::end(disabledRootKeys), std::string{ "rootkey2" }));
        }

        // verify "disabledSigningKeys"
        REQUIRE(VECTOR_size(pkg.protectedProperties.disabledSigningKeys) == 1);

        void* el = VECTOR_element(pkg.protectedProperties.disabledSigningKeys, 0);
        auto a = static_cast<ADUC_RootKeyPackage_Hash*>(el);
        ADUC_RootKeyPackage_Hash signingKeyHash{ *a };
        CHECK(signingKeyHash.alg == ADUC_RootKeyShaAlgorithm_SHA256);

        const CONSTBUFFER* hashBuffer = CONSTBUFFER_GetContent(signingKeyHash.hash);
        REQUIRE(hashBuffer != nullptr);

        //        std::string encodedHash = GetEncodedHashValue(hashBuffer->buffer, hashBuffer->size);
        //        CONSTBUFFER_DecRef(signingKeyHash.hash);// Cleanup CONSTBUFFER
        //        CHECK(encodedHash.c_str() == PUBLIC_SIGNING_KEY_CHAINING_UP_TO_ROOTKEY_3);

        //
        // Verify "signatures" properties
        //
    }
}
