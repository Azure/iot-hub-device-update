/**
 * @file root_key_utils_ut.cpp
 * @brief Unit Tests for root_key_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"

#define ENABLE_MOCKS
#include "root_key_list.h"
#include "root_key_store.h"
#undef ENABLE_MOCKS

#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_utils.h"
#include "root_key_util.h"
#include <aduc/calloc_wrapper.hpp>
#include <aduc/result.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <iostream>
#include <parson.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

using ADUC::StringUtils::cstr_wrapper;
using Catch::Matchers::Equals;
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;

const RSARootKey testHardcodedRootKeys[] = {
    { "testrootkey1",
      "rPMhXSpDY53R8pZJi0oW3qvP9l_9ntRkJo-2US109_wr5_P-t54uCx-EwZs7UkXYNYnyu8GIbzL2dyQp7lMhmLv_cQFIhs6HMlLpSIMtbHYq_v6jMgdGb5ovfbwPbMsiESQiZc0xSEVMq60p8iCw58gIzMK1nWdYeUQMC8mU-G-O8c_z8SXlVjbwZ5AmjVg42Z3NW558gcgez0LxkRnGyALHZCCjJNSUjPTp7AMKL21S-C6aFFVWwFJdeUrhJkf__2cdAB6m3C6-wuO2pq1HRX-epjMmnQ06UjdUmKuIUjB2uyVcMTnkVzXPUD6D2rbBFAy1230Svw20YSP8P7n9xQ",
      65537 },
    { "testrootkey2",
      "tKN8mDrKOtLIsUjPC_KHcu6YcyitaG9nKtpR_WQAYT8rNjtlORd5H2TuAsr4DutkX7x6SZv3y5RyTqVZKZNkmlbUALoRR1bJ-pGkEUtntB9Oaga2ZcmtYYOwTy2QdOLEee_fE6UZun-mWNPv2swMhmWJuMTkixUakq8PN4bSPDNjdn_moVowXmfesN31Ioi97zxKSp9XXhU6E92MX2E782-uxqshPFe-xEWRLGCA50-_yJeJMiRJSiMZdjQ4Su9o6D86WdgiTERP9cUoSQFoZWFnG8c76WL_Gn0T6pM47kPOJeGv2ZyDm9hGrLL2bjI2WTevDmOrzCjf8qHw6Kg58Q",
      65537 }
};

static std::string get_valid_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/validrootkeypackage.json";
    return path;
};

static std::string get_invalid_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/invalidrootkeypackage.json";
    return path;
};

static std::string get_nonexistent_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/doesnotexistrootkeypackage.json";
    return path;
};

static std::string get_dest_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/test-rootkeypackage-write.json";
    return path;
};

static std::string get_prod_disabled_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/disabledRootKeyProd.json";
    return path;
};

static std::string get_prod_disabled_signingkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/disabledSigningKeyProd.json";
    return path;
};

// Correct signature,
// clang-format off
std::string validRootKeyPackageJson  =  R"({"protected":{"version":1,"published":1675972876,"disabledRootKeys":[],"disabledSigningKeys":[],"rootKeys":{"testrootkey1":{"keyType":"RSA","n":"rPMhXSpDY53R8pZJi0oW3qvP9l_9ntRkJo-2US109_wr5_P-t54uCx-EwZs7UkXYNYnyu8GIbzL2dyQp7lMhmLv_cQFIhs6HMlLpSIMtbHYq_v6jMgdGb5ovfbwPbMsiESQiZc0xSEVMq60p8iCw58gIzMK1nWdYeUQMC8mU-G-O8c_z8SXlVjbwZ5AmjVg42Z3NW558gcgez0LxkRnGyALHZCCjJNSUjPTp7AMKL21S-C6aFFVWwFJdeUrhJkf__2cdAB6m3C6-wuO2pq1HRX-epjMmnQ06UjdUmKuIUjB2uyVcMTnkVzXPUD6D2rbBFAy1230Svw20YSP8P7n9xQ","e":65537},"testrootkey2":{"keyType":"RSA","n":"tKN8mDrKOtLIsUjPC_KHcu6YcyitaG9nKtpR_WQAYT8rNjtlORd5H2TuAsr4DutkX7x6SZv3y5RyTqVZKZNkmlbUALoRR1bJ-pGkEUtntB9Oaga2ZcmtYYOwTy2QdOLEee_fE6UZun-mWNPv2swMhmWJuMTkixUakq8PN4bSPDNjdn_moVowXmfesN31Ioi97zxKSp9XXhU6E92MX2E782-uxqshPFe-xEWRLGCA50-_yJeJMiRJSiMZdjQ4Su9o6D86WdgiTERP9cUoSQFoZWFnG8c76WL_Gn0T6pM47kPOJeGv2ZyDm9hGrLL2bjI2WTevDmOrzCjf8qHw6Kg58Q","e":65537}}},"signatures":[{"alg":"RS256","sig":"AjXYiFNj7kN7jbKZckwBQXDBiLKCTPQj8Oh1aqaUteW8tjyg6MZGjev-V8MirIw77e4xTLj7ZgBPt_qgWXH-otVal1zyUJlolPsmicugm8whhS4OVOqGfMJ8jTbT8yjiHGdKR3nWW1EQbRE6y38iAYCETXrHgVA2BM77fkBHj27P1etYPCkkjSdKtN6D-gdAuAzNsRQOv-8YZkIuMeD1d9kZAoHbDYmfpVHjooBd1_iys8f4aRKkhk18_3Alsxx63VtTNK3eSkPqb1v3Z-fpGnpW0rJZbf4XskCuTyysPu_Vgmnxn2CJccfseijlnnQiqpN5jEaNOU4TX_Yhc15wMg"},{"alg":"RS256","sig":"SBWi1Ae0EWrj9EKrL4qQGh_xumhdLl1BhASGX1Jc8QaM9PtreRIOIMxbC7LaZXmpTyjDrxpaNKwwIBVO8poWT5BocchL8vzcn0KTAwbl0OD-zoa5CdvurrtQJkx0L-yr685oz4AP05SiRBRuSYPGCty0D4Pzy09Yp9gHDN_2KGnFfkph5I64GmA6CB9mexXXz26xSucYHpMApO9yUgpkYCBVirdgP7aKyb-c6GK4LLuLCi_nTtrUMfEpfxYNmNp4zm0R2IjQ_C9Jyn7mY3YO3sSPRw88iv5f0QzKTGazRdkOHOnwPbDdsykZ4uSABBKtCKN9VVSUusnuv53ZPkc63Q"}]})";
// clang-format on

// clang-format off
std::string invalidRootKeyPackageJson = R"({"protected":{"version":1,"published":1675972876,"disabledRootKeys":[],"disabledSigningKeys":[],"rootKeys":{"testrootkey1":{"keyType":"RSA","n":"rPMhXSpDY53R8pZJi0oW3qvP9l_9ntRkJo-2US109_wr5_P-t54uCx-EwZs7UkXYNYnyu8GIbzL2dyQp7lMhmLv_cQFIhs6HMlLpSIMtbHYq_v6jMgdGb5ovfbwPbMsiESQiZc0xSEVMq60p8iCw58gIzMK1nWdYeUQMC8mU-G-O8c_z8SXlVjbwZ5AmjVg42Z3NW558gcgez0LxkRnGyALHZCCjJNSUjPTp7AMKL21S-C6aFFVWwFJdeUrhJkf__2cdAB6m3C6-wuO2pq1HRX-epjMmnQ06UjdUmKuIUjB2uyVcMTnkVzXPUD6D2rbBFAy1230Svw20YSP8P7n9xQ","e":65537},"testrootkey2":{"keyType":"RSA","n":"tKN8mDrKOtLIsUjPC_KHcu6YcyitaG9nKtpR_WQAYT8rNjtlORd5H2TuAsr4DutkX7x6SZv3y5RyTqVZKZNkmlbUALoRR1bJ-pGkEUtntB9Oaga2ZcmtYYOwTy2QdOLEee_fE6UZun-mWNPv2swMhmWJuMTkixUakq8PN4bSPDNjdn_moVowXmfesN31Ioi97zxKSp9XXhU6E92MX2E782-uxqshPFe-xEWRLGCA50-_yJeJMiRJSiMZdjQ4Su9o6D86WdgiTERP9cUoSQFoZWFnG8c76WL_Gn0T6pM47kPOJeGv2ZyDm9hGrLL2bjI2WTevDmOrzCjf8qHw6Kg58Q","e":65537}}},"signatures":[{"alg":"RS256","sig":"AjXYiFNj7kN7jbKZckwBQXDBiLKCTPQj8Oh1aqaUteW8tjyg6MZGjev-V8MirIw77e4xTLj7ZgBPt_qgWXH-otVal1zyUJlolPsmicugm8whhS4OVOqGfMJ8jTbT8yjiHGdKR3nWW1EQbRE6y38iAYCETXrHgVA2BM77fkBHj27P1etYPCkkjSdKtN6D-gdAuAzNsRQOv-8YZkIuMeD1d9kZAoHbDYmfpVHjooBd1_iys8f4aRKkhk18_3Alsxx63VtTNK3eSkPqb1v3Z-fpGnpW0rJZbf4XskCuTyysPu_Vgmnxn2CJccfseijlnnQiqpN5jEaNOU4TX_Yhc15wMg"},{"alg":"RS256","sig":"SBWi1Ae0EWrj9EKrL4qQGh_xumhdLl1BhASGX1Jc8QaM9PtreRIOIMxbC7LaZXmpTyjDrxpaNKwwIBVO8poWT5BocchL8vzcn0KTAwbl0OD-zoa5CdvurrtQJkx0L-yr685oz4AP05SiRBRuSYPGCty0D4Pzy09Yp9gHDN_2KGnFfkph5I64GmA6CB9mexXXz26xSucYHpMApO9yUgpkYCBVirdgP7aKyb-c6GK4LLuLCi_nTtrUMfEpfxYNmNp4zm0R2IjQ_C9Jyn7mY3YO3sSPRw88iv5f0QzKTGazRdkOHOnwPbDdsykZ4uSABBKtCKN9VVSUusnuv53ZPk1234"}]})";
// // clang-format on



const RSARootKey* MockRootKeyList_GetHardcodedRsaRootKeys()
{
    return testHardcodedRootKeys;
}

size_t MockRootKeyList_numHardcodedKeys()
{
    return ARRAY_SIZE(testHardcodedRootKeys);
}

std::string g_mockedRootKeyStorePath = "";

const char* MockRootKeyStore_GetRootKeyStorePath()
{
    return g_mockedRootKeyStorePath.c_str();
}

class SignatureValidationMockHook
{
public:
    SignatureValidationMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, MockRootKeyList_numHardcodedKeys);
    }

    ~SignatureValidationMockHook() = default;

    SignatureValidationMockHook(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook& operator=(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook(SignatureValidationMockHook&&) = delete;
    SignatureValidationMockHook& operator=(SignatureValidationMockHook&&) = delete;
};

class GetRootKeyValidationMockHook
{
public:
    GetRootKeyValidationMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, MockRootKeyList_numHardcodedKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyStore_GetRootKeyStorePath, MockRootKeyStore_GetRootKeyStorePath);
    }

    ~GetRootKeyValidationMockHook() = default;

    GetRootKeyValidationMockHook(const GetRootKeyValidationMockHook&) = delete;
    GetRootKeyValidationMockHook& operator=(const GetRootKeyValidationMockHook&) = delete;
    GetRootKeyValidationMockHook(GetRootKeyValidationMockHook&&) = delete;
    GetRootKeyValidationMockHook& operator=(GetRootKeyValidationMockHook&&) = delete;
};

TEST_CASE_METHOD(SignatureValidationMockHook, "RootKeyUtility_ValidateRootKeyPackage Signature Validation")
{
    SECTION("RootKeyUtility_ValidateRootKeyPackage - Valid RootKeyPackage Signature")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

        CHECK(IsAducResultCodeSuccess(validationResult.ResultCode));

    }
    SECTION("RootKeyUtil_ValidateRootKeyPackage - Invalid RootKeyPackage Signature")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(invalidRootKeyPackageJson.c_str(),&pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

        CHECK(validationResult.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED);
    }
}


TEST_CASE_METHOD(SignatureValidationMockHook, "RootKeyUtility_LoadPackageFromDisk")
{
    SECTION("Success case- valid package with valid signatures")
    {
        std::string filePath = get_valid_example_rootkey_package_json_path();
        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage,filePath.c_str(), true /* validateSignatures */);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(rootKeyPackage != nullptr);

        ADUC_RootKeyPackage correctPackage = {};

        ADUC_Result lResult = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&correctPackage);

        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));

        CHECK(ADUC_RootKeyPackageUtils_AreEqual(rootKeyPackage,&correctPackage));

        ADUC_RootKeyPackageUtils_Destroy(rootKeyPackage);
        free(rootKeyPackage);
    }
    SECTION ("Fail case - valid test package path, invalid signature")
    {
        std::string filePath = get_invalid_example_rootkey_package_json_path();
        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage,filePath.c_str(), true /* validateSignatures */);

        CHECK(IsAducResultCodeFailure(result.ResultCode));
        REQUIRE(rootKeyPackage == nullptr);
    }
    SECTION ("Fail case - invalid test package path")
    {
        std::string filePath = get_nonexistent_example_rootkey_package_json_path();

        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage,filePath.c_str(), true /* validateSignatures */);
        CHECK(IsAducResultCodeFailure(result.ResultCode));

        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_LOAD_FROM_STORE);

        REQUIRE(rootKeyPackage == nullptr);
    }
}

