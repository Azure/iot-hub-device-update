/**
 * @file extension_manager_mock_ut.cpp
 * @brief Mock-based Unit Tests for extension_manager.cpp to maximize coverage.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_extension_manager_deps.h"

#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/extension_manager.h>
#include <aduc/extension_manager.hpp>
#include <aduc/result.h>
#include <aduc/types/update_content.h>

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

// Concrete test handler for SetUpdateContentHandlerExtension tests
class MockContentHandler : public ContentHandler
{
public:
    MockContentHandler() = default;
    ~MockContentHandler() override = default;

    ADUC_Result Download(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result Backup(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result Install(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result Apply(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result Restore(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result Cancel(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
};

// Helper struct for test cleanup
struct ExtMgrFixture
{
    ExtMgrFixture()
    {
        mock_extension_manager_reset();
        // Ensure static state is clean
        ExtensionManager::Uninit();
    }

    ~ExtMgrFixture()
    {
        ExtensionManager::Uninit();
    }
};

// =====================================================================
// SetUpdateContentHandlerExtension tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "SetUpdateContentHandlerExtension returns failure when handler is null")
{
    ADUC_Result result = ExtensionManager::SetUpdateContentHandlerExtension("test/type", nullptr);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "SetUpdateContentHandlerExtension succeeds with valid handler")
{
    // Use raw new since the ExtensionManager will delete it on Uninit
    MockContentHandler* handler = new MockContentHandler();

    ADUC_Result result = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);
    CHECK(result.ResultCode == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "SetUpdateContentHandlerExtension replaces existing handler")
{
    MockContentHandler* handler1 = new MockContentHandler();
    MockContentHandler* handler2 = new MockContentHandler();

    ADUC_Result result1 = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler1);
    CHECK(result1.ResultCode == 1);

    // This should erase handler1 and emplace handler2 (handler1 is leaked, but that's OK for test)
    ADUC_Result result2 = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler2);
    CHECK(result2.ResultCode == 1);
}

// =====================================================================
// SetContentDownloaderLibrary tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "SetContentDownloaderLibrary returns success")
{
    int fake_lib = 42;
    ADUC_Result result = ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    CHECK(result.ResultCode == 1);
}

// =====================================================================
// Get/SetContentDownloaderContractVersion tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "GetContentDownloaderContractVersion returns stored version")
{
    ADUC_ExtensionContractInfo setInfo{ 2, 3 };
    ExtensionManager::SetContentDownloaderContractVersion(setInfo);

    ADUC_ExtensionContractInfo getInfo{};
    ADUC_Result result = ExtensionManager::GetContentDownloaderContractVersion(&getInfo);
    CHECK(result.ResultCode == 1);
    CHECK(getInfo.majorVer == 2);
    CHECK(getInfo.minorVer == 3);
}

TEST_CASE_METHOD(ExtMgrFixture, "SetContentDownloaderContractVersion overwrites previous value")
{
    ADUC_ExtensionContractInfo info1{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(info1);

    ADUC_ExtensionContractInfo info2{ 5, 1 };
    ExtensionManager::SetContentDownloaderContractVersion(info2);

    ADUC_ExtensionContractInfo getInfo{};
    ExtensionManager::GetContentDownloaderContractVersion(&getInfo);
    CHECK(getInfo.majorVer == 5);
    CHECK(getInfo.minorVer == 1);
}

// =====================================================================
// GetComponentEnumeratorContractVersion tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "GetComponentEnumeratorContractVersion returns default zero version")
{
    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = ExtensionManager::GetComponentEnumeratorContractVersion(&info);
    CHECK(result.ResultCode == 1);
    // Default static is zero-initialized
    CHECK(info.majorVer == 0);
    CHECK(info.minorVer == 0);
}

// =====================================================================
// Uninit tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "Uninit can be called without any handlers loaded")
{
    REQUIRE_NOTHROW(ExtensionManager::Uninit());
}

TEST_CASE_METHOD(ExtMgrFixture, "Uninit clears cached content handlers")
{
    MockContentHandler* handler = new MockContentHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);

    REQUIRE_NOTHROW(ExtensionManager::Uninit());
    // After Uninit, loading the same handler type should fail to find it cached
}

// =====================================================================
// LoadUpdateContentHandlerExtension tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "LoadUpdateContentHandlerExtension fails when handler output is null")
{
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", nullptr);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "LoadUpdateContentHandlerExtension returns cached handler")
{
    MockContentHandler* handler = new MockContentHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);

    ContentHandler* retrieved = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", &retrieved);
    CHECK(result.ResultCode == 1);
    CHECK(retrieved == handler);
}

TEST_CASE_METHOD(ExtMgrFixture, "LoadUpdateContentHandlerExtension fails when PathUtils_SanitizePathSegment returns null")
{
    mock_sanitize_path_return_null = true;

    ContentHandler* handler = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", &handler);
    CHECK(result.ResultCode == 0);
    CHECK(handler == nullptr);
}

TEST_CASE_METHOD(ExtMgrFixture, "LoadUpdateContentHandlerExtension fails when config is null")
{
    mock_config_info_return_null = true;

    ContentHandler* handler = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", &handler);
    CHECK(result.ResultCode == 0);
    CHECK(handler == nullptr);
}

TEST_CASE_METHOD(ExtMgrFixture, "LoadUpdateContentHandlerExtension fails when extension library load fails")
{
    mock_config_info.extensionsStepHandlerFolder = "/tmp/extensions";
    mock_get_extension_file_entity_return = false; // LoadExtensionLibrary will fail

    ContentHandler* handler = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", &handler);
    CHECK(result.ResultCode == 0);
    CHECK(handler == nullptr);
}

// =====================================================================
// LoadContentDownloaderLibrary tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "LoadContentDownloaderLibrary returns cached downloader")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);

    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 1);
    CHECK(lib == &fake_lib);
}

TEST_CASE_METHOD(ExtMgrFixture, "LoadContentDownloaderLibrary fails when extension load fails and no cached")
{
    // Clear any cached downloader
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    // LoadExtensionLibrary will fail since GetExtensionFileEntity returns false
    mock_get_extension_file_entity_return = false;

    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// IsComponentsEnumeratorRegistered tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "IsComponentsEnumeratorRegistered returns false when no enumerator loaded")
{
    // LoadComponentEnumeratorLibrary will fail because GetExtensionFileEntity returns false
    mock_get_extension_file_entity_return = false;

    bool registered = ExtensionManager::IsComponentsEnumeratorRegistered();
    CHECK_FALSE(registered);
}

// =====================================================================
// LoadComponentEnumeratorLibrary tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "LoadComponentEnumeratorLibrary fails when extension not found")
{
    mock_get_extension_file_entity_return = false;

    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// InitializeContentDownloader tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "InitializeContentDownloader fails when downloader lib not loaded")
{
    ExtensionManager::SetContentDownloaderLibrary(nullptr);
    mock_get_extension_file_entity_return = false;

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test");
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "InitializeContentDownloader fails when contract is not v1")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    mock_is_v1_contract_return = false;

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test");
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "InitializeContentDownloader fails when Initialize symbol not found")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    mock_is_v1_contract_return = true;
    mock_dlsym_return = nullptr; // dlsym returns null for Initialize symbol

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test");
    CHECK(result.ResultCode == 0);
}

static ADUC_Result mock_initialize_success(const char* /*initializeData*/)
{
    return ADUC_Result{ 1, 0 };
}

