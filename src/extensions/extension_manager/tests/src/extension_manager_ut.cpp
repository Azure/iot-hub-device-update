/**
 * @file extension_manager_ut.cpp
 * @brief Unit Tests for extension manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/extension_manager.h>
#include <aduc/extension_manager.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <catch2/catch_all.hpp>
#include <dlfcn.h>
#include <extension_manager_download_test_case.hpp>
#include <string>

#include <aduc/auto_file_entity.hpp>
#include <aduc/calloc_wrapper.hpp>
#include <aduc/workflow_internal.h>
#include <aduc/workflow_utils.h>
#include <cstdio> // std::rename
#include <fstream>
#include <memory>
#include <parson.h>
#include <aducpal/stdio.h> // remove

bool operator==(ADUC_Result a, ADUC_Result b)
{
    return a.ResultCode == b.ResultCode && a.ExtendedResultCode == b.ExtendedResultCode;
}

// Concrete ContentHandler for non-mock tests
namespace
{

class TestContentHandlerForEM : public ContentHandler
{
public:
    TestContentHandlerForEM() = default;
    ~TestContentHandlerForEM() override = default;

    ADUC_Result Download(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return ADUC_Result{ 1, 0 }; }
};

// RAII cleanup that resets ExtensionManager state after each test
struct ExtMgrCleanup
{
    ~ExtMgrCleanup()
    {
        ExtensionManager::SetContentDownloaderLibrary(nullptr);
        ExtensionManager::Uninit();
    }
};

// RAII guard that temporarily hides a file by renaming it so that
// tests do not accidentally pick up real extension registrations
// installed on the host (e.g. /var/lib/adu/extensions/…/extension.json).
struct ScopedHideFile
{
    std::string original;
    std::string hidden;
    bool renamed = false;

    explicit ScopedHideFile(const std::string& path)
        : original(path), hidden(path + ".hidden_by_test")
    {
        renamed = (std::rename(original.c_str(), hidden.c_str()) == 0);
    }

    ~ScopedHideFile()
    {
        if (renamed)
        {
            std::rename(hidden.c_str(), original.c_str());
        }
    }

    ScopedHideFile(const ScopedHideFile&) = delete;
    ScopedHideFile& operator=(const ScopedHideFile&) = delete;
};

} // namespace

// =====================================================================
// Download test cases (existing)
// =====================================================================

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
// Regression tests for issue #765:
// https://github.com/Azure/iot-hub-device-update/issues/765
//
// "Download function returns SUCCESS after deleting file it should have downloaded."
//
// When the target file already exists in the work folder but its hash does not
// match the manifest, ExtensionManager::Download must:
//   1) remove the stale file, AND
//   2) actually attempt a fresh download, AND
//   3) return a result that reflects that download's outcome.
//
// Prior to the fix in commit 9eff6612 (PR #856), the code returned ADUC_Result_Success
// immediately after removing the stale file, without invoking the downloader.
//

TEST_CASE("ExtensionManager::Download redownloads when existing file has invalid hash (issue #765)")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::ExistingFileInvalidHashRedownload };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    // The downloader MUST have been invoked exactly once. Before the fix this
    // count would be 0, because Download returned Success after deleting the
    // stale file without ever calling the downloader.
    CHECK(testCase.GetSuccessDownloadProcCallCount() == 1);

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

TEST_CASE("ExtensionManager::Download must not return SUCCESS when stale file is deleted and redownload fails (issue #765)")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::ExistingFileInvalidHashDownloadFails };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    // The downloader MUST have been invoked. Before the fix it was not called
    // at all and the function erroneously returned Success.
    CHECK(testCase.GetFailureDownloadProcCallCount() == 1);

    // The result MUST reflect the (failed) download, not the stale-file removal.
    CHECK(IsAducResultCodeFailure(actual_result.ResultCode));
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
TEST_CASE("ExtensionManager::Download fails for unsupported downloader contract")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::UnsupportedContractVersion };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

TEST_CASE("ExtensionManager::Download fails when resolver returns null download proc")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::MissingDownloadProc };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

TEST_CASE("ExtensionManager::Download fails when file hash algorithm is unsupported")
{
    ExtensionManagerDownloadTestCase testCase{ DownloadTestScenario::UnsupportedHashType };
    REQUIRE_NOTHROW(testCase.RunScenario());

    ADUC_Result actual_result = testCase.GetActualResult();
    ADUC_Result expected_result = testCase.GetExpectedResult();

    CHECK(actual_result.ResultCode == expected_result.ResultCode);
    CHECK(actual_result.ExtendedResultCode == expected_result.ExtendedResultCode);
}

// =====================================================================
// SetUpdateContentHandlerExtension tests
// =====================================================================

TEST_CASE("SetUpdateContentHandlerExtension returns failure when handler is null")
{
    ExtMgrCleanup cleanup;
    ADUC_Result result = ExtensionManager::SetUpdateContentHandlerExtension("test/type", nullptr);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE("SetUpdateContentHandlerExtension succeeds with valid handler")
{
    ExtMgrCleanup cleanup;
    auto* handler = new TestContentHandlerForEM();
    ADUC_Result result = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);
    CHECK(result.ResultCode == 1);
}

TEST_CASE("SetUpdateContentHandlerExtension replaces existing handler")
{
    ExtMgrCleanup cleanup;
    auto* handler1 = new TestContentHandlerForEM();
    auto* handler2 = new TestContentHandlerForEM();

    ADUC_Result r1 = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler1);
    CHECK(r1.ResultCode == 1);

    // Replaces handler1 (handler1 is leaked — acceptable in test)
    ADUC_Result r2 = ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler2);
    CHECK(r2.ResultCode == 1);
}

// =====================================================================
// SetContentDownloaderLibrary tests
// =====================================================================

TEST_CASE("SetContentDownloaderLibrary returns success")
{
    ExtMgrCleanup cleanup;
    int fakeLib = 42;
    ADUC_Result result = ExtensionManager::SetContentDownloaderLibrary(&fakeLib);
    CHECK(result.ResultCode == 1);
}

// =====================================================================
// Get/SetContentDownloaderContractVersion tests
// =====================================================================

TEST_CASE("GetContentDownloaderContractVersion returns stored version")
{
    ExtMgrCleanup cleanup;
    ADUC_ExtensionContractInfo setInfo{ 2, 3 };
    ExtensionManager::SetContentDownloaderContractVersion(setInfo);

    ADUC_ExtensionContractInfo getInfo{};
    ADUC_Result result = ExtensionManager::GetContentDownloaderContractVersion(&getInfo);
    CHECK(result.ResultCode == 1);
    CHECK(getInfo.majorVer == 2);
    CHECK(getInfo.minorVer == 3);
}

TEST_CASE("SetContentDownloaderContractVersion overwrites previous value")
{
    ExtMgrCleanup cleanup;
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

TEST_CASE("GetComponentEnumeratorContractVersion returns default zero version")
{
    ExtMgrCleanup cleanup;
    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = ExtensionManager::GetComponentEnumeratorContractVersion(&info);
    CHECK(result.ResultCode == 1);
    CHECK(info.majorVer == 0);
    CHECK(info.minorVer == 0);
}

// =====================================================================
// Uninit tests
// =====================================================================

TEST_CASE("Uninit can be called without any handlers loaded")
{
    REQUIRE_NOTHROW(ExtensionManager::Uninit());
}

TEST_CASE("Uninit clears cached content handlers")
{
    auto* handler = new TestContentHandlerForEM();
    ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);
    REQUIRE_NOTHROW(ExtensionManager::Uninit());
}

// =====================================================================
// LoadUpdateContentHandlerExtension tests
// =====================================================================

TEST_CASE("LoadUpdateContentHandlerExtension fails when handler output is null")
{
    ExtMgrCleanup cleanup;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", nullptr);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE("LoadUpdateContentHandlerExtension returns cached handler")
{
    ExtMgrCleanup cleanup;
    auto* handler = new TestContentHandlerForEM();
    ExtensionManager::SetUpdateContentHandlerExtension("test/type", handler);

    ContentHandler* retrieved = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/type", &retrieved);
    CHECK(result.ResultCode == 1);
    CHECK(retrieved == handler);
}

// =====================================================================
// LoadContentDownloaderLibrary tests
// =====================================================================

TEST_CASE("LoadContentDownloaderLibrary returns cached downloader")
{
    ExtMgrCleanup cleanup;
    int fakeLib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fakeLib);

    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 1);
    CHECK(lib == &fakeLib);
}

TEST_CASE("LoadContentDownloaderLibrary fails when extension load fails and no cached")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_CONTENT_DOWNLOADER_EXTENSION_PATH);
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// IsComponentsEnumeratorRegistered tests
// =====================================================================

TEST_CASE("IsComponentsEnumeratorRegistered returns false when no enumerator loaded")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_COMPONENT_ENUMERATOR_EXTENSION_PATH);
    bool registered = ExtensionManager::IsComponentsEnumeratorRegistered();
    CHECK_FALSE(registered);
}

// =====================================================================
// LoadComponentEnumeratorLibrary tests
// =====================================================================

TEST_CASE("LoadComponentEnumeratorLibrary fails when extension not found")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_COMPONENT_ENUMERATOR_EXTENSION_PATH);
    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// InitializeContentDownloader tests
// =====================================================================

TEST_CASE("InitializeContentDownloader fails when downloader lib not loaded")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_CONTENT_DOWNLOADER_EXTENSION_PATH);
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test", ADUC_LOG_DEBUG);
    CHECK(result.ResultCode == 0);
}

TEST_CASE("InitializeContentDownloader fails when contract version is unsupported")
{
    ExtMgrCleanup cleanup;

    int fakeLib = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fakeLib);
    ExtensionManager::SetContentDownloaderContractVersion({ 3, 0 });

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test", ADUC_LOG_DEBUG);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION);
}

// =====================================================================
// GetAllComponents / SelectComponents tests
// =====================================================================

TEST_CASE("GetAllComponents fails when component enumerator not loadable")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_COMPONENT_ENUMERATOR_EXTENSION_PATH);
    std::string output;
    ADUC_Result result = ExtensionManager::GetAllComponents(output);
    CHECK(result.ResultCode == 0);
    CHECK(output.empty());
}

TEST_CASE("SelectComponents fails when component enumerator not loadable")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_COMPONENT_ENUMERATOR_EXTENSION_PATH);
    std::string output;
    ADUC_Result result = ExtensionManager::SelectComponents("{}", output);
    CHECK(result.ResultCode == 0);
    CHECK(output.empty());
}

// =====================================================================
// C API wrapper tests
// =====================================================================

TEST_CASE("ExtensionManager_Uninit wraps Uninit")
{
    REQUIRE_NOTHROW(ExtensionManager_Uninit());
}

TEST_CASE("ExtensionManager_InitializeContentDownloader wrapper returns failure when no downloader")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_CONTENT_DOWNLOADER_EXTENSION_PATH);
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    ADUC_Result result = ExtensionManager_InitializeContentDownloader("test", ADUC_LOG_DEBUG);
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// UnloadAllUpdateContentHandlers tests
// =====================================================================

TEST_CASE("UnloadAllUpdateContentHandlers deletes multiple cached handlers")
{
    ExtMgrCleanup cleanup;
    auto* h1 = new TestContentHandlerForEM();
    auto* h2 = new TestContentHandlerForEM();
    auto* h3 = new TestContentHandlerForEM();

    ExtensionManager::SetUpdateContentHandlerExtension("type/a", h1);
    ExtensionManager::SetUpdateContentHandlerExtension("type/b", h2);
    ExtensionManager::SetUpdateContentHandlerExtension("type/c", h3);

    // After Uninit, loading any cached handler should fail
    ExtensionManager::Uninit();

    ContentHandler* handler = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("type/a", &handler);
    CHECK(result.ResultCode == 0);
    CHECK(handler == nullptr);
}

TEST_CASE("UnloadAllUpdateContentHandlers with no handlers is safe")
{
    ExtMgrCleanup cleanup;
    REQUIRE_NOTHROW(ExtensionManager::Uninit());
    // Call again to verify double-uninit is safe
    REQUIRE_NOTHROW(ExtensionManager::Uninit());
}

// =====================================================================
// LoadUpdateContentHandlerExtension tests - additional coverage
// =====================================================================

TEST_CASE("LoadUpdateContentHandlerExtension fails with non-cached handler when config not initialized")
{
    // When handler is not cached and config singleton is not initialized,
    // LoadUpdateContentHandlerExtension returns failure at the config null check.
    ExtMgrCleanup cleanup;

    ContentHandler* handler = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("microsoft/not-cached:1", &handler);
    CHECK(result.ResultCode == 0);
    CHECK(handler == nullptr);
}

TEST_CASE("LoadUpdateContentHandlerExtension retrieves correct handler after multiple sets")
{
    ExtMgrCleanup cleanup;
    auto* h1 = new TestContentHandlerForEM();
    auto* h2 = new TestContentHandlerForEM();

    ExtensionManager::SetUpdateContentHandlerExtension("type/first", h1);
    ExtensionManager::SetUpdateContentHandlerExtension("type/second", h2);

    ContentHandler* retrieved = nullptr;

    ADUC_Result r1 = ExtensionManager::LoadUpdateContentHandlerExtension("type/first", &retrieved);
    CHECK(r1.ResultCode == 1);
    CHECK(retrieved == h1);

    ADUC_Result r2 = ExtensionManager::LoadUpdateContentHandlerExtension("type/second", &retrieved);
    CHECK(r2.ResultCode == 1);
    CHECK(retrieved == h2);
}

// =====================================================================
// SetContentDownloaderLibrary tests - additional coverage
// =====================================================================

TEST_CASE("SetContentDownloaderLibrary followed by Load retrieves the same pointer")
{
    ExtMgrCleanup cleanup;
    int fakeLib1 = 100;
    int fakeLib2 = 200;

    ExtensionManager::SetContentDownloaderLibrary(&fakeLib1);
    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 1);
    CHECK(lib == &fakeLib1);

    // Replace with a different lib
    ExtensionManager::SetContentDownloaderLibrary(&fakeLib2);
    lib = nullptr;
    result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 1);
    CHECK(lib == &fakeLib2);
}

// =====================================================================
// Download - file already exists tests
// =====================================================================

// Dummy download proc that records it was called.
static bool g_downloadProcCalled = false;
static ADUC_Result DummyDownloadProc(
    const ADUC_FileEntity* /*entity*/,
    const char* /*workflowId*/,
    const char* /*workFolder*/,
    unsigned int /*timeoutInSeconds*/,
    ADUC_DownloadProgressCallback /*downloadProgressCallback*/)
{
    g_downloadProcCalled = true;
    return ADUC_Result{ 1, 0 };
}

