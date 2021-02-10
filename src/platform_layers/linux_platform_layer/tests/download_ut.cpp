/**
 * @file download_ut.cpp
 * @brief Unit tests for download functionality implemented in deviceUpdateReferenceImpl library.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <catch2/catch.hpp>
#include <cstring>

#include "mock_do_download.hpp"
#include "mock_do_download_status.hpp"
#include "mock_do_exceptions.hpp"
namespace msdo = microsoft::deliveryoptimization;

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

    if (result.ResultCode != ADUC_DownloadResult_InProgress)
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
 * @brief Initialize ADUC_DownloadInfo data.
 */
static void InitDownloadInfo(
    ADUC_DownloadInfo* downloadInfo,
    ADUC_DownloadProgressCallback downloadProgressCallback,
    int fileCount,
    ADUC_FileEntity* files)
{
    downloadInfo->NotifyDownloadProgress = downloadProgressCallback;
    downloadInfo->FileCount = fileCount;
    downloadInfo->Files = files;
}

/**
 * @brief Test content download scenarios.
 */
TEST_CASE("Download content")
{
    REQUIRE(pthread_mutex_init(&downloadMutex, nullptr) == 0);
    REQUIRE(pthread_cond_init(&downloadCompletedCond, nullptr) == 0);

    char updateType[] = "microsoft/apt:1";
    const size_t MAX_WORKFLOWID{ 13 };
    char workflowId[MAX_WORKFLOWID];

    ADUC_WorkCompletionData workflowCompletionData = { mockWorkCompletionCallback };

    GenerateUniqueId(workflowId, sizeof(workflowId) / sizeof(workflowId[0]));

    // Setup test
    ADUC_RegisterData registerData = {};
    ADUC_DownloadInfo downloadInfo = {};

    // Register
    ADUC_Result result = ADUC_Register(&registerData, 0, nullptr);
    bool isRegistered = IsAducResultCodeSuccess(result.ResultCode);
    REQUIRE(isRegistered);

    // verify register data
    REQUIRE(registerData.IdleCallback != nullptr);
    REQUIRE(registerData.DownloadCallback != nullptr);
    REQUIRE(registerData.InstallCallback != nullptr);
    REQUIRE(registerData.ApplyCallback != nullptr);
    REQUIRE(registerData.IsInstalledCallback != nullptr);

// TODO(shiyipeng): Bug 28605123: Download UT's require DO to download for hash checks
#if 0
    SECTION("Positive: download success.")
    {
        InitDownloadInfo(&downloadInfo, mockDownloadProgressCallback, 1, fileEntityWithGoodHash);

        // Normal behavior
        msdo::download::set_mock_download_behavior(msdo::mock_download_behavior::Normal);

        // Create sandbox
        ADUC_Result sandboxCreateResult = registerData.SandboxCreateCallback(registerData.Token, workflowId, &downloadInfo.WorkFolder);
        REQUIRE(sandboxCreateResult.ResultCode == ADUC_SandboxCreateResult_Success);

        // Start download
        ADUC_Result downloadResult = registerData.DownloadCallback(
            registerData.Token, workflowId, updateType, &workflowCompletionData, &downloadInfo);
        REQUIRE(downloadResult.ResultCode == ADUC_DownloadResult_InProgress);

        // Wait until download is done (either completed or failed);
        pthread_cond_wait(&downloadCompletedCond, &downloadMutex);

        // Verify expected results.
        INFO("state: " << downloadProgressInfo.state);
        INFO("ResultCode: " << downloadTestResult.ResultCode);
        INFO("extendedResultCode: " << std::hex << downloadTestResult.ExtendedResultCode);
        REQUIRE(downloadProgressInfo.state == ADUC_DownloadProgressState_Completed);
        REQUIRE(downloadTestResult.ResultCode == ADUC_DownloadResult_Success);
        REQUIRE(downloadTestResult.ExtendedResultCode == 0);

        // Destroy sandbox
        registerData.SandboxDestroyCallback(registerData.Token, workflowId, downloadInfo.WorkFolder);
    }
#endif

// TODO(shiyipeng): Bug 28605123: Download UT's require DO to download for hash checks
#if 0
    SECTION("Negative: invalid file hash.")
    {
        InitDownloadInfo(&downloadInfo, mockDownloadProgressCallback, 1, fileEntityWithBadHash);

        // Normal behavior
        msdo::download::set_mock_download_behavior(msdo::mock_download_behavior::Normal);

        // Create sandbox
        ADUC_Result sandboxCreateResult = registerData.SandboxCreateCallback(registerData.Token, workflowId, &downloadInfo.WorkFolder);
        REQUIRE(sandboxCreateResult.ResultCode == ADUC_SandboxCreateResult_Success);

        // Start download
        ADUC_Result downloadResult = registerData.DownloadCallback(
            registerData.Token, workflowId, updateType, &workflowCompletionData, &downloadInfo);
        REQUIRE(downloadResult.ResultCode == ADUC_DownloadResult_InProgress);

        // Wait until download is done (either completed or failed);
        pthread_cond_wait(&downloadCompletedCond, &downloadMutex);

        // Verify expected results.
        INFO("state: " << downloadProgressInfo.state);
        INFO("resultCode: " << downloadTestResult.ResultCode);
        INFO("extendedResultCode: " << std::hex << downloadTestResult.ExtendedResultCode);
        REQUIRE(downloadProgressInfo.state == ADUC_DownloadProgressState_Error);
        REQUIRE(downloadTestResult.ResultCode == ADUC_DownloadResult_Failure);
        REQUIRE(downloadTestResult.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH);

        // Destroy sandbox
        registerData.SandboxDestroyCallback(registerData.Token, workflowId, downloadInfo.WorkFolder);
    }
#endif

// TODO(shiyipeng): Bug 28605123: Download UT's require DO to download for hash checks
#if 0
    SECTION("Negative: force abort")
    {
        InitDownloadInfo(&downloadInfo, mockDownloadProgressCallback, 1, fileEntityWithGoodHash);

        // Emulate abort action.
        msdo::download::set_mock_download_behavior(msdo::mock_download_behavior::Aborted);

        // Create sandbox
        // Create sandbox
        ADUC_Result sandboxCreateResult =
            registerData.SandboxCreateCallback(registerData.Token, workflowId, &downloadInfo.WorkFolder);
        REQUIRE(sandboxCreateResult.ResultCode == ADUC_SandboxCreateResult_Success);

        // Start download
        ADUC_Result downloadResult = registerData.DownloadCallback(
            registerData.Token, workflowId, updateType, &workflowCompletionData, &downloadInfo);
        REQUIRE(downloadResult.ResultCode == ADUC_DownloadResult_InProgress);

        // Wait until download is done (either completed or failed);
        pthread_cond_wait(&downloadCompletedCond, &downloadMutex);

        // Verify expected results.
        INFO("state: " << downloadProgressInfo.state);
        INFO("resultCode: " << downloadTestResult.ResultCode);
        INFO("extendedResultCode: " << std::hex << downloadTestResult.ExtendedResultCode);
        REQUIRE(downloadProgressInfo.state == ADUC_DownloadProgressState_Cancelled);
        REQUIRE(downloadTestResult.ResultCode == ADUC_DownloadResult_Cancelled);
        REQUIRE(downloadTestResult.ExtendedResultCode == MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(0x7D));

        // Destroy sandbox
        registerData.SandboxDestroyCallback(registerData.Token, workflowId, downloadInfo.WorkFolder);
    }
#endif

// TODO(shiyipeng): Bug 28605123: Download UT's require DO to download for hash checks
#if 0
    SECTION("Negative: unsupported file hash.")
    {
        InitDownloadInfo(&downloadInfo, mockDownloadProgressCallback, 1, fileEntityWithUnsupportedHash);

        // Normal behavior
        msdo::download::set_mock_download_behavior(msdo::mock_download_behavior::Normal);

        // Create sandbox
        ADUC_Result sandboxCreateResult =
            registerData.SandboxCreateCallback(registerData.Token, workflowId, &downloadInfo.WorkFolder);
        REQUIRE(sandboxCreateResult.ResultCode == ADUC_SandboxCreateResult_Success);

        // Start download
        ADUC_Result downloadResult = registerData.DownloadCallback(
            registerData.Token, workflowId, updateType, &workflowCompletionData, &downloadInfo);
        REQUIRE(downloadResult.ResultCode == ADUC_DownloadResult_InProgress);

        // Wait until download is done (either completed or failed);
        pthread_cond_wait(&downloadCompletedCond, &downloadMutex);

        // Verify expected results.
        INFO("state: " << downloadProgressInfo.state);
        INFO("resultCode: " << downloadTestResult.ResultCode);
        INFO("extendedResultCode: " << std::hex << downloadTestResult.ExtendedResultCode);
        REQUIRE(downloadProgressInfo.state == ADUC_DownloadProgressState_Error);
        REQUIRE(downloadTestResult.ResultCode == ADUC_DownloadResult_Failure);
        REQUIRE(downloadTestResult.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED);

        // Destroy sandbox
        registerData.SandboxDestroyCallback(registerData.Token, workflowId, downloadInfo.WorkFolder);
    }
#endif

#if 0
    SECTION("Negative: empty file hash.")
    {
        InitDownloadInfo(&downloadInfo, mockDownloadProgressCallback, 1, fileEntityWithEmptyHash);

        // Normal behavior
        msdo::download::set_mock_download_behavior(msdo::mock_download_behavior::Normal);

        // Create sandbox
        ADUC_Result sandboxCreateResult =
            registerData.SandboxCreateCallback(registerData.Token, workflowId, &downloadInfo.WorkFolder);
        REQUIRE(sandboxCreateResult.ResultCode == ADUC_SandboxCreateResult_Success);

        // Start download
        ADUC_Result downloadResult = registerData.DownloadCallback(
            registerData.Token, workflowId, updateType, &workflowCompletionData, &downloadInfo);
        REQUIRE(downloadResult.ResultCode == ADUC_DownloadResult_InProgress);

        // Wait until download is done (either completed or failed);
        pthread_cond_wait(&downloadCompletedCond, &downloadMutex);

        // Verify expected results.
        INFO("state: " << downloadProgressInfo.state);
        INFO("resultCode: " << downloadTestResult.ResultCode);
        INFO("extendedResultCode: " << std::hex << downloadTestResult.ExtendedResultCode);
        REQUIRE(downloadProgressInfo.state == ADUC_DownloadProgressState_Error);
        REQUIRE(downloadTestResult.ResultCode == ADUC_DownloadResult_Failure);
        REQUIRE(downloadTestResult.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY);

        // Destroy sandbox
        registerData.SandboxDestroyCallback(registerData.Token, workflowId, downloadInfo.WorkFolder);
    }
#endif

    // Clean up
    if (isRegistered)
    {
        ADUC_Unregister(registerData.Token);
    }
}
