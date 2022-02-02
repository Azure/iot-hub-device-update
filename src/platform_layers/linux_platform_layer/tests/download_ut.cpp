/**
 * @file download_ut.cpp
 * @brief Unit tests for download functionality implemented in deviceUpdateReferenceImpl library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
#include <cstring>

#include "mock_do_download.hpp"
#include "mock_do_download_status.hpp"
#include "mock_do_exceptions.hpp"

#include <aduc/adu_core_exports.h>
#include <aduc/system_utils.h>

//
// Test helper data.
//
class DownloadProgressInfo
{
public:
    const char* workflowId;
    const char* fileId;
    ADUC_DownloadProgressState state;
    uint64_t bytesTransferred;
    uint64_t bytesTotal;
};

static DownloadProgressInfo downloadProgressInfo;
static pthread_cond_t downloadCompletedCond;
static pthread_mutex_t downloadMutex;
static ADUC_Result downloadTestResult;

/**
 * @brief Mock for workflow idle callback. No-op at the moment.
 */
static void mockIdleCallback(ADUC_Token /*token*/, const char* /*workflowId*/)
{
}

/**
 * @brief Mock for download progress callback. Cache download progress info for test verifications.
 */
static void mockDownloadProgressCallback(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal)
{
    downloadProgressInfo.workflowId = workflowId;
    downloadProgressInfo.fileId = fileId;
    downloadProgressInfo.state = state;
    downloadProgressInfo.bytesTransferred = bytesTransferred;
    downloadProgressInfo.bytesTotal = bytesTotal;
}

/**
 * @brief Mock for completion callback.
 */
static void mockWorkCompletionCallback(const void* /*workCompletionToken*/, ADUC_Result result)
{
    // Save result for test validation.
    downloadTestResult = result;

    if (result.ResultCode != ADUC_Result_Download_InProgress)
    {
        pthread_cond_signal(&downloadCompletedCond);
    }
}

static const char* testUrl =
    "http://download.windowsupdate.com/c/msdownload/update/software/secu/2020/07/windows6.0-kb4575904-x64_9272724f637d85a12bfe022191c1ba56cd4bc59e.msu";
// openssl dgst -binary -sha256 < windows6.0-kb4575904-x64_9272724f637d85a12bfe022191c1ba56cd4bc59e.msu  | openssl base64
static const char* testUrlSha256Hash = "d05sDON3NP+Xc8+HMRLiGWJ944KXLvqZYpYMkUNn/TI=";

ADUC_Hash goodHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>(testUrlSha256Hash), // hash --
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("sha256"), // type
} };

ADUC_Hash badHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuMW="), // hash
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("sha256"), // type
} };

ADUC_Hash unsupportedHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU="), // hash
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("sha1024"), // type
} };

ADUC_Hash emptyHash[] = {};

ADUC_FileEntity fileEntityWithGoodHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("good_hash"), // TargetFilename
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>(testUrl), // DownloadUri
    goodHash, // Hash
    1, // HashCount
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("0") // FileId
} };

ADUC_FileEntity fileEntityWithBadHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("bad_hash"), // TargetFilename
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>(testUrl), // DownloadUri
    badHash, // Hash
    1, // HashCount
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("1") // FileId
} };

// 2020-07 Extended Security Updates (ESU) Licensing Preparation Package for Windows Server 2008 for x64-based Systems (KB4575904)
ADUC_FileEntity fileEntityWithUnsupportedHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("bad_hash"), // TargetFilename
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>(testUrl), // DownloadUri
    unsupportedHash, // Hash
    1, // HashCount
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("1") // FileId
} };

// 2020-07 Extended Security Updates (ESU) Licensing Preparation Package for Windows Server 2008 for x64-based Systems (KB4575904)
ADUC_FileEntity fileEntityWithEmptyHash[] = { {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("bad_hash"), // TargetFilename
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>(testUrl), // DownloadUri
    emptyHash, // Hash
    0, // HashCount
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    const_cast<char*>("1") // FileId
} };
/**
 * @brief Generate a unique identifier.
 *
 * @param buffer Where to store identifier.
 * @param buffer_cch Number of characters in @p buffer.
 */
static void GenerateUniqueId(char* buffer, size_t buffer_cch)
{
    const time_t timer = time(nullptr);
    struct tm tm
    {
    };
    (void)gmtime_r(&timer, &tm);
    (void)strftime(buffer, buffer_cch, "%y%m%d%H%M%S", &tm);
}

/**
 * @brief Test content download scenarios.
 */
TEST_CASE("Download content")
{
    REQUIRE(pthread_mutex_init(&downloadMutex, nullptr) == 0);
    REQUIRE(pthread_cond_init(&downloadCompletedCond, nullptr) == 0);

    const size_t MAX_WORKFLOWID{ 13 };
    char workflowId[MAX_WORKFLOWID];

    GenerateUniqueId(workflowId, sizeof(workflowId) / sizeof(workflowId[0]));

    // Setup test
    ADUC_UpdateActionCallbacks updateActionCallbacks = {};

    // Register
    ADUC_Result result = ADUC_RegisterPlatformLayer(&updateActionCallbacks, 0, nullptr);
    bool isRegistered = IsAducResultCodeSuccess(result.ResultCode);
    REQUIRE(isRegistered);

    // verify register data
    REQUIRE(updateActionCallbacks.IdleCallback != nullptr);
    REQUIRE(updateActionCallbacks.DownloadCallback != nullptr);
    REQUIRE(updateActionCallbacks.InstallCallback != nullptr);
    REQUIRE(updateActionCallbacks.ApplyCallback != nullptr);
    REQUIRE(updateActionCallbacks.CancelCallback != nullptr);
    REQUIRE(updateActionCallbacks.IsInstalledCallback != nullptr);

    // Clean up
    if (isRegistered)
    {
        ADUC_Unregister(updateActionCallbacks.PlatformLayerHandle);
    }
}
