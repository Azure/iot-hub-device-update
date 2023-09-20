
/**
 * @file root_key_utils_ut.cpp
 * @brief Unit Tests for root_key_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "catch2/catch.hpp"
#include "umock_c/umock_c.h"

#include <rootkey_store_helper.hpp>

TEST_CASE("WriteRootKeyPackageToFileAtomically")
{
    SECTION("Success case")
    {
        std::string filePath = get_dest_rootkey_package_json_path();
        ADUC_RootKeyPackage loadedRootKeyPackage = {};
        ADUC_RootKeyPackage* writtenRootKeyPackage = nullptr;

        STRING_HANDLE destPath = STRING_construct(filePath.c_str());
        REQUIRE(destPath != nullptr);
        REQUIRE(STRING_length(destPath) > 0);

        ADUC_Result packageResult = ADUC_RootKeyPackageUtils_Parse(validRootKeyPackageJson.c_str(), &loadedRootKeyPackage);
        REQUIRE(IsAducResultCodeSuccess(packageResult.ResultCode));

        ADUC_Result writeFileResult = rootkeystore::helper::WriteRootKeyPackageToFileAtomically(&loadedRootKeyPackage, destPath);
        CHECK(IsAducResultCodeSuccess(writeFileResult.ResultCode));


        REQUIRE(writtenRootKeyPackage != nullptr);

        CHECK(ADUC_RootKeyPackageUtils_AreEqual(writtenRootKeyPackage, &loadedRootKeyPackage));

        ADUC_RootKeyPackageUtils_Destroy(&loadedRootKeyPackage);

        STRING_delete(destPath);
    }
}

