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

const RSARootKey testHardcodedRootKeys [] = {
    {
        "rootkey1",
        "AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8",
        65537
    },
    {
        "rootkey2",
        "AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U",
        65537
    }
};

// Correct signature,
// clang-format off
std::string validRootKeyPackageJson  =  R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537},"rootkey2":{"keyType":"RSA","n":"AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"},{"alg":"RS256","sig":"UStcZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"}]})";
// clang-format on

// clang-format off
std::string invalidRootKeyPackageJson = R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537},"rootkey2":{"keyType":"RSA","n":"AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"},{"alg":"RS256","sig":"asdfZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"}]})";
// clang-format on

// clang-format off
std::string rootKeyPackageWithoutHardcodedRootKeyId =   R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537},"rootkey2":{"keyType":"RSA","n":"AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"}]})";
// clang-format off

const RSARootKey* MockRootKeyList_GetHardcodedRsaRootKeys()
{
    return testHardcodedRootKeys;
}

size_t MockRootKeyList_numHardcodedKeys()
{
    return ARRAY_SIZE(testHardcodedRootKeys);
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