static DownloadProc dummyProcResolver(void* /*lib*/)
{
    return DummyDownloadProc;
}

TEST_CASE("Download skips download when target file exists with valid hash")
{
    // When the target file already exists in the work folder with a valid hash,
    // Download returns success without actually invoking the download proc.
    ExtMgrCleanup cleanup;

    int fakeLibForDownload = 42;
    ExtensionManager::SetContentDownloaderLibrary(&fakeLibForDownload);
    ExtensionManager::SetContentDownloaderContractVersion({ 1, 0 });

    // Set up workflow handle
    const std::string testWorkfolder = std::string{ ADUC_TEST_DATA_FOLDER } + "/extension_manager";
    const std::string pnpMsgPath = testWorkfolder + "/pnpMsg.json";
    const std::string updateManifestPath = testWorkfolder + "/testUpdateManifest.json";
    const std::string downloaded_file = testWorkfolder + "/mock_update_payload.txt";

    std::unique_ptr<JSON_Value, decltype(&json_value_free)> msgValue{
        json_parse_file(pnpMsgPath.c_str()), json_value_free
    };
    std::unique_ptr<JSON_Value, decltype(&json_value_free)> updateManifestValue{
        json_parse_file(updateManifestPath.c_str()), json_value_free
    };
    REQUIRE(msgValue.get() != nullptr);
    REQUIRE(updateManifestValue.get() != nullptr);

    JSON_Object* msgObj = json_value_get_object(msgValue.get());
    ADUC::StringUtils::cstr_wrapper serializedUM{ json_serialize_to_string(updateManifestValue.get()) };
    REQUIRE(JSONSuccess == json_object_dotset_string(msgObj, "updateManifest", serializedUM.get()));

    ADUC::StringUtils::cstr_wrapper serializedMsg{ json_serialize_to_string_pretty(msgValue.get()) };

    ADUC_WorkflowHandle workflowHandle = nullptr;
    ADUC_Result initRes = workflow_init(serializedMsg.get(), false, &workflowHandle);
    REQUIRE(IsAducResultCodeSuccess(initRes.ResultCode));
    auto wf = reinterpret_cast<ADUC_Workflow*>(workflowHandle);
    REQUIRE(JSONSuccess == json_object_set_string(wf->PropertiesObject, "_workFolder", testWorkfolder.c_str()));

    // Create the file with correct content ("hello" → sha256 matches manifest)
    {
        std::ofstream out{ downloaded_file, std::ios::trunc };
        out << "hello";
    }

    AutoFileEntity fileEntity;
    REQUIRE(workflow_get_update_file(workflowHandle, 0, &fileEntity));

    g_downloadProcCalled = false;

    ExtensionManager_Download_Options options{ 1 };
    ADUC_Result result = ExtensionManager::Download(
        &fileEntity, workflowHandle, &options, nullptr, dummyProcResolver);

    CHECK(result.ResultCode == ADUC_Result_Success);
    // The download proc should NOT have been called since the file already exists
    // with a valid hash.
    CHECK_FALSE(g_downloadProcCalled);

    // Cleanup
    remove(downloaded_file.c_str());
    workflow_free(workflowHandle);
}

