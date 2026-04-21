/**
 * @file extension_manager_ut.cpp
 * @brief Unit Tests for extension manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/contract_utils.h>
#include <aduc/extension_manager.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <catch2/catch_all.hpp>
#include <dlfcn.h>
#include <extension_manager_download_test_case.hpp>

bool operator==(ADUC_Result a, ADUC_Result b)
{
    return a.ResultCode == b.ResultCode && a.ExtendedResultCode == b.ExtendedResultCode;
}

TEST_CASE("ExtensionManager::Download success should return success ResultCode")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadSuccess };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

TEST_CASE("ExtensionManager::Download failure should return failure ResultCode and ERC")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadFailure };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

//
// V2 contract tests
//

TEST_CASE("ExtensionManager::Download with V2 contract should succeed")
{
    ADUC_ExtensionContractInfo v2Contract{ ADUC_V2_CONTRACT_MAJOR_VER, ADUC_V2_CONTRACT_MINOR_VER };

    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadSuccess, v2Contract };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    CHECK(actual_result.ResultCode == 1); // success
    CHECK(actual_result.ExtendedResultCode == 0);
}

TEST_CASE("ExtensionManager::Download with unsupported contract version should fail")
{
    ADUC_ExtensionContractInfo unsupportedContract{ 3, 0 };

    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::BasicDownloadSuccess, unsupportedContract };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    CHECK(IsAducResultCodeFailure(actual_result.ResultCode));
    CHECK(actual_result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION);
}

TEST_CASE("InitializeContentDownloader with unsupported contract version returns error")
{
    // Set up mock library and an unsupported contract version.
    // The unsupported version check happens before any dlsym call,
    // so this is safe with a mock (non-dlopen) library handle.
    class MockLib
    {
    };
    MockLib mock;
    ExtensionManager::SetContentDownloaderLibrary(&mock);
    ExtensionManager::SetContentDownloaderContractVersion(ADUC_ExtensionContractInfo{ 3, 0 });

    ADUC_Result result = ExtensionManager::InitializeContentDownloader(nullptr, ADUC_LOG_DEBUG);

    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION);
}

TEST_CASE("InitializeContentDownloader with V1 contract does not return unsupported version error")
{
    // Use a real dlopen handle (the main program) so dlsym is safe to call.
    // dlsym will return NULL for "Initialize" since the test binary doesn't export it.
    void* safeHandle = dlopen(nullptr, RTLD_LAZY);
    REQUIRE(safeHandle != nullptr);

    ExtensionManager::SetContentDownloaderLibrary(safeHandle);
    ExtensionManager::SetContentDownloaderContractVersion(
        ADUC_ExtensionContractInfo{ ADUC_V1_CONTRACT_MAJOR_VER, ADUC_V1_CONTRACT_MINOR_VER });

    ADUC_Result result = ExtensionManager::InitializeContentDownloader(nullptr, ADUC_LOG_DEBUG);

    CHECK(IsAducResultCodeFailure(result.ResultCode));
    // Must NOT be the unsupported contract version error — proves V1 path was taken.
    CHECK(result.ExtendedResultCode != ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION);
    // Should be INITIALIZEPROC_NOTIMP since the test binary doesn't export "Initialize".
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZEPROC_NOTIMP);

    ExtensionManager::SetContentDownloaderLibrary(nullptr);
    dlclose(safeHandle);
}

TEST_CASE("InitializeContentDownloader with V2 contract does not return unsupported version error")
{
    // Same as V1 test but for V2 contract: verify V2 is accepted and V2 code path is taken.
    void* safeHandle = dlopen(nullptr, RTLD_LAZY);
    REQUIRE(safeHandle != nullptr);

    ExtensionManager::SetContentDownloaderLibrary(safeHandle);
    ExtensionManager::SetContentDownloaderContractVersion(
        ADUC_ExtensionContractInfo{ ADUC_V2_CONTRACT_MAJOR_VER, ADUC_V2_CONTRACT_MINOR_VER });

    ADUC_Result result = ExtensionManager::InitializeContentDownloader(nullptr, ADUC_LOG_DEBUG);

    CHECK(IsAducResultCodeFailure(result.ResultCode));
    // Must NOT be the unsupported contract version error — proves V2 path was taken.
    CHECK(result.ExtendedResultCode != ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION);
    // Should be INITIALIZEPROC_NOTIMP since the test binary doesn't export "Initialize".
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZEPROC_NOTIMP);

    ExtensionManager::SetContentDownloaderLibrary(nullptr);
    dlclose(safeHandle);
}
