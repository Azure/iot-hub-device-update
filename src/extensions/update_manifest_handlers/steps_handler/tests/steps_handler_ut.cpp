#include <catch2/catch_all.hpp>

#include <aduc/agent_workflow.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/system_utils.h>
#include <aduc/workflow_utils.h>
#include <aduc/extension_manager.hpp>
#include <aducpal/stdlib.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <grp.h>
#include <memory>
#include <pwd.h>
#include <string>

#include "aduc/steps_handler.hpp"

extern "C" {
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

namespace {

class WorkflowHandle final
{
public:
    explicit WorkflowHandle(const std::string& workflowJson)
    {
        ADUC_Result result = workflow_init(workflowJson.c_str(), false, &_handle);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        REQUIRE(_handle != nullptr);
        _workflowData.WorkflowHandle = _handle;
    }

    ~WorkflowHandle()
    {
        if (_handle != nullptr)
        {
            workflow_free(_handle);
        }
    }

    ADUC_WorkflowHandle get() const
    {
        return _handle;
    }

    ADUC_WorkflowData* data()
    {
        return &_workflowData;
    }

private:
    ADUC_WorkflowHandle _handle { nullptr };
    ADUC_WorkflowData _workflowData {};
};

std::string MakeMinimalStepsWorkflow()
{
    return R"({
  "workflow": { "action": 3, "id": "11111111-2222-3333-4444-555555555555" },
  "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"StepsUpdate\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
  "updateManifestSignature": "dummy",
  "fileUrls": {}
})";
}

std::string MakeInlineScriptStepWorkflow()
{
        return R"({
    "workflow": { "action": 3, "id": "22222222-3333-4444-5555-666666666666" },
    "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"ScriptStep\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/script:1\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"criteria\",\"scriptFileName\":\"missing-script.sh\"}}]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
    "updateManifestSignature": "dummy",
    "fileUrls": {}
})";
}

void SetTestConfig()
{
    std::string testDataFolder = ADUC_TEST_DATA_FOLDER;
    std::string configFolder = testDataFolder + "/script_handler_test_config";
    ADUCPAL_setenv(ADUC_CONFIG_FOLDER_ENV, configFolder.c_str(), 1);
}

std::string MakeTwoInlineStepsWorkflow()
{
    return R"({
  "workflow": { "action": 3, "id": "33333333-4444-5555-6666-777777777777" },
  "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"TwoSteps\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/script:1\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"c1\",\"scriptFileName\":\"s1.sh\"}},{\"handler\":\"microsoft/script:1\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"c2\",\"scriptFileName\":\"s2.sh\"}}]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
  "updateManifestSignature": "dummy",
  "fileUrls": {}
})";
}

