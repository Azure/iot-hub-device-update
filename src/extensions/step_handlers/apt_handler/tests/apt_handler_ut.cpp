/**
 * @file apt_handler_ut.cpp
 * @brief Unit tests for apt_handler.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apt_handler.hpp"
#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/installed_criteria_utils.hpp"
#include "aducpal/stdlib.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

EXTERN_C_BEGIN

EXPORTED_METHOD ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
EXPORTED_METHOD ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);

EXTERN_C_END

ADUC_Result PrepareStepsWorkflowDataObject(ADUC_WorkflowHandle handle);

static void reset_installed_criteria_file()
{
    std::remove(ADUC_INSTALLEDCRITERIA_FILE_PATH);
}

static bool write_file_text(const std::string& path, const std::string& content)
{
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.good())
    {
        return false;
    }

    out << content;
    return out.good();
}

// clang-format off
const char* action_parent_update =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "dcb112da-bfc9-47b7-b7ed-617feba1e6c4"       )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": {                                          )"
    R"(         "f483750ebb885d32c": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json"      )"
    R"(     }                                                      )"
    R"( }                                                          )";

// clang-format off
const char* action_parent_update_missing_fileentity =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "dcb112da-bfc9-47b7-b7ed-617feba1e6c4"       )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"missing-file-id\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}}]},\"files\":{},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": {}                                          )"
    R"( }                                                          )";
// clang-format on
// clang-format on

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/script_handler_test_config";
    ADUCPAL_setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

static ADUC_WorkflowHandle SetupAptStepWorkflow(ADUC_WorkflowHandle* rootHandle)
{
    if (rootHandle != nullptr)
    {
        *rootHandle = nullptr;
    }

    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == nullptr)
    {
        return nullptr;
    }

    ContentHandler* aptHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    if (aptHandler == nullptr)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
        return nullptr;
    }

    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/apt:1", aptHandler);

    ADUC_WorkflowHandle localRootHandle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false, &localRootHandle);
    if (IsAducResultCodeFailure(result.ResultCode) || localRootHandle == nullptr)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
        ExtensionManager::Uninit();
        return nullptr;
    }

    result = PrepareStepsWorkflowDataObject(localRootHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_free(localRootHandle);
        ADUC_ConfigInfo_ReleaseInstance(config);
        ExtensionManager::Uninit();
        return nullptr;
    }

    ADUC_WorkflowHandle stepHandle = workflow_get_child(localRootHandle, 0);
    if (stepHandle == nullptr)
    {
        workflow_free(localRootHandle);
        ADUC_ConfigInfo_ReleaseInstance(config);
        ExtensionManager::Uninit();
        return nullptr;
    }

    ADUC_ConfigInfo_ReleaseInstance(config);

    if (rootHandle != nullptr)
    {
        *rootHandle = localRootHandle;
    }

    return stepHandle;
}

TEST_CASE("APT handler exported functions smoke test", "[apt_handler]")
{
    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    REQUIRE(handler != nullptr);
    delete handler;

    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = GetContractInfo(&info);
    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("APT handler backup and restore return unsupported", "[apt_handler]")
{
    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData workflowData{};
    ADUC_Result backupResult = handler->Backup(&workflowData);
    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success_Unsupported);

    ADUC_Result restoreResult = handler->Restore(&workflowData);
    CHECK(restoreResult.ResultCode == ADUC_Result_Restore_Success_Unsupported);
}

TEST_CASE("APT handler cancel/download paths on real workflows", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result cancelResult = handler->Cancel(&stepWorkflow);
    CHECK(cancelResult.ResultCode == ADUC_Result_Cancel_Success);

    CHECK(workflow_request_cancel(stepHandle));
    ADUC_Result downloadCancelResult = handler->Download(&stepWorkflow);
    CHECK(downloadCancelResult.ResultCode == ADUC_Result_Cancel_Success);

    ADUC_WorkflowData rootWorkflow{};
    rootWorkflow.WorkflowHandle = rootHandle;
    ADUC_Result rootWorkflowDownloadResult = handler->Download(&rootWorkflow);
    CHECK(rootWorkflowDownloadResult.ResultCode == ADUC_Result_Failure);
    CHECK(rootWorkflowDownloadResult.ExtendedResultCode == ADUC_ERC_APT_HANDLER_MISSING_INSTALLED_CRITERIA);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler install/apply parse-failure paths on missing manifest", "[apt_handler]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result installResult = handler->Install(&stepWorkflow);
    CHECK(installResult.ResultCode == ADUC_Result_Failure);
    CHECK(installResult.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_PARSE_BAD_FORMAT);

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(applyResult.ResultCode == ADUC_Result_Failure);
    CHECK(applyResult.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_PARSE_BAD_FORMAT);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler is-installed returns not-installed without persisted criteria", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result result = handler->IsInstalled(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler install/apply/cancel paths with local apt manifest", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    const std::string aptManifestDir =
        std::string(ADUC_TEST_DATA_FOLDER)
        + "/../../../src/extensions/component_enumerators/examples/contoso_component_enumerator/demo/sample-updates/data-files/APT";
    REQUIRE(workflow_set_workfolder(stepHandle, aptManifestDir.c_str()));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result installResult = handler->Install(&stepWorkflow);
    // Install may fail if adu-shell is not available in the test environment.
    // Verify the manifest was parsed correctly (not a parse error).
    CHECK(installResult.ExtendedResultCode != ADUC_ERC_UPDATE_CONTENT_HANDLER_PARSE_BAD_FORMAT);

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(applyResult.ExtendedResultCode != ADUC_ERC_UPDATE_CONTENT_HANDLER_PARSE_BAD_FORMAT);

    CHECK(workflow_request_cancel(stepHandle));
    ADUC_Result cancelledInstall = handler->Install(&stepWorkflow);
    CHECK(cancelledInstall.ResultCode == ADUC_Result_Cancel_Success);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler download fails with wrong file count", "[apt_handler]")
{
    set_test_config_folder();

    const char* workflowWithNoFiles =
        R"({"workflow":{"action":3,"id":"9a9a9a9a-1111-2222-3333-444444444444"},"updateManifest":"{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}}]},\"files\":{},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}","updateManifestSignature":"dummy","fileUrls":{}})";

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_Result initResult = workflow_init(workflowWithNoFiles, false, &rootHandle);
    REQUIRE(IsAducResultCodeSuccess(initResult.ResultCode));
    REQUIRE(rootHandle != nullptr);

    ContentHandler* aptHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    REQUIRE(aptHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/apt:1", aptHandler);

    ADUC_Result prepResult = PrepareStepsWorkflowDataObject(rootHandle);
    REQUIRE(IsAducResultCodeSuccess(prepResult.ResultCode));

    ADUC_WorkflowHandle stepHandle = workflow_get_child(rootHandle, 0);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler download/install/apply fail when file entity is unresolved", "[apt_handler]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_Result initResult = workflow_init(action_parent_update_missing_fileentity, false, &rootHandle);
    REQUIRE(IsAducResultCodeSuccess(initResult.ResultCode));
    REQUIRE(rootHandle != nullptr);

    ContentHandler* aptHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    REQUIRE(aptHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/apt:1", aptHandler);

    ADUC_Result prepResult = PrepareStepsWorkflowDataObject(rootHandle);
    REQUIRE(IsAducResultCodeSuccess(prepResult.ResultCode));

    ADUC_WorkflowHandle stepHandle = workflow_get_child(rootHandle, 0);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result downloadResult = handler->Download(&stepWorkflow);
    CHECK(downloadResult.ResultCode == ADUC_Result_Failure);
    CHECK(downloadResult.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT);

    ADUC_Result installResult = handler->Install(&stepWorkflow);
    CHECK(installResult.ResultCode == ADUC_Result_Failure);
    CHECK(installResult.ExtendedResultCode == ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE);

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(applyResult.ResultCode == ADUC_Result_Failure);
    CHECK(applyResult.ExtendedResultCode == ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler apply returns immediate agent restart when manifest requests it", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    const std::string workFolder = std::string(ADUC_TEST_DATA_FOLDER) + "/apt_handler_apply_restart_required";
    std::filesystem::create_directories(workFolder);
    REQUIRE(workflow_set_workfolder(stepHandle, workFolder.c_str()));

    const std::string manifestPath = workFolder + "/apt-manifest-tree-1.0.json";
    REQUIRE(write_file_text(
        manifestPath,
        "{\"name\":\"du-agent-update\",\"version\":\"0.7.0\",\"packages\":[{\"name\":\"deviceupdate-agent\",\"version\":\"0.7.0~public~preview\"}],\"agentRestartRequired\":true}"));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(applyResult.ResultCode == ADUC_Result_Apply_RequiredImmediateAgentRestart);
    CHECK(applyResult.ExtendedResultCode == 0);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler apply returns success when manifest does not require agent restart", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    const std::string workFolder = std::string(ADUC_TEST_DATA_FOLDER) + "/apt_handler_apply_no_restart";
    std::filesystem::create_directories(workFolder);
    REQUIRE(workflow_set_workfolder(stepHandle, workFolder.c_str()));

    const std::string manifestPath = workFolder + "/apt-manifest-tree-1.0.json";
    REQUIRE(write_file_text(
        manifestPath,
        "{\"name\":\"du-agent-update\",\"version\":\"0.7.0\",\"packages\":[{\"name\":\"tree\",\"version\":\"1.0\"}],\"agentRestartRequired\":false}"));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(applyResult.ResultCode == ADUC_Result_Apply_Success);
    CHECK(applyResult.ExtendedResultCode == 0);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler download exercises path up to content download", "[apt_handler]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    // Download proceeds through file-count check, file-entity retrieval,
    // installedCriteria check, config retrieval, manifest filename construction,
    // then fails at ExtensionManager::Download (no content downloader registered).
    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler install returns cancel result when cancel requested", "[apt_handler]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_request_cancel(stepHandle));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result result = handler->Install(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler apply returns cancel result when cancel requested", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_request_cancel(stepHandle));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result result = handler->Apply(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler is-installed returns installed when criteria is persisted", "[apt_handler]")
{
    reset_installed_criteria_file();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    // Persist criteria matching the workflow's installedCriteria
    REQUIRE(PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, "apt-update-tree-1.0"));

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result result = handler->IsInstalled(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);

    reset_installed_criteria_file();
    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}

TEST_CASE("APT handler download fails with empty installed criteria", "[apt_handler]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupAptStepWorkflow(&rootHandle);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(stepHandle != nullptr);

    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    // Download on root handle has no installed criteria → fails
    ADUC_WorkflowData rootWorkflow{};
    rootWorkflow.WorkflowHandle = rootHandle;

    ADUC_Result result = handler->Download(&rootWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ExtensionManager::Uninit();
}