TEST_CASE_METHOD(SignatureValidationMockHook,"RootKeyUtility_WriteRootKeyPackageToFileAtomically")
{
    SECTION("Success case")
    {
        std::string filePath = get_dest_rootkey_package_json_path();
        ADUC_RootKeyPackage loadedRootKeyPackage = {};
        ADUC_RootKeyPackage* writtenRootKeyPackage = nullptr;

        STRING_HANDLE destPath = STRING_construct(filePath.c_str());
        REQUIRE(destPath != nullptr);

        ADUC_Result pResult = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&loadedRootKeyPackage);

        REQUIRE(IsAducResultCodeSuccess(pResult.ResultCode));

        ADUC_Result wResult = RootKeyUtility_WriteRootKeyPackageToFileAtomically(&loadedRootKeyPackage,destPath);

        CHECK(IsAducResultCodeSuccess(wResult.ResultCode));

        ADUC_Result lResult = RootKeyUtility_LoadPackageFromDisk(&writtenRootKeyPackage,filePath.c_str(), true /* validateSignatures */);

        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));
        REQUIRE(writtenRootKeyPackage != nullptr);

        CHECK(ADUC_RootKeyPackageUtils_AreEqual(writtenRootKeyPackage,&loadedRootKeyPackage));

        ADUC_RootKeyPackageUtils_Destroy(&loadedRootKeyPackage);
        ADUC_RootKeyPackageUtils_Destroy(writtenRootKeyPackage);

        free(writtenRootKeyPackage);

        STRING_delete(destPath);
    }
}

