/**
 * @file apt_handler_ut.cpp
 * @brief Non-mock unit tests for apt_handler.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apt_handler.hpp"
#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aducpal/stdlib.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>

#include <cstdio>
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

TEST_CASE("APT handler exported functions smoke test", "[apt_handler][non_mock]")
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

TEST_CASE("APT handler backup and restore return unsupported", "[apt_handler][non_mock]")
{
    std::unique_ptr<ContentHandler> handler(AptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData workflowData{};
    ADUC_Result backupResult = handler->Backup(&workflowData);
    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success_Unsupported);

    ADUC_Result restoreResult = handler->Restore(&workflowData);
    CHECK(restoreResult.ResultCode == ADUC_Result_Restore_Success_Unsupported);
}

TEST_CASE("APT handler cancel/download paths on real workflows", "[apt_handler][non_mock]")
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

TEST_CASE("APT handler install/apply parse-failure paths on missing manifest", "[apt_handler][non_mock]")
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

TEST_CASE("APT handler is-installed returns not-installed without persisted criteria", "[apt_handler][non_mock]")
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

TEST_CASE("APT handler install/apply/cancel paths with local apt manifest", "[apt_handler][non_mock]")
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
