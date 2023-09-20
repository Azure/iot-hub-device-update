/**
 * @file root_key_utils_ut.cpp
 * @brief Unit Tests for root_key_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "catch2/catch.hpp"
#include "umock_c/umock_c.h"

#include <aduc/calloc_wrapper.hpp>
#include <aduc/result.h>
#include <aduc/rootkeypackage_parse.h>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/types/adu_core.h>
#include <base64_utils.h>
#include <crypto_lib.h>

#define ENABLE_MOCKS
#include <root_key_list.h>
#undef ENABLE_MOCKS

#include <root_key_util.h>
#include <rootkey_store.hpp>

#define ENABLE_MOCKS
#include <root_key_util_helper.h>
#undef ENABLE_MOCKS

using ADUC::StringUtils::cstr_wrapper;
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

// Correct signature,
// clang-format off
std::string validRootKeyPackageJson  =  R"({"protected":{"version":1,"published":1675972876,"disabledRootKeys":[],"disabledSigningKeys":[],"rootKeys":{"testrootkey1":{"keyType":"RSA","n":"rPMhXSpDY53R8pZJi0oW3qvP9l_9ntRkJo-2US109_wr5_P-t54uCx-EwZs7UkXYNYnyu8GIbzL2dyQp7lMhmLv_cQFIhs6HMlLpSIMtbHYq_v6jMgdGb5ovfbwPbMsiESQiZc0xSEVMq60p8iCw58gIzMK1nWdYeUQMC8mU-G-O8c_z8SXlVjbwZ5AmjVg42Z3NW558gcgez0LxkRnGyALHZCCjJNSUjPTp7AMKL21S-C6aFFVWwFJdeUrhJkf__2cdAB6m3C6-wuO2pq1HRX-epjMmnQ06UjdUmKuIUjB2uyVcMTnkVzXPUD6D2rbBFAy1230Svw20YSP8P7n9xQ","e":65537},"testrootkey2":{"keyType":"RSA","n":"tKN8mDrKOtLIsUjPC_KHcu6YcyitaG9nKtpR_WQAYT8rNjtlORd5H2TuAsr4DutkX7x6SZv3y5RyTqVZKZNkmlbUALoRR1bJ-pGkEUtntB9Oaga2ZcmtYYOwTy2QdOLEee_fE6UZun-mWNPv2swMhmWJuMTkixUakq8PN4bSPDNjdn_moVowXmfesN31Ioi97zxKSp9XXhU6E92MX2E782-uxqshPFe-xEWRLGCA50-_yJeJMiRJSiMZdjQ4Su9o6D86WdgiTERP9cUoSQFoZWFnG8c76WL_Gn0T6pM47kPOJeGv2ZyDm9hGrLL2bjI2WTevDmOrzCjf8qHw6Kg58Q","e":65537}}},"signatures":[{"alg":"RS256","sig":"AjXYiFNj7kN7jbKZckwBQXDBiLKCTPQj8Oh1aqaUteW8tjyg6MZGjev-V8MirIw77e4xTLj7ZgBPt_qgWXH-otVal1zyUJlolPsmicugm8whhS4OVOqGfMJ8jTbT8yjiHGdKR3nWW1EQbRE6y38iAYCETXrHgVA2BM77fkBHj27P1etYPCkkjSdKtN6D-gdAuAzNsRQOv-8YZkIuMeD1d9kZAoHbDYmfpVHjooBd1_iys8f4aRKkhk18_3Alsxx63VtTNK3eSkPqb1v3Z-fpGnpW0rJZbf4XskCuTyysPu_Vgmnxn2CJccfseijlnnQiqpN5jEaNOU4TX_Yhc15wMg"},{"alg":"RS256","sig":"SBWi1Ae0EWrj9EKrL4qQGh_xumhdLl1BhASGX1Jc8QaM9PtreRIOIMxbC7LaZXmpTyjDrxpaNKwwIBVO8poWT5BocchL8vzcn0KTAwbl0OD-zoa5CdvurrtQJkx0L-yr685oz4AP05SiRBRuSYPGCty0D4Pzy09Yp9gHDN_2KGnFfkph5I64GmA6CB9mexXXz26xSucYHpMApO9yUgpkYCBVirdgP7aKyb-c6GK4LLuLCi_nTtrUMfEpfxYNmNp4zm0R2IjQ_C9Jyn7mY3YO3sSPRw88iv5f0QzKTGazRdkOHOnwPbDdsykZ4uSABBKtCKN9VVSUusnuv53ZPkc63Q"}]})";
// clang-format on

// clang-format off
std::string invalidRootKeyPackageJson = R"({"protected":{"version":1,"published":1675972876,"disabledRootKeys":[],"disabledSigningKeys":[],"rootKeys":{"testrootkey1":{"keyType":"RSA","n":"rPMhXSpDY53R8pZJi0oW3qvP9l_9ntRkJo-2US109_wr5_P-t54uCx-EwZs7UkXYNYnyu8GIbzL2dyQp7lMhmLv_cQFIhs6HMlLpSIMtbHYq_v6jMgdGb5ovfbwPbMsiESQiZc0xSEVMq60p8iCw58gIzMK1nWdYeUQMC8mU-G-O8c_z8SXlVjbwZ5AmjVg42Z3NW558gcgez0LxkRnGyALHZCCjJNSUjPTp7AMKL21S-C6aFFVWwFJdeUrhJkf__2cdAB6m3C6-wuO2pq1HRX-epjMmnQ06UjdUmKuIUjB2uyVcMTnkVzXPUD6D2rbBFAy1230Svw20YSP8P7n9xQ","e":65537},"testrootkey2":{"keyType":"RSA","n":"tKN8mDrKOtLIsUjPC_KHcu6YcyitaG9nKtpR_WQAYT8rNjtlORd5H2TuAsr4DutkX7x6SZv3y5RyTqVZKZNkmlbUALoRR1bJ-pGkEUtntB9Oaga2ZcmtYYOwTy2QdOLEee_fE6UZun-mWNPv2swMhmWJuMTkixUakq8PN4bSPDNjdn_moVowXmfesN31Ioi97zxKSp9XXhU6E92MX2E782-uxqshPFe-xEWRLGCA50-_yJeJMiRJSiMZdjQ4Su9o6D86WdgiTERP9cUoSQFoZWFnG8c76WL_Gn0T6pM47kPOJeGv2ZyDm9hGrLL2bjI2WTevDmOrzCjf8qHw6Kg58Q","e":65537}}},"signatures":[{"alg":"RS256","sig":"AjXYiFNj7kN7jbKZckwBQXDBiLKCTPQj8Oh1aqaUteW8tjyg6MZGjev-V8MirIw77e4xTLj7ZgBPt_qgWXH-otVal1zyUJlolPsmicugm8whhS4OVOqGfMJ8jTbT8yjiHGdKR3nWW1EQbRE6y38iAYCETXrHgVA2BM77fkBHj27P1etYPCkkjSdKtN6D-gdAuAzNsRQOv-8YZkIuMeD1d9kZAoHbDYmfpVHjooBd1_iys8f4aRKkhk18_3Alsxx63VtTNK3eSkPqb1v3Z-fpGnpW0rJZbf4XskCuTyysPu_Vgmnxn2CJccfseijlnnQiqpN5jEaNOU4TX_Yhc15wMg"},{"alg":"RS256","sig":"SBWi1Ae0EWrj9EKrL4qQGh_xumhdLl1BhASGX1Jc8QaM9PtreRIOIMxbC7LaZXmpTyjDrxpaNKwwIBVO8poWT5BocchL8vzcn0KTAwbl0OD-zoa5CdvurrtQJkx0L-yr685oz4AP05SiRBRuSYPGCty0D4Pzy09Yp9gHDN_2KGnFfkph5I64GmA6CB9mexXXz26xSucYHpMApO9yUgpkYCBVirdgP7aKyb-c6GK4LLuLCi_nTtrUMfEpfxYNmNp4zm0R2IjQ_C9Jyn7mY3YO3sSPRw88iv5f0QzKTGazRdkOHOnwPbDdsykZ4uSABBKtCKN9VVSUusnuv53ZPk1234"}]})";
// clang-format on

const RSARootKey* MockRootKeyList_GetHardcodedRsaRootKeys()
{
    return testHardcodedRootKeys;
}

size_t MockRootKeyList_numHardcodedKeys()
{
    return ARRAY_SIZE(testHardcodedRootKeys);
}

bool MockRootKeyUtility_RootKeyIsDisabled(const ADUC_RootKeyPackage* rootKeyPackage, const char* keyId)
{
    if (strcmp(keyId, "testrootkey2") == 0 || strcmp(keyId, "testrootkey_from_package") == 0)
    {
        return true;
    }
    return false;
}

class MockRootKeyStore : public IRootKeyStoreInternal
{
public:
    MockRootKeyStore() : m_pkg(nullptr) {}
    bool SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept override { return true; }
    const std::string* GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept override { return new std::string{""}; }
    bool SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept override
    {
        m_pkg = const_cast<ADUC_RootKeyPackage*>(package);
        return true;
    }

    bool GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) noexcept override
    {
        *outPackage = *m_pkg;
        m_pkg = nullptr;
        return true;
    }
    bool Load() noexcept override { return true; }
    ADUC_Result Persist() noexcept override { return { ADUC_Result_Success, 0 }; }

private:
    ADUC_RootKeyPackage* m_pkg;
};

class SignatureValidationMockHook
{
public:
    SignatureValidationMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, MockRootKeyList_numHardcodedKeys);
    }

    ~SignatureValidationMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, nullptr);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, nullptr);
    }

    SignatureValidationMockHook(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook& operator=(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook(SignatureValidationMockHook&&) = delete;
    SignatureValidationMockHook& operator=(SignatureValidationMockHook&&) = delete;

    RootKeyUtilContext* GetRootKeyUtilityContext()
    {
        m_context.rootKeyStoreHandle = &m_rootkey_store;
        m_context.rootKeyExtendedResult = 0;
        return &m_context;
    }

private:
    MockRootKeyStore m_rootkey_store;
    RootKeyUtilContext m_context;
};

class GetKeyForKidMockHook
{
public:
    GetKeyForKidMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, MockRootKeyList_numHardcodedKeys);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyUtility_RootKeyIsDisabled, MockRootKeyUtility_RootKeyIsDisabled);
    }

    ~GetKeyForKidMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, nullptr);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, nullptr);
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyUtility_RootKeyIsDisabled, nullptr);
    }

    GetKeyForKidMockHook(const GetKeyForKidMockHook&) = delete;
    GetKeyForKidMockHook& operator=(const GetKeyForKidMockHook&) = delete;
    GetKeyForKidMockHook(GetKeyForKidMockHook&&) = delete;
    GetKeyForKidMockHook& operator=(GetKeyForKidMockHook&&) = delete;

    RootKeyUtilContext* GetRootKeyUtilityContext()
    {
        m_context.rootKeyStoreHandle = &m_rootkey_store;
        m_context.rootKeyExtendedResult = 0;
        return &m_context;
    }

private:
    MockRootKeyStore m_rootkey_store;
    RootKeyUtilContext m_context;
};

TEST_CASE_METHOD(SignatureValidationMockHook, "RootKeyUtility_ValidateRootKeyPackage Signature Validation")
{
    SECTION("RootKeyUtility_ValidateRootKeyPackage - Valid RootKeyPackage Signature")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(), &pkg);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(GetRootKeyUtilityContext(), &pkg);

        CHECK(IsAducResultCodeSuccess(validationResult.ResultCode));
    }

    SECTION("RootKeyUtil_ValidateRootKeyPackage - Invalid RootKeyPackage Signature")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(invalidRootKeyPackageJson.c_str(), &pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(GetRootKeyUtilityContext(), &pkg);

        CHECK(validationResult.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED);
    }
}

TEST_CASE_METHOD(GetKeyForKidMockHook, "RootKeyUtility_GetKeyForKid - not disabled")
{
    SECTION("rootkey not disabled, found in hard-coded list")
    {
        CryptoKeyHandle rootKeyCryptoKey = nullptr;
        ADUC_Result result = RootKeyUtility_GetKeyForKid(GetRootKeyUtilityContext(), &rootKeyCryptoKey, "testrootkey1");
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(rootKeyCryptoKey != nullptr);
        if (rootKeyCryptoKey != nullptr)
        {
            CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
        }
    }

    SECTION("rootkey not found in either hard-coded list, or local store")
    {
        CryptoKeyHandle rootKeyCryptoKey = nullptr;
        ADUC_Result result = RootKeyUtility_GetKeyForKid(GetRootKeyUtilityContext(), &rootKeyCryptoKey, "testrootkey_does_not_exist");
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID);
        CHECK(rootKeyCryptoKey == nullptr);
        if (rootKeyCryptoKey != nullptr)
        {
            CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
        }
    }

    SECTION("disabled hard-coded rootkey")
    {
        CryptoKeyHandle rootKeyCryptoKey = nullptr;
        ADUC_Result result = RootKeyUtility_GetKeyForKid(GetRootKeyUtilityContext(), &rootKeyCryptoKey, "testrootkey2");
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED);
        CHECK(rootKeyCryptoKey == nullptr);
        if (rootKeyCryptoKey != nullptr)
        {
            CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
        }
    }

    SECTION("disabled non-hardcoded rootkey")
    {
        CryptoKeyHandle rootKeyCryptoKey = nullptr;
        ADUC_Result result = RootKeyUtility_GetKeyForKid(GetRootKeyUtilityContext(), &rootKeyCryptoKey, "testrootkey_from_package");
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED);
        CHECK(rootKeyCryptoKey == nullptr);
        if (rootKeyCryptoKey != nullptr)
        {
            CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
        }
    }
}