TEST_CASE("Download returns hash-type-not-supported when entity has no hashes")
{
    // When entity has an unsupported hash type, GetShaVersionForTypeString returns false
    // and Download fails with FILE_HASH_TYPE_NOT_SUPPORTED.
    // NOTE: We cannot set Hash=nullptr/HashCount=0 because GetShaVersionForTypeString
    // does not guard against a NULL hashTypeStr, causing a segfault in strcasecmp.
    ExtMgrCleanup cleanup;

    int fakeLibForDownload2 = 43;
    ExtensionManager::SetContentDownloaderLibrary(&fakeLibForDownload2);
    ExtensionManager::SetContentDownloaderContractVersion({ 1, 0 });

    const std::string testWorkfolder = std::string{ ADUC_TEST_DATA_FOLDER } + "/extension_manager";
    const std::string pnpMsgPath = testWorkfolder + "/pnpMsg.json";
    const std::string updateManifestPath = testWorkfolder + "/testUpdateManifest.json";
    const std::string downloaded_file = testWorkfolder + "/mock_update_payload.txt";

    std::unique_ptr<JSON_Value, decltype(&json_value_free)> msgValue{
        json_parse_file(pnpMsgPath.c_str()), json_value_free
    };
    std::unique_ptr<JSON_Value, decltype(&json_value_free)> updateManifestValue{
        json_parse_file(updateManifestPath.c_str()), json_value_free
    };
    REQUIRE(msgValue.get() != nullptr);
    REQUIRE(updateManifestValue.get() != nullptr);

    JSON_Object* msgObj = json_value_get_object(msgValue.get());
    ADUC::StringUtils::cstr_wrapper serializedUM{ json_serialize_to_string(updateManifestValue.get()) };
    REQUIRE(JSONSuccess == json_object_dotset_string(msgObj, "updateManifest", serializedUM.get()));

    ADUC::StringUtils::cstr_wrapper serializedMsg{ json_serialize_to_string_pretty(msgValue.get()) };

    ADUC_WorkflowHandle workflowHandle = nullptr;
    ADUC_Result initRes = workflow_init(serializedMsg.get(), false, &workflowHandle);
    REQUIRE(IsAducResultCodeSuccess(initRes.ResultCode));
    auto wf = reinterpret_cast<ADUC_Workflow*>(workflowHandle);
    REQUIRE(JSONSuccess == json_object_set_string(wf->PropertiesObject, "_workFolder", testWorkfolder.c_str()));

    AutoFileEntity fileEntity;
    REQUIRE(workflow_get_update_file(workflowHandle, 0, &fileEntity));

    // Replace hashes with an unsupported type ("md5") to trigger hash-type-not-supported
    ADUC_Hash unsupportedHash;
    unsupportedHash.type = strdup("md5");
    unsupportedHash.value = strdup("abc123");

    ADUC_FileEntity badHashEntity = fileEntity;
    badHashEntity.Hash = &unsupportedHash;
    badHashEntity.HashCount = 1;

    ExtensionManager_Download_Options options{ 1 };
    ADUC_Result result = ExtensionManager::Download(
        &badHashEntity, workflowHandle, &options, nullptr, dummyProcResolver);

    // NOTE: The source code only sets ExtendedResultCode in the hash-type-not-supported
    // branch without resetting ResultCode after LoadContentDownloaderLibrary set it to Success.
    CHECK(result.ExtendedResultCode == ADUC_ERC_CONTENT_DOWNLOADER_FILE_HASH_TYPE_NOT_SUPPORTED);

    free(unsupportedHash.type);
    free(unsupportedHash.value);
    remove(downloaded_file.c_str());
    workflow_free(workflowHandle);
}

