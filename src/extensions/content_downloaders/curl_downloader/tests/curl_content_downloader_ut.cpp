/**
 * @file curl_content_downloader_ut.cpp
 * @brief Unit Tests for curl_content_downloader and EXPORTS functions.
 *
 * Covers input-validation paths, download execution paths (via link-time
 * mock of ADUC_LaunchChildProcess), progress callback paths, and the
 * EXPORTS wrapper functions (Initialize, GetContractInfo, Download).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include "curl_content_downloader.h"
#include "mock_process_utils.h"
#include <aduc/contract_utils.h>
#include <aduc/hash_utils.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <aduc/types/download.h>
#include <aduc/types/update_content.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// =====================================================================
// EXPORTS function declarations (extern "C" since EXPORTS.cpp uses
// EXTERN_C_BEGIN / EXTERN_C_END).
// =====================================================================
extern "C"
{
    ADUC_Result Download(
        const ADUC_FileEntity* entity,
        const char* workflowId,
        const char* workFolder,
        unsigned int timeoutInSeconds,
        ADUC_DownloadProgressCallback downloadProgressCallback);

    ADUC_Result Initialize(const char* initializeData);

    ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

// =====================================================================
// Helpers
// =====================================================================

// Captures progress callback invocations.
struct CallbackRecord
{
    bool invoked = false;
    ADUC_DownloadProgressState state = ADUC_DownloadProgressState_Error;
    uint64_t bytesTransferred = 0;
    uint64_t bytesTotal = 0;

    static void Reset()
    {
        Instance() = {};
    }

    static CallbackRecord& Instance()
    {
        static CallbackRecord s_record;
        return s_record;
    }
};

static void RecordingCallback(
    const char* /*workflowId*/,
    const char* /*fileId*/,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal)
{
    auto& r = CallbackRecord::Instance();
    r.invoked = true;
    r.state = state;
    r.bytesTransferred = bytesTransferred;
    r.bytesTotal = bytesTotal;
}

/// Write content to a file and return its SHA-256 hash (base64).
static std::string CreateFileAndGetHash(const std::string& path, const std::string& content)
{
    {
        std::ofstream ofs(path, std::ios::binary);
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    char* hashBase64 = nullptr;
    bool ok = ADUC_HashUtils_GetFileHash(path.c_str(), SHA256, &hashBase64);
    std::string hashStr;
    if (ok && hashBase64 != nullptr)
    {
        hashStr = hashBase64;
        free(hashBase64);
    }
    return hashStr;
}

/// Helper to build a temp directory path under /tmp.
static std::string MakeTempDir(const char* label)
{
    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "/tmp/curl_ut_%s_XXXXXX", label);
    char* dir = mkdtemp(tmpl);
    return (dir != nullptr) ? std::string(dir) : std::string{};
}

// =====================================================================
// Section 1: Download_curl null / bad-arg validation tests
// =====================================================================

TEST_CASE("Download_curl returns failure when entity is nullptr")
{
    MockLaunchChildProcess_Reset();

    ADUC_Result result = Download_curl(
        nullptr /* entity */,
        "wf-1" /* workflowId */,
        "/tmp" /* workFolder */,
        60 /* timeoutInSeconds */,
        nullptr /* downloadProgressCallback */);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY);
    CHECK_FALSE(MockLaunchChildProcess_WasCalled());
}

TEST_CASE("Download_curl returns failure when entity DownloadUri is nullptr")
{
    MockLaunchChildProcess_Reset();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = nullptr;
    entity.TargetFilename = const_cast<char*>("file.bin");

    ADUC_Result result = Download_curl(
        &entity, "wf-2", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI);
}

TEST_CASE("Download_curl returns failure when entity DownloadUri is empty string")
{
    MockLaunchChildProcess_Reset();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("");
    entity.TargetFilename = const_cast<char*>("file.bin");

    ADUC_Result result = Download_curl(
        &entity, "wf-3", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI);
}

TEST_CASE("Download_curl returns failure when HashCount is zero, no callback")
{
    MockLaunchChildProcess_Reset();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/file.bin");
    entity.TargetFilename = const_cast<char*>("file.bin");
    entity.FileId = const_cast<char*>("fid-1");
    entity.HashCount = 0;
    entity.Hash = nullptr;

    ADUC_Result result = Download_curl(
        &entity, "wf-4", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY);
}

TEST_CASE("Download_curl calls progress callback on HashCount-zero error")
{
    MockLaunchChildProcess_Reset();
    CallbackRecord::Reset();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/file.bin");
    entity.TargetFilename = const_cast<char*>("file.bin");
    entity.FileId = const_cast<char*>("fid-2");
    entity.HashCount = 0;
    entity.Hash = nullptr;

    ADUC_Result result = Download_curl(
        &entity, "wf-5", "/tmp", 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY);
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Error);
}