TEST_CASE("RootKeyUtility_RootKeyIsDisabled")
{
    ADUC_RootKeyPackage* pkg = nullptr;
    std::string filePath = get_prod_disabled_rootkey_package_json_path();
    ADUC_Result lResult = RootKeyUtility_LoadPackageFromDisk(&pkg, filePath.c_str(), false /* validateSignatures */);
    REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));
    REQUIRE(pkg != nullptr);
    CHECK(RootKeyUtility_RootKeyIsDisabled(pkg, "ADU.200702.R"));
    CHECK_FALSE(RootKeyUtility_RootKeyIsDisabled(pkg, "ADU.200703.R"));
    free(pkg);
}

TEST_CASE("RootKeyUtility_ReloadPackageFromDisk")
{
    SECTION("bad signature should fail")
    {
        std::string filePath = get_prod_disabled_rootkey_package_json_path();
        ADUC_Result lResult = RootKeyUtility_ReloadPackageFromDisk(filePath.c_str(), true /* validateSignatures */);
        CHECK(IsAducResultCodeFailure(lResult.ResultCode));
    }
}

TEST_CASE("RootKeyUtility_GetDisabledSigningKeys")
{
    SECTION("prod - no disabled signing keys")
    {
        std::string filePath = get_prod_disabled_rootkey_package_json_path();
        ADUC_Result lResult = RootKeyUtility_ReloadPackageFromDisk(filePath.c_str(), false /* validateSignatures */);
        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));
        VECTOR_HANDLE disabledSigningKeyList = nullptr;
        ADUC_Result result = RootKeyUtility_GetDisabledSigningKeys(&disabledSigningKeyList);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(disabledSigningKeyList != nullptr);
        CHECK(VECTOR_size(disabledSigningKeyList) == 0);
    }

    SECTION("prod - one disabled signing key")
    {
        std::string filePath = get_prod_disabled_signingkey_package_json_path();
        ADUC_Result lResult = RootKeyUtility_ReloadPackageFromDisk(filePath.c_str(), false /* validateSignatures */);
        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));
        VECTOR_HANDLE disabledSigningKeyList = nullptr;
        ADUC_Result result = RootKeyUtility_GetDisabledSigningKeys(&disabledSigningKeyList);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(disabledSigningKeyList != nullptr);
        CHECK(VECTOR_size(disabledSigningKeyList) == 1);
        CHECK(VECTOR_element(disabledSigningKeyList, 0) != nullptr);
        auto hashElement = (ADUC_RootKeyPackage_Hash*)VECTOR_element(disabledSigningKeyList, 0);
        CHECK(hashElement->alg == SHA256);
        CHECK(hashElement->hash != nullptr);

        cstr_wrapper base64url{ Base64URLEncode(CONSTBUFFER_GetContent(hashElement->hash)->buffer, CONSTBUFFER_GetContent(hashElement->hash)->size) };
        CHECK_THAT(base64url.get(), Equals("q5xF2ARjhdtH-kaLNTwZAMoXdy0iJQjziQ_AyZWDPRA"));
    }
}