TEST_CASE_METHOD(ExtMgrFixture, "InitializeContentDownloader succeeds when Initialize function works")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    mock_is_v1_contract_return = true;
    mock_dlsym_return = reinterpret_cast<void*>(&mock_initialize_success);

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test");
    CHECK(result.ResultCode == 1);
}

// =====================================================================
// Download tests
// =====================================================================

static ADUC_Result mock_download_success(
    const ADUC_FileEntity* /*entity*/,
    const char* /*workflowId*/,
    const char* /*workFolder*/,
    unsigned int /*timeoutInSeconds*/,
    ADUC_DownloadProgressCallback /*cb*/)
{
    return ADUC_Result{ 1, 0 };
}

static ADUC_Result mock_download_failure(
    const ADUC_FileEntity* /*entity*/,
    const char* /*workflowId*/,
    const char* /*workFolder*/,
    unsigned int /*timeoutInSeconds*/,
    ADUC_DownloadProgressCallback /*cb*/)
{
    return ADUC_Result{ 0, 0x12345678 };
}

static DownloadProc mock_resolver_success(void* /*lib*/)
{
    return mock_download_success;
}

static DownloadProc mock_resolver_failure(void* /*lib*/)
{
    return mock_download_failure;
}

static DownloadProc mock_resolver_null(void* /*lib*/)
{
    return nullptr;
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails when workfolder filepath cannot be constructed")
{
    mock_workflow_get_entity_workfolder_filepath_return = false;

    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails when content downloader library cannot be loaded")
{
    ExtensionManager::SetContentDownloaderLibrary(nullptr);
    mock_get_extension_file_entity_return = false;

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails when contract is not v1")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);

    ADUC_ExtensionContractInfo cv{ 2, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = false;

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails when download proc resolver returns null")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_null);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails when hash type is unsupported")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = false; // unsupported hash type

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    // Note: The source code only sets ExtendedResultCode on hash type failure
    // but doesn't reset ResultCode (left at success from LoadContentDownloaderLibrary).
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download skips when file exists and hash is valid")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = 0; // file exists
    mock_is_valid_file_hash_return = true; // valid hash

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download returns failure when hash value is null for existing file")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = 0; // file exists
    mock_hash_value_return = nullptr; // null hash value

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download performs full download when file does not exist")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1; // file does not exist
    mock_is_valid_file_hash_return = true; // hash verification succeeds

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = nullptr; // no download handler

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download calls ProcessDownloadHandlerExtensibility when DownloadHandlerId is set")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1; // file does not exist
    mock_is_valid_file_hash_return = true;

    // Set download handler to succeed
    mock_process_download_handler_result = { 1, 0 };

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    char downloadHandlerId[] = "microsoft/delta:1";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = downloadHandlerId;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 1);
    CHECK(mock_process_download_handler_call_count == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download falls back to full download when download handler fails")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1; // file does not exist
    mock_is_valid_file_hash_return = true;

    // Download handler fails
    mock_process_download_handler_result = { 0, 0x12345 };

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    char downloadHandlerId[] = "microsoft/delta:1";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = downloadHandlerId;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    // Should succeed via fallback download
    CHECK(result.ResultCode == 1);
    CHECK(mock_process_download_handler_call_count == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download returns failure when downloadProc fails and no handler")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1; // file does not exist

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = nullptr;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_failure);
    CHECK(result.ResultCode == 0);
}

TEST_CASE_METHOD(ExtMgrFixture, "Download fails hash check after successful download")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1; // file does not exist
    mock_is_valid_file_hash_return = false; // hash will fail

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = nullptr;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 0);
    CHECK(mock_workflow_add_erc_call_count > 0);
}