TEST_CASE("Download_curl returns failure with unsupported hash type, no callback")
{
    MockLaunchChildProcess_Reset();

    ADUC_Hash hash;
    hash.type = const_cast<char*>("unsupported_hash_algo_xyz");
    hash.value = const_cast<char*>("abc123");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/file.bin");
    entity.TargetFilename = const_cast<char*>("file.bin");
    entity.FileId = const_cast<char*>("fid-3");
    entity.HashCount = 1;
    entity.Hash = &hash;

    ADUC_Result result = Download_curl(
        &entity, "wf-6", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED);
    CHECK_FALSE(MockLaunchChildProcess_WasCalled());
}

// =====================================================================
// Section 2: Unsupported hash type WITH progress callback
// =====================================================================

TEST_CASE("Download_curl calls progress callback on unsupported hash type")
{
    MockLaunchChildProcess_Reset();
    CallbackRecord::Reset();

    ADUC_Hash hash;
    hash.type = const_cast<char*>("bad_algo");
    hash.value = const_cast<char*>("deadbeef");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/f.bin");
    entity.TargetFilename = const_cast<char*>("f.bin");
    entity.FileId = const_cast<char*>("fid-7");
    entity.HashCount = 1;
    entity.Hash = &hash;

    ADUC_Result result = Download_curl(
        &entity, "wf-7", "/tmp", 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED);
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Error);
}

// =====================================================================
// Section 3: File-already-exists with valid hash → skip download
// =====================================================================

TEST_CASE("Download_curl skips download when file already exists with valid hash")
{
    MockLaunchChildProcess_Reset();

    std::string tmpDir = MakeTempDir("skip");
    REQUIRE_FALSE(tmpDir.empty());

    // Create a file with known content and get its hash.
    const std::string fileContent = "hello_world_test_content";
    const std::string filePath = tmpDir + "/existing.bin";
    std::string hashBase64 = CreateFileAndGetHash(filePath, fileContent);
    REQUIRE_FALSE(hashBase64.empty());

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>(hashBase64.c_str());

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/existing.bin");
    entity.TargetFilename = const_cast<char*>("existing.bin");
    entity.FileId = const_cast<char*>("fid-skip");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(fileContent.size());

    ADUC_Result result = Download_curl(
        &entity, "wf-skip", tmpDir.c_str(), 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Skipped_FileExists);
    CHECK_FALSE(MockLaunchChildProcess_WasCalled());

    // Cleanup
    remove(filePath.c_str());
    rmdir(tmpDir.c_str());
}

TEST_CASE("Download_curl skip-download reports progress on success callback")
{
    MockLaunchChildProcess_Reset();
    CallbackRecord::Reset();

    std::string tmpDir = MakeTempDir("skipCb");
    REQUIRE_FALSE(tmpDir.empty());

    const std::string fileContent = "callback_test_content_data";
    const std::string filePath = tmpDir + "/cb.bin";
    std::string hashBase64 = CreateFileAndGetHash(filePath, fileContent);
    REQUIRE_FALSE(hashBase64.empty());

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>(hashBase64.c_str());

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/cb.bin");
    entity.TargetFilename = const_cast<char*>("cb.bin");
    entity.FileId = const_cast<char*>("fid-skipcb");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(fileContent.size());

    ADUC_Result result = Download_curl(
        &entity, "wf-skipcb", tmpDir.c_str(), 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Download_Skipped_FileExists);
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Completed);

    // The callback should receive the actual file size from stat()
    struct stat st;
    REQUIRE(stat(filePath.c_str(), &st) == 0);
    CHECK(CallbackRecord::Instance().bytesTransferred == static_cast<uint64_t>(st.st_size));
    CHECK(CallbackRecord::Instance().bytesTotal == fileContent.size());

    remove(filePath.c_str());
    rmdir(tmpDir.c_str());
}

// =====================================================================
// Section 4: Successful download (mock returns 0) with valid hash
// =====================================================================

TEST_CASE("Download_curl succeeds when curl exits 0 and hash is valid")
{
    MockLaunchChildProcess_Reset();

    std::string tmpDir = MakeTempDir("dlok");
    REQUIRE_FALSE(tmpDir.empty());

    const std::string fileContent = "downloaded_content_ok";
    const std::string filePath = tmpDir + "/dl.bin";

    // Pre-compute hash for the content the mock will "download".
    std::string hashBase64 = CreateFileAndGetHash(filePath, fileContent);
    REQUIRE_FALSE(hashBase64.empty());

    // Remove the file so Download_curl doesn't skip download.
    remove(filePath.c_str());

    // Configure mock: curl exits 0 and creates the file.
    MockLaunchChildProcess_SetExitCode(0);
    MockLaunchChildProcess_SetFileToCreate(filePath, fileContent);

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>(hashBase64.c_str());

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/dl.bin");
    entity.TargetFilename = const_cast<char*>("dl.bin");
    entity.FileId = const_cast<char*>("fid-dl");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(fileContent.size());

    ADUC_Result result = Download_curl(
        &entity, "wf-dl", tmpDir.c_str(), 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(MockLaunchChildProcess_WasCalled());
    CHECK(MockLaunchChildProcess_GetCommand() == "/usr/bin/curl");

    // Verify curl args contain expected flags.
    auto args = MockLaunchChildProcess_GetArgs();
    REQUIRE(args.size() >= 6);
    CHECK(args[0] == "-L");
    CHECK(args[1] == "-C");
    CHECK(args[2] == "-");
    CHECK(args[3] == "-o");
    CHECK(args[4] == filePath);
    CHECK(args[5] == "http://example.com/dl.bin");

    remove(filePath.c_str());
    rmdir(tmpDir.c_str());
}

// =====================================================================
// Section 5: Failed download (curl exits non-zero)
// =====================================================================

TEST_CASE("Download_curl fails when curl exits non-zero")
{
    MockLaunchChildProcess_Reset();
    CallbackRecord::Reset();

    std::string tmpDir = MakeTempDir("dlfail");
    REQUIRE_FALSE(tmpDir.empty());

    // Configure mock: curl exits 7 (could not connect).
    MockLaunchChildProcess_SetExitCode(7);

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("dGVzdA=="); // doesn't matter, won't be checked

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/fail.bin");
    entity.TargetFilename = const_cast<char*>("fail.bin");
    entity.FileId = const_cast<char*>("fid-fail");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = 100;

    ADUC_Result result = Download_curl(
        &entity, "wf-fail", tmpDir.c_str(), 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(7));
    CHECK(MockLaunchChildProcess_WasCalled());

    // Progress callback should be invoked with Error state.
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Error);

    rmdir(tmpDir.c_str());
}