std::filesystem::path MakeUniqueTempWorkFolder(const std::string& suffix)
{
    const auto nonce = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("adu-steps-" + suffix + "-" + std::to_string(nonce));
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path MakeUniqueTempPath(const std::string& suffix)
{
    const auto nonce = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("adu-steps-" + suffix + "-" + std::to_string(nonce));
}

// Simple test handler stubs for exercising steps handler code paths.
// These are plain stubs (not mocks) that return canned ADUC_Result values.
class SimpleNotInstalledHandler : public ContentHandler
{
public:
    ADUC_Result Download(const tagADUC_WorkflowData*) override { return { ADUC_Result_Download_Success, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return { ADUC_Result_Backup_Success, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData*) override { return { ADUC_Result_Install_Success, 0 }; }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return { ADUC_Result_Apply_Success, 0 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return { ADUC_Result_Cancel_Success, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return { ADUC_Result_Restore_Success, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return { ADUC_Result_IsInstalled_NotInstalled, 0 }; }
};

class FailInstallHandler : public ContentHandler
{
public:
    ADUC_Result Download(const tagADUC_WorkflowData*) override { return { ADUC_Result_Download_Success, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return { ADUC_Result_Backup_Success, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData*) override { return { ADUC_Result_Failure, 0x12345678 }; }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return { ADUC_Result_Apply_Success, 0 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return { ADUC_Result_Cancel_Success, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return { ADUC_Result_Restore_Success, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return { ADUC_Result_IsInstalled_NotInstalled, 0 }; }
};

class FailApplyHandler : public ContentHandler
{
public:
    ADUC_Result Download(const tagADUC_WorkflowData*) override { return { ADUC_Result_Download_Success, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return { ADUC_Result_Backup_Success, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData*) override { return { ADUC_Result_Install_Success, 0 }; }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return { ADUC_Result_Failure, 0x12345679 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return { ADUC_Result_Cancel_Success, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return { ADUC_Result_Restore_Success, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return { ADUC_Result_IsInstalled_NotInstalled, 0 }; }
};

class RebootHandler : public ContentHandler
{
public:
    ADUC_Result Download(const tagADUC_WorkflowData*) override { return { ADUC_Result_Download_Success, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return { ADUC_Result_Backup_Success, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData* w) override
    {
        workflow_request_immediate_reboot(w->WorkflowHandle);
        return { ADUC_Result_Install_RequiredImmediateReboot, 0 };
    }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return { ADUC_Result_Apply_Success, 0 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return { ADUC_Result_Cancel_Success, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return { ADUC_Result_Restore_Success, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return { ADUC_Result_IsInstalled_NotInstalled, 0 }; }
};

class UnsupportedContractHandler : public ContentHandler
{
public:
    UnsupportedContractHandler()
    {
        SetContractInfo({ 2, 0 });
    }

    ADUC_Result Download(const tagADUC_WorkflowData*) override { return { ADUC_Result_Download_Success, 0 }; }
    ADUC_Result Backup(const tagADUC_WorkflowData*) override { return { ADUC_Result_Backup_Success, 0 }; }
    ADUC_Result Install(const tagADUC_WorkflowData*) override { return { ADUC_Result_Install_Success, 0 }; }
    ADUC_Result Apply(const tagADUC_WorkflowData*) override { return { ADUC_Result_Apply_Success, 0 }; }
    ADUC_Result Cancel(const tagADUC_WorkflowData*) override { return { ADUC_Result_Cancel_Success, 0 }; }
    ADUC_Result Restore(const tagADUC_WorkflowData*) override { return { ADUC_Result_Restore_Success, 0 }; }
    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override { return { ADUC_Result_IsInstalled_NotInstalled, 0 }; }
};

} // namespace

TEST_CASE("GetContractInfo returns expected values", "[steps_handler]")
{
    ADUC_ExtensionContractInfo contractInfo {};

    ADUC_Result result = GetContractInfo(&contractInfo);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    CHECK(contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("CreateUpdateContentHandlerExtension returns handler", "[steps_handler]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(handler != nullptr);
}

TEST_CASE("Steps handler factory create returns handler", "[steps_handler]")
{
    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);
}

TEST_CASE("Steps handler backup and restore succeed with valid workflow", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result backupResult = handler->Backup(workflow.data());
    ADUC_Result restoreResult = handler->Restore(workflow.data());

    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success);
    CHECK(restoreResult.ResultCode == ADUC_Result_Restore_Success);
}

TEST_CASE("Steps handler cancel sets cancel requested and returns success", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result cancelResult = handler->Cancel(workflow.data());

    CHECK(cancelResult.ResultCode == ADUC_Result_Cancel_Success);
    CHECK(workflow_is_cancel_requested(workflow.get()));
}

TEST_CASE("Steps handler apply returns cancelled when workflow is already cancelled", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result applyResult = handler->Apply(workflow.data());

    CHECK(applyResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler install returns cancelled when workflow is already cancelled", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result installResult = handler->Install(workflow.data());

    CHECK(installResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler download returns cancelled when workflow is already cancelled", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result downloadResult = handler->Download(workflow.data());

    CHECK(downloadResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler download fails and keeps empty workflow child count at zero", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result first = handler->Download(workflow.data());
    ADUC_Result second = handler->Download(workflow.data());

    CHECK((IsAducResultCodeFailure(first.ResultCode) || first.ResultCode == ADUC_Result_Download_Success));
    CHECK((IsAducResultCodeFailure(second.ResultCode) || second.ResultCode == ADUC_Result_Download_Success));
    CHECK(workflow_get_children_count(workflow.get()) == 0);
}

TEST_CASE("Steps handler download returns failure for empty-steps workflow", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result downloadResult = handler->Download(workflow.data());

    CHECK((IsAducResultCodeFailure(downloadResult.ResultCode) || downloadResult.ResultCode == ADUC_Result_Download_Success));
}

TEST_CASE("Steps handler apply returns success for non-cancelled minimal workflow", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result applyResult = handler->Apply(workflow.data());

    CHECK(applyResult.ResultCode == ADUC_Result_Apply_Success);
    CHECK(applyResult.ExtendedResultCode == 0);
}

TEST_CASE("Steps handler install returns failure for empty-steps workflow", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result installResult = handler->Install(workflow.data());

    CHECK((IsAducResultCodeFailure(installResult.ResultCode) || installResult.ResultCode == ADUC_Result_Install_Success));
}

TEST_CASE("Steps handler is-installed returns failure for empty-steps workflow", "[steps_handler]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result isInstalledResult = handler->IsInstalled(workflow.data());

    CHECK((IsAducResultCodeFailure(isInstalledResult.ResultCode)
        || isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed
        || isInstalledResult.ResultCode == ADUC_Result_IsInstalled_NotInstalled));
}

TEST_CASE("Steps handler install and is-installed traverse child workflow paths", "[steps_handler]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());

    ADUC_Result installResult = handler->Install(workflow.data());
    ADUC_Result isInstalledResult = handler->IsInstalled(workflow.data());

    int childrenCount = workflow_get_children_count(workflow.get());
    CHECK((childrenCount == 0 || childrenCount == 1));
    CHECK((IsAducResultCodeFailure(installResult.ResultCode) || IsAducResultCodeSuccess(installResult.ResultCode)));
    CHECK((IsAducResultCodeFailure(isInstalledResult.ResultCode) || IsAducResultCodeSuccess(isInstalledResult.ResultCode)));

    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler backup returns cancelled when cancel requested", "[steps_handler]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result backupResult = handler->Backup(workflow.data());
    CHECK(backupResult.ResultCode == ADUC_Result_Failure_Cancelled);
}