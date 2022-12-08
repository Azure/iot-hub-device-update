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
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;

const RSARootKey testHardcodedRootKeys[] = {
    { "rootkey1",
      "AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8",
      65537 },
    { "rootkey2",
      "AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U",
      65537 }
};

static std::string get_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/root_key_utils/rootkeypackage.json";
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
std::string validRootKeyPackageJson  =  R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537},"rootkey2":{"keyType":"RSA","n":"AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"},{"alg":"RS256","sig":"UStcZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"}]})";
// clang-format on

// clang-format off
std::string invalidRootKeyPackageJson = R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537},"rootkey2":{"keyType":"RSA","n":"AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"},{"alg":"RS256","sig":"asdfZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"}]})";
// clang-format on

// clang-format off
std::string rootKeyPackageWithoutHardcodedRootKeyId =   R"({"protected":{"version":1,"published":1667343602,"disabledRootKeys":["rootkey2"],"disabledSigningKeys":[{"alg":"SHA256","hash":"sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"}],"rootKeys":{"rootkey1":{"keyType":"RSA","n":"AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8","e":65537}}},"signatures":[{"alg":"RS256","sig":"aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"},{"alg":"RS256","sig":"UStcZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"}]})";
// clang-format off

const RSARootKey* MockRootKeyList_GetHardcodedRsaRootKeys()
{
    return testHardcodedRootKeys;
}

size_t MockRootKeyList_numHardcodedKeys()
{
    return ARRAY_SIZE(testHardcodedRootKeys);
}

//
// Only commented out for unit tests running
// Bug for Fix: https://microsoft.visualstudio.com/OS/_workitems/edit/42520383
//
// class SignatureValidationMockHook
// {
// public:
//     SignatureValidationMockHook()
//     {
//         REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_GetHardcodedRsaRootKeys, MockRootKeyList_GetHardcodedRsaRootKeys);
//         REGISTER_GLOBAL_MOCK_HOOK(RootKeyList_numHardcodedKeys, MockRootKeyList_numHardcodedKeys);
//     }

//     ~SignatureValidationMockHook() = default;

//     SignatureValidationMockHook(const SignatureValidationMockHook&) = delete;
//     SignatureValidationMockHook& operator=(const SignatureValidationMockHook&) = delete;
//     SignatureValidationMockHook(SignatureValidationMockHook&&) = delete;
//     SignatureValidationMockHook& operator=(SignatureValidationMockHook&&) = delete;
// };

// TEST_CASE_METHOD(SignatureValidationMockHook, "RootKeyUtility_ValidateRootKeyPackage Signature Validation")
// {
//     SECTION("RootKeyUtility_ValidateRootKeyPackage - Valid RootKeyPackage")
//     {
//         ADUC_RootKeyPackage pkg = {};

//         ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&pkg);

//         REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
//         ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

//         CHECK(IsAducResultCodeSuccess(validationResult.ResultCode));

//     }
//     SECTION("RootKeyUtil_ValidateRootKeyPackage - Invalid RootKeyPackage")
//     {
//         ADUC_RootKeyPackage pkg = {};

//         ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(invalidRootKeyPackageJson.c_str(),&pkg);

//         REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
//         ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

//         CHECK(validationResult.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED);
//     }
//     SECTION("RootKeyUtil_ValidateRootKeyPackage - Missing Signature")
//     {
//         ADUC_RootKeyPackage pkg = {};

//         ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageWithoutHardcodedRootKeyId.c_str(),&pkg);

//         REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
//         ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&pkg);

//         CHECK(validationResult.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_FOR_KEY_NOT_FOUND);
//     }
// }


TEST_CASE("RootKeyUtility_LoadPackageFromDisk")
{
    SECTION("Success case")
    {
        std::string filePath = get_example_rootkey_package_json_path();
        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage,filePath.c_str());

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(rootKeyPackage != nullptr);

        ADUC_RootKeyPackage correctPackage = {};

        ADUC_Result lResult = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(),&correctPackage);

        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));

        CHECK(ADUC_RootKeyPackageUtils_AreEqual(rootKeyPackage,&correctPackage));

        ADUC_RootKeyPackageUtils_Destroy(rootKeyPackage);
        free(rootKeyPackage);
    }
}

TEST_CASE("RootKeyUtility_WriteRootKeyPackageToFileAtomically")
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

        ADUC_Result lResult = RootKeyUtility_LoadPackageFromDisk(&writtenRootKeyPackage,filePath.c_str());

        REQUIRE(IsAducResultCodeSuccess(lResult.ResultCode));
        REQUIRE(writtenRootKeyPackage != nullptr);

        CHECK(ADUC_RootKeyPackageUtils_AreEqual(writtenRootKeyPackage,&loadedRootKeyPackage));

        ADUC_RootKeyPackageUtils_Destroy(&loadedRootKeyPackage);
        ADUC_RootKeyPackageUtils_Destroy(writtenRootKeyPackage);

        free(writtenRootKeyPackage);

        STRING_delete(destPath);
    }
}

