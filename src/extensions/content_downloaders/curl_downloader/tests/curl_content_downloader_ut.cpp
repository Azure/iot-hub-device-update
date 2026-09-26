/**
 * @file curl_content_downloader_ut.cpp
 * @brief Unit Tests for curl_content_downloader and EXPORTS functions.
 *
 * Covers input-validation paths, file-already-exists path, progress
 * callback paths, and the EXPORTS wrapper functions (Initialize,
 * GetContractInfo, Download). Tests use real dependencies only.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include "curl_content_downloader.h"
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
    ADUC_Result result = Download_curl(
        nullptr /* entity */,
        "wf-1" /* workflowId */,
        "/tmp" /* workFolder */,
        60 /* timeoutInSeconds */,
        nullptr /* downloadProgressCallback */);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY);
}

TEST_CASE("Download_curl returns failure when entity DownloadUri is nullptr")
{
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
    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("");
    entity.TargetFilename = const_cast<char*>("file.bin");

    ADUC_Result result = Download_curl(
        &entity, "wf-3", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI);
}


// =====================================================================
// Section 2: File-already-exists with valid hash → skip download
// =====================================================================


TEST_CASE("Download_curl returns external-failure when curl command fails")
{
    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("file:///tmp/curl_ut_does_not_exist.bin");
    entity.TargetFilename = const_cast<char*>("missing.bin");
    entity.FileId = const_cast<char*>("fid-curl-fail");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = 1;

    ADUC_Result result = Download_curl(
        &entity, "wf-curl-fail", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE("Download_curl reports error progress when curl command fails")
{
    CallbackRecord::Reset();

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>("file:///tmp/curl_ut_does_not_exist_cb.bin");
    entity.TargetFilename = const_cast<char*>("missing_cb.bin");
    entity.FileId = const_cast<char*>("fid-curl-fail-cb");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = 123;

    ADUC_Result result = Download_curl(
        &entity, "wf-curl-fail-cb", "/tmp", 60, RecordingCallback);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode != 0);
    CHECK(CallbackRecord::Instance().invoked);
    CHECK(CallbackRecord::Instance().state == ADUC_DownloadProgressState_Error);
    CHECK(CallbackRecord::Instance().bytesTransferred == 0);
    CHECK(CallbackRecord::Instance().bytesTotal == 123);
}

TEST_CASE("Download_curl succeeds for local file URI with valid hash")
{
    std::string srcDir = MakeTempDir("srcok");
    std::string dstDir = MakeTempDir("dstok");
    REQUIRE_FALSE(srcDir.empty());
    REQUIRE_FALSE(dstDir.empty());

    const std::string sourcePath = srcDir + "/src.bin";
    const std::string content = "curl_local_file_success_payload";
    std::string hashBase64 = CreateFileAndGetHash(sourcePath, content);
    REQUIRE_FALSE(hashBase64.empty());

    ADUC_Hash hash;
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>(hashBase64.c_str());

    std::string uri = "file://" + sourcePath;

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.DownloadUri = const_cast<char*>(uri.c_str());
    entity.TargetFilename = const_cast<char*>("downloaded.bin");
    entity.FileId = const_cast<char*>("fid-local-success");
    entity.HashCount = 1;
    entity.Hash = &hash;
    entity.SizeInBytes = static_cast<unsigned int>(content.size());

    ADUC_Result result = Download_curl(&entity, "wf-local-success", dstDir.c_str(), 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Success);
    CHECK(result.ExtendedResultCode == 0);

    remove(sourcePath.c_str());
    remove((dstDir + "/downloaded.bin").c_str());
    rmdir(srcDir.c_str());
    rmdir(dstDir.c_str());
}


// =====================================================================
// EXPORTS functions
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

TEST_CASE("GetContractInfo populates version 2.0 and returns success")
{
    ADUC_ExtensionContractInfo contractInfo{};
    contractInfo.majorVer = 99;
    contractInfo.minorVer = 99;

    ADUC_Result result = GetContractInfo(&contractInfo);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(contractInfo.majorVer == ADUC_V2_CONTRACT_MAJOR_VER);
    CHECK(contractInfo.minorVer == ADUC_V2_CONTRACT_MINOR_VER);
}

TEST_CASE("Download EXPORT delegates to Download_curl (nullptr entity)")
{
    // The EXPORT Download() just calls Download_curl(), so it should
    // give the same result for nullptr entity.
    ADUC_Result result = Download(
        nullptr, "wf-export", "/tmp", 60, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY);
}