TEST_CASE_METHOD(GetRootKeyValidationMockHook, "RootKeyUtility_GetKeyForKid")
{
    SECTION("Get hardcoded key")
    {
         // set the store path to the valid package tht we know contains the hardcoded key
        g_mockedRootKeyStorePath = get_valid_example_rootkey_package_json_path();

        // make sure to reload the store
        RootKeyUtility_ReloadPackageFromDisk(g_mockedRootKeyStorePath.c_str(), true /* validateSignatures */);
        CryptoKeyHandle key = nullptr;
        const char* keyId = "testrootkey1";

        ADUC_Result result = RootKeyUtility_GetKeyForKid(&key, keyId);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(key != nullptr);

        CryptoUtils_FreeCryptoKeyHandle(key);
    }
    SECTION("Get hardcoded key, disabled")
    {
        // set the store path to the valid package tht we know contains the hardcoded key
        g_mockedRootKeyStorePath = get_prod_disabled_rootkey_package_json_path();

        // Make sure to reload the store
        RootKeyUtility_ReloadPackageFromDisk(g_mockedRootKeyStorePath.c_str(), true /* validateSignatures */);

        CryptoKeyHandle key = nullptr;
        const char* keyId = "ADU.200702.R";

        ADUC_Result result = RootKeyUtility_GetKeyForKid(&key, keyId);

        CHECK(IsAducResultCodeFailure(result.ResultCode));

        CHECK(key == nullptr);


    }
    SECTION("Get key from store")
    {
        // set the store path to the valid package tht we know contains the hardcoded key
        g_mockedRootKeyStorePath = get_valid_example_rootkey_package_json_path();

        // make sure to reload the store
        RootKeyUtility_ReloadPackageFromDisk(g_mockedRootKeyStorePath.c_str(), true /* validateSignatures */);
        CryptoKeyHandle key = nullptr;
        const char* keyId = "testrootkey1";

        ADUC_Result result = RootKeyUtility_GetKeyForKid(&key, keyId);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(key != nullptr);

        CryptoUtils_FreeCryptoKeyHandle(key);
    }
    SECTION("Get non-existent key")
    {
        // set the store path to the valid package tht we know contains the hardcoded key
        g_mockedRootKeyStorePath = get_nonexistent_example_rootkey_package_json_path();

        // make sure to reload the store
        RootKeyUtility_ReloadPackageFromDisk(g_mockedRootKeyStorePath.c_str(), true /* validateSignatures */);
        CryptoKeyHandle key = nullptr;
        const char* keyId = "testrootkey3";

        ADUC_Result result = RootKeyUtility_GetKeyForKid(&key, keyId);

        CHECK(IsAducResultCodeFailure(result.ResultCode));

        CHECK(key == nullptr);
    }
}
