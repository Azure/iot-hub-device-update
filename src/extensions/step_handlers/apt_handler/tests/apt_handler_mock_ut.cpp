/**
 * @file apt_handler_mock_ut.cpp
 * @brief Mock-based unit tests for apt_handler.cpp to maximize coverage.
 *
 * Tests all public methods of AptHandlerImpl: Download, Install, Apply,
 * Cancel, IsInstalled, Backup, Restore, plus the exported C functions.
 */
#include "mock_apt_handler_deps.h"

#include "aduc/apt_handler.hpp"
#include <aduc/content_handler.hpp>
#include <aduc/result.h>
#include <aduc/types/workflow.h>

#include <catch2/catch_all.hpp>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// Valid APT manifest JSON for ParseContent to succeed
static const char* s_valid_apt_manifest =
    R"({"name":"test-apt","version":"1.0","packages":[{"name":"pkg1"}]})";

// APT manifest with agentRestartRequired=true
static const char* s_restart_apt_manifest =
    R"({"name":"test-apt","version":"1.0","agentRestartRequired":true,"packages":[{"name":"pkg1"}]})";

// Helper: write a string to a file
static void WriteFile(const std::string& path, const char* content)
{
    std::ofstream f(path);
    f << content;
    f.close();
}

// Fixture that resets mock state before each test
struct AptFixture
{
    AptFixture()
    {
        mock_apt_handler_reset();
        // Ensure the mock workfolder directory exists so tests can write manifest files
        std::filesystem::create_directories(mock_workfolder);
    }
    ~AptFixture()
    {
        // Clean up any manifest files and workfolder created during tests
        std::filesystem::remove_all(mock_workfolder);
    }
};

// =====================================================================
// Exported C functions
// =====================================================================

TEST_CASE("GetContractInfo: fills version and returns success")
{
    ADUC_ExtensionContractInfo info = {};
    ADUC_Result result = GetContractInfo(&info);
    CHECK(IsAducResultCodeSuccess(result.ResultCode));
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("CreateUpdateContentHandlerExtension: returns non-null handler")
{
    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    CHECK(handler != nullptr);
    delete handler;
}

// =====================================================================
// CreateContentHandler & destructor
// =====================================================================

TEST_CASE("AptHandlerImpl::CreateContentHandler returns non-null")
{
    ContentHandler* h = AptHandlerImpl::CreateContentHandler();
    CHECK(h != nullptr);
    delete h;
}

// =====================================================================
// Backup & Restore (simple no-op methods)
// =====================================================================

TEST_CASE_METHOD(AptFixture, "Backup: returns Backup_Success_Unsupported")
{
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    ADUC_WorkflowData wd = {};
    ADUC_Result result = handler->Backup(&wd);
    CHECK(result.ResultCode == ADUC_Result_Backup_Success_Unsupported);
}

TEST_CASE_METHOD(AptFixture, "Restore: returns Restore_Success_Unsupported")
{
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    ADUC_WorkflowData wd = {};
    ADUC_Result result = handler->Restore(&wd);
    CHECK(result.ResultCode == ADUC_Result_Restore_Success_Unsupported);
}

// =====================================================================
// Cancel
// =====================================================================

TEST_CASE_METHOD(AptFixture, "Cancel: success -> Cancel_Success")
{
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Cancel(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);
}

TEST_CASE_METHOD(AptFixture, "Cancel: request_cancel fails -> Cancel_UnableToCancel")
{
    mock_request_cancel_return = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Cancel(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_UnableToCancel);
}

// =====================================================================
// IsInstalled
// =====================================================================

TEST_CASE_METHOD(AptFixture, "IsInstalled: installed criteria null -> NotInstalled")
{
    mock_workflowdata_installed_criteria = nullptr;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
}

TEST_CASE_METHOD(AptFixture, "IsInstalled: installed criteria present -> delegates to GetIsInstalled")
{
    mock_get_is_installed_result = { ADUC_Result_IsInstalled_Installed, 0 };
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
}

// =====================================================================
// Download
// =====================================================================

TEST_CASE_METHOD(AptFixture, "Download: cancel requested -> Cancel_Success")
{
    mock_is_cancel_requested = true;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);
}

