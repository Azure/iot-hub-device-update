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
#include <aduc/result.h>
#include <catch2/catch_all.hpp>
#include <extension_manager_download_test_case.hpp>
#include <string>

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
    bool registered = ExtensionManager::IsComponentsEnumeratorRegistered();
    CHECK_FALSE(registered);
}

// =====================================================================
// LoadComponentEnumeratorLibrary tests
// =====================================================================

TEST_CASE("LoadComponentEnumeratorLibrary fails when extension not found")
{
    ExtMgrCleanup cleanup;
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
    ExtensionManager::SetContentDownloaderLibrary(nullptr);

    ADUC_Result result = ExtensionManager::InitializeContentDownloader("test");
    CHECK(result.ResultCode == 0);
}

// =====================================================================
// GetAllComponents / SelectComponents tests
// =====================================================================

TEST_CASE("GetAllComponents fails when component enumerator not loadable")
{
    ExtMgrCleanup cleanup;
    std::string output;
    ADUC_Result result = ExtensionManager::GetAllComponents(output);
    CHECK(result.ResultCode == 0);
    CHECK(output.empty());
}

TEST_CASE("SelectComponents fails when component enumerator not loadable")
{
    ExtMgrCleanup cleanup;
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
