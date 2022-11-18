/**
 * @file rootkeypackage_utils_ut.cpp
 * @brief Unit Tests for rootkeypackage_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/rootkeypackage_utils.h>
#include <catch2/catch.hpp>

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
}