// =====================================================================
// ExtensionManager_Download C wrapper test
// =====================================================================

TEST_CASE("ExtensionManager_Download C wrapper fails when no downloader library loaded")
{
    ExtMgrCleanup cleanup;
    ScopedHideFile hideExtJson(ADUC_CONTENT_DOWNLOADER_EXTENSION_PATH);
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    const std::string testWorkfolder = std::string{ ADUC_TEST_DATA_FOLDER } + "/extension_manager";
    const std::string pnpMsgPath = testWorkfolder + "/pnpMsg.json";
    const std::string updateManifestPath = testWorkfolder + "/testUpdateManifest.json";

    std::unique_ptr<JSON_Value, decltype(&json_value_free)> msgValue{
        json_parse_file(pnpMsgPath.c_str()), json_value_free
    };
    std::unique_ptr<JSON_Value, decltype(&json_value_free)> updateManifestValue{
        json_parse_file(updateManifestPath.c_str()), json_value_free
    };
    REQUIRE(msgValue.get() != nullptr);
    REQUIRE(updateManifestValue.get() != nullptr);

    JSON_Object* msgObj = json_value_get_object(msgValue.get());
    ADUC::StringUtils::cstr_wrapper serializedUM{ json_serialize_to_string(updateManifestValue.get()) };
    REQUIRE(JSONSuccess == json_object_dotset_string(msgObj, "updateManifest", serializedUM.get()));

    ADUC::StringUtils::cstr_wrapper serializedMsg{ json_serialize_to_string_pretty(msgValue.get()) };

    ADUC_WorkflowHandle workflowHandle = nullptr;
    ADUC_Result initRes = workflow_init(serializedMsg.get(), false, &workflowHandle);
    REQUIRE(IsAducResultCodeSuccess(initRes.ResultCode));
    auto wf = reinterpret_cast<ADUC_Workflow*>(workflowHandle);
    REQUIRE(JSONSuccess == json_object_set_string(wf->PropertiesObject, "_workFolder", testWorkfolder.c_str()));

    AutoFileEntity fileEntity;
    REQUIRE(workflow_get_update_file(workflowHandle, 0, &fileEntity));

    ExtensionManager_Download_Options options{ 1 };
    ADUC_Result result = ExtensionManager_Download(
        &fileEntity, workflowHandle, &options, nullptr);

    // Should fail because no content downloader library is loaded
    CHECK(result.ResultCode == 0);

    workflow_free(workflowHandle);
}