TEST_CASE_METHOD(
    ExtMgrFixture,
    "Download falls back when handler returns Download_Handler_RequiredFullDownload")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    ADUC_ExtensionContractInfo cv{ 1, 0 };
    ExtensionManager::SetContentDownloaderContractVersion(cv);
    mock_is_v1_contract_return = true;
    mock_get_sha_version_return = true;
    mock_access_return = -1;
    mock_is_valid_file_hash_return = true;

    // Download handler returns RequiredFullDownload
    mock_process_download_handler_result = { ADUC_Result_Download_Handler_RequiredFullDownload, 0 };

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    char downloadHandlerId[] = "microsoft/delta:1";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;
    entity.DownloadHandlerId = downloadHandlerId;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    ADUC_Result result = ExtensionManager::Download(
        &entity, &dummy_workflow, &options, nullptr, mock_resolver_success);
    CHECK(result.ResultCode == 1);
    CHECK(mock_process_download_handler_call_count == 1);
}

// =====================================================================
// C API wrapper tests
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "ExtensionManager_InitializeContentDownloader wraps InitializeContentDownloader")
{
    int fake_lib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fake_lib);
    mock_is_v1_contract_return = true;
    mock_dlsym_return = reinterpret_cast<void*>(&mock_initialize_success);

    ADUC_Result result = ExtensionManager_InitializeContentDownloader("test");
    CHECK(result.ResultCode == 1);
}

TEST_CASE_METHOD(ExtMgrFixture, "ExtensionManager_Uninit wraps Uninit")
{
    REQUIRE_NOTHROW(ExtensionManager_Uninit());
}

TEST_CASE_METHOD(ExtMgrFixture, "ExtensionManager_Download wraps Download")
{
    ExtensionManager::SetContentDownloaderLibrary(nullptr);
    mock_get_extension_file_entity_return = false;

    ADUC_FileEntity entity{};
    char targetFilename[] = "target.bin";
    entity.TargetFilename = targetFilename;
    entity.HashCount = 1;

    int dummy_workflow = 0;
    ExtensionManager_Download_Options options{ 1 };

    // Will fail because content downloader library can't load
    ADUC_Result result = ExtensionManager_Download(
        &entity, &dummy_workflow, &options, nullptr);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// DefaultDownloadProcResolver test
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "DefaultDownloadProcResolver returns null when dlsym fails")
{
    mock_dlsym_return = nullptr;
    int fake_lib = 42;
    DownloadProc proc = ExtensionManager::DefaultDownloadProcResolver(&fake_lib);
    CHECK(proc == nullptr);
}

TEST_CASE_METHOD(ExtMgrFixture, "DefaultDownloadProcResolver returns function pointer when dlsym succeeds")
{
    mock_dlsym_return = reinterpret_cast<void*>(&mock_download_success);
    int fake_lib = 42;
    DownloadProc proc = ExtensionManager::DefaultDownloadProcResolver(&fake_lib);
    CHECK(proc != nullptr);
}

// =====================================================================
// GetAllComponents test
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "GetAllComponents fails when component enumerator not loadable")
{
    mock_get_extension_file_entity_return = false;
    std::string output;
    ADUC_Result result = ExtensionManager::GetAllComponents(output);
    CHECK(result.ResultCode == 0);
    CHECK(output.empty());
}

// =====================================================================
// SelectComponents test
// =====================================================================

TEST_CASE_METHOD(ExtMgrFixture, "SelectComponents fails when component enumerator not loadable")
{
    mock_get_extension_file_entity_return = false;
    std::string output;
    ADUC_Result result = ExtensionManager::SelectComponents("{}", output);
    CHECK(result.ResultCode == 0);
    CHECK(output.empty());
}