TEST_CASE("Download_curl fails when curl exits non-zero, no callback")
{
    MockLaunchChildProcess_Reset();

    std::string tmpDir = MakeTempDir("dlfailnc");
    REQUIRE_FALSE(tmpDir.empty());

    MockLaunchChildProcess_SetExitCode(28); // timeout

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("dGVzdA==");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/fail2.bin");
    entity.TargetFilename = const_cast<char*>("fail2.bin");
    entity.FileId = const_cast<char*>("fid-fail2");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = 100;

    ADUC_Result result = Download_curl(
        &entity, "wf-fail2", tmpDir.c_str(), 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(28));

    rmdir(tmpDir.c_str());
}

// =====================================================================
// Section 6: Successful download but hash mismatch
// =====================================================================

TEST_CASE("Download_curl fails when post-download hash is invalid")
{
    MockLaunchChildProcess_Reset();
    CallbackRecord::Reset();

    std::string tmpDir = MakeTempDir("dlhash");
    REQUIRE_FALSE(tmpDir.empty());

    const std::string fileContent = "downloaded_content_mismatch";
    const std::string filePath = tmpDir + "/mismatch.bin";

    // Mock creates the file on "download" but we give a wrong hash.
    MockLaunchChildProcess_SetExitCode(0);
    MockLaunchChildProcess_SetFileToCreate(filePath, fileContent);

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    // Provide a completely wrong hash value (valid base64 but won't match).
    hash.value = const_cast<char*>("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/mismatch.bin");
    entity.TargetFilename = const_cast<char*>("mismatch.bin");
    entity.FileId = const_cast<char*>("fid-mismatch");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(fileContent.size());

    ADUC_Result result = Download_curl(
        &entity, "wf-mismatch", tmpDir.c_str(), 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH);
    CHECK(MockLaunchChildProcess_WasCalled());

    // reportProgress should fire with Error state.
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Error);

    remove(filePath.c_str());
    rmdir(tmpDir.c_str());
}

TEST_CASE("Download_curl post-download hash invalid, no callback")
{
    MockLaunchChildProcess_Reset();

    std::string tmpDir = MakeTempDir("dlhashnc");
    REQUIRE_FALSE(tmpDir.empty());

    const std::string fileContent = "wrong_hash_no_callback";
    const std::string filePath = tmpDir + "/nochash.bin";

    MockLaunchChildProcess_SetExitCode(0);
    MockLaunchChildProcess_SetFileToCreate(filePath, fileContent);

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("http://example.com/nochash.bin");
    entity.TargetFilename = const_cast<char*>("nochash.bin");
    entity.FileId = const_cast<char*>("fid-nochash");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(fileContent.size());

    ADUC_Result result = Download_curl(
        &entity, "wf-nochash", tmpDir.c_str(), 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH);

    remove(filePath.c_str());
    rmdir(tmpDir.c_str());
}

// =====================================================================
// Section 7: EXPORTS functions
// =====================================================================

TEST_CASE("Initialize returns success")
{
    ADUC_Result result = Initialize(nullptr);
    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
}

TEST_CASE("Initialize returns success with non-null data")
{
    ADUC_Result result = Initialize("some-init-data");
    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
}

TEST_CASE("GetContractInfo populates version 1.0 and returns success")
{
    ADUC_ExtensionContractInfo contractInfo{};
    contractInfo.majorVer = 99;
    contractInfo.minorVer = 99;

    ADUC_Result result = GetContractInfo(&contractInfo);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("Download EXPORT delegates to Download_curl (nullptr entity)")
{
    MockLaunchChildProcess_Reset();

    // The EXPORT Download() just calls Download_curl(), so it should
    // give the same result for nullptr entity.
    ADUC_Result result = Download(
        nullptr, "wf-export", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY);
}