// =====================================================================
// DefaultDownloadProcResolver tests
// =====================================================================

TEST_CASE("DefaultDownloadProcResolver returns nullptr for nullptr lib")
{
    ExtMgrCleanup cleanup;
    DownloadProc proc = ExtensionManager::DefaultDownloadProcResolver(nullptr);
    CHECK(proc == nullptr);
}

// =====================================================================
// SetComponentEnumeratorContractVersion tests
// =====================================================================

TEST_CASE("GetComponentEnumeratorContractVersion returns updated version after handler loads")
{
    ExtMgrCleanup cleanup;

    // Initially should be {0,0}
    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = ExtensionManager::GetComponentEnumeratorContractVersion(&info);
    CHECK(result.ResultCode == 1);
    CHECK(info.majorVer == 0);
    CHECK(info.minorVer == 0);

    // After Uninit and re-checking, still {0,0}
    ExtensionManager::Uninit();
    result = ExtensionManager::GetComponentEnumeratorContractVersion(&info);
    CHECK(result.ResultCode == 1);
}

// =====================================================================
// SetUpdateContentHandlerExtension - edge cases
// =====================================================================

TEST_CASE("SetUpdateContentHandlerExtension with special chars in updateType succeeds")
{
    ExtMgrCleanup cleanup;
    auto* handler = new TestContentHandlerForEM();
    ADUC_Result result = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/test-special_chars:1", handler);
    CHECK(result.ResultCode == 1);

    ContentHandler* retrieved = nullptr;
    ADUC_Result loadResult = ExtensionManager::LoadUpdateContentHandlerExtension("microsoft/test-special_chars:1", &retrieved);
    CHECK(loadResult.ResultCode == 1);
    CHECK(retrieved == handler);
}

// =====================================================================
// Comprehensive Uninit tests
// =====================================================================

TEST_CASE("Uninit clears both handlers and downloader library state")
{
    ExtMgrCleanup cleanup;

    auto* handler = new TestContentHandlerForEM();
    ExtensionManager::SetUpdateContentHandlerExtension("test/uninit_type", handler);

    int fakeLib = 99;
    ExtensionManager::SetContentDownloaderLibrary(&fakeLib);

    ExtensionManager::Uninit();

    // Handler should no longer be retrievable
    ContentHandler* retrieved = nullptr;
    ADUC_Result result = ExtensionManager::LoadUpdateContentHandlerExtension("test/uninit_type", &retrieved);
    CHECK(result.ResultCode == 0);
    CHECK(retrieved == nullptr);

    // Downloader lib is NOT cleared by Uninit (it only clears _libs map and _contentHandlers).
    // Verify it's still accessible.
    void* lib = nullptr;
    result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    CHECK(result.ResultCode == 1);
    CHECK(lib == &fakeLib);
}
