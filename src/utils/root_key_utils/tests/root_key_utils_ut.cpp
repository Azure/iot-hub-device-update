/**
 * @file crypto_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"

#define ENABLE_MOCKS
#include "root_key_list.h"
#undef ENABLE_MOCKS

#include "root_key_result.h"
#include "root_key_util.h"
#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_utils.h"

#include <aduc/calloc_wrapper.hpp>
#include <catch2/catch.hpp>
#include <cstring>
#include <parson.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <iostream>

using ADUC::StringUtils::cstr_wrapper;
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;

RSARootKey testHardcodedRootKeys [] = {
    {
        "testrootkey1",
        "",
        65537
    },
    {
        "testrootkey1",
        "",
        65537
    },
    {
        "testrootkey1",
        "",
        65537
    },
};

// Correct signature,
std::string validRootKeyPackageJson  = "";

std::string invalidRootKeyPackageJson = "";

std::string rootKeyPackageWithoutHardcodedRootKeyId = "";

const RSARootKey* MockRootKeyList_GetHardcodedRsaRootKeys()
{
    return testHardcodedRootKeys;
}

class SignatureValidationMockHook
{
public:
    SignatureValidationMockHook()
    {
        REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
    }

    ~SignatureValidationMockHook() = default;

    SignatureValidationMockHook(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook& operator=(const SignatureValidationMockHook&) = delete;
    SignatureValidationMockHook(SignatureValidationMockHook&&) = delete;
    SignatureValidationMockHook& operator=(SignatureValidationMockHook&&) = delete;
};

TEST_CASE_METHOD(SignatureValidationMockHook, "RootKeyUtil_ValidateRootKeyPackage Signature Validation")
{
    SECTION("RootKeyUtil_ValidateRootKeyPackage - Valid RootKeyPackage")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        RootKeyUtility_ValidationResult validationResult = RootKeyUtil_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

        CHECK(validationResult == RootKeyUtility_ValidationResult_Success);

    }
    SECTION("RootKeyUtil_ValidateRootKeyPackage - Invalid RootKeyPackage")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(invalidRootKeyPackageJson.c_str(),&pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        RootKeyUtility_ValidationResult validationResult = RootKeyUtil_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

        CHECK(validationResult == RootKeyUtility_ValidationResult_SignatureValidationFailed);
    }
    SECTION("RootKeyUtil_ValidateRootKeyPackage - Missing Signature")
    {
        ADUC_RootKeyPackage pkg = {};

        ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageWithoutHardcodedRootKeyId.c_str(),&pkg);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        RootKeyUtility_ValidationResult validationResult = RootKeyUtil_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

        CHECK(validationResult == RootKeyUtility_ValidationResult_SignatureForKeyNotFound);
    }
}
