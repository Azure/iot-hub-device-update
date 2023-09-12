/**
 * @file root_key_utils_load_package_ut.cpp
 * @brief Unit Tests for root_key_utils library load package functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"

#include "aduc/rootkeypackage_utils.h"
#include "root_key_util_helper.h"

#include <aduc/calloc_wrapper.hpp>
#include <aduc/result.h>
#include <catch2/catch.hpp>

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


TEST_CASE("RootKeyUtility_LoadPackageFromDisk")
{
    SECTION("Success case- valid package with valid signatures")
    {
        std::string filePath = get_valid_example_rootkey_package_json_path();
        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage, filePath.c_str());

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(rootKeyPackage != nullptr);

        ADUC_RootKeyPackageUtils_Destroy(rootKeyPackage);
        free(rootKeyPackage);
    }
    SECTION("Fail case - valid test package path, invalid signature")
    {
        std::string filePath = get_invalid_example_rootkey_package_json_path();
        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage, filePath.c_str());

        CHECK(IsAducResultCodeFailure(result.ResultCode));
        REQUIRE(rootKeyPackage == nullptr);
    }
    SECTION("Fail case - invalid test package path")
    {
        std::string filePath = get_nonexistent_example_rootkey_package_json_path();

        ADUC_RootKeyPackage* rootKeyPackage = nullptr;

        ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(&rootKeyPackage, filePath.c_str());
        CHECK(IsAducResultCodeFailure(result.ResultCode));

        CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_LOAD_FROM_STORE);

        REQUIRE(rootKeyPackage == nullptr);
    }
}