TEST_CASE_METHOD(AptFixture, "Download: wrong file count -> failure")
{
    mock_update_files_count = 0;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT);
}

TEST_CASE_METHOD(AptFixture, "Download: get_update_file fails -> failure")
{
    mock_get_update_file_return = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE);
}

TEST_CASE_METHOD(AptFixture, "Download: empty installed criteria -> failure")
{
    mock_installed_criteria = nullptr;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_MISSING_INSTALLED_CRITERIA);
}

TEST_CASE_METHOD(AptFixture, "Download: config unavailable -> failure")
{
    mock_config_available = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_DOWNLOAD_FAILED_TO_GET_CONFIG_INSTANCE);
}

TEST_CASE_METHOD(AptFixture, "Download: ExtensionManager::Download fails -> failure")
{
    mock_extension_download_result = { ADUC_Result_Failure, 0x12345678 };
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
}

TEST_CASE_METHOD(AptFixture, "Download: full success path with valid manifest file")
{
    // Write a valid APT manifest to the expected path
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(mock_launch_child_call_count == 2); // initialize + download

    std::remove(manifestPath.c_str());
}

TEST_CASE_METHOD(AptFixture, "Download: apt download phase fails -> failure")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    mock_launch_child_exit_code_2 = 1; // download phase fails
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_DOWNLOAD_FAILURE);

    std::remove(manifestPath.c_str());
}

// =====================================================================
// Install
// =====================================================================

TEST_CASE_METHOD(AptFixture, "Install: cancel requested -> Cancel_Success")
{
    mock_is_cancel_requested = true;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);
}

TEST_CASE_METHOD(AptFixture, "Install: get_update_file fails -> failure")
{
    mock_get_update_file_return = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE);
}

TEST_CASE_METHOD(AptFixture, "Install: config unavailable -> failure")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    mock_config_available = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_INSTALL_FAILED_TO_GET_CONFIG_INSTANCE);

    std::remove(manifestPath.c_str());
}

TEST_CASE_METHOD(AptFixture, "Install: successful install path")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);

    std::remove(manifestPath.c_str());
}

TEST_CASE_METHOD(AptFixture, "Install: apt install fails -> failure")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    mock_launch_child_exit_code = 1;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_INSTALL_FAILURE);

    std::remove(manifestPath.c_str());
}

// =====================================================================
// Apply
// =====================================================================

TEST_CASE_METHOD(AptFixture, "Apply: cancel requested -> Cancel_Success")
{
    mock_is_cancel_requested = true;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);
}

TEST_CASE_METHOD(AptFixture, "Apply: PersistInstalledCriteria fails -> failure")
{
    mock_persist_installed_criteria_return = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE);
}

TEST_CASE_METHOD(AptFixture, "Apply: get_update_file fails -> failure")
{
    mock_get_update_file_return = false;
    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE);
}

TEST_CASE_METHOD(AptFixture, "Apply: success without restart")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_valid_apt_manifest);

    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Apply_Success);

    std::remove(manifestPath.c_str());
}

TEST_CASE_METHOD(AptFixture, "Apply: agentRestartRequired -> RequiredImmediateAgentRestart")
{
    std::string manifestPath = std::string(mock_workfolder) + "/" + mock_target_filename;
    WriteFile(manifestPath, s_restart_apt_manifest);

    auto handler = std::unique_ptr<ContentHandler>(AptHandlerImpl::CreateContentHandler());
    int dummyHandle = 0;
    ADUC_WorkflowData wd = {};
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle);
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Apply_RequiredImmediateAgentRestart);

    std::remove(manifestPath.c_str());
}
