/**
 * @file extension_manager_ut.cpp
 * @brief Unit Tests for extension manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/result.h>
#include <catch2/catch.hpp>
#include <extension_manager_download_test_case.hpp>

bool operator==(ADUC_Result a, ADUC_Result b)
{
    return a.ResultCode == b.ResultCode && a.ExtendedResultCode == b.ExtendedResultCode;
}

TEST_CASE("ExtensionManager::Download success should return success ResultCode")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadSuccess };
    REQUIRE_NOTHROW(testCase.RunScenario());
    CHECK(testCase.GetActualResult() == testCase.GetExpectedResult());
}

TEST_CASE("ExtensionManager::Download failure should return failure ResultCode and ERC")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadFailure };
    REQUIRE_NOTHROW(testCase.RunScenario());
    CHECK(testCase.GetActualResult() == testCase.GetExpectedResult());
}
