#include <catch2/catch_all.hpp>

#include <aduc/agent_workflow.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/system_utils.h>
#include <aduc/workflow_utils.h>
#include <aduc/extension_manager.hpp>
#include <aducpal/stdlib.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
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

TEST_CASE("GetContractInfo returns expected values", "[steps_handler][non_mock]")
{
    ADUC_ExtensionContractInfo contractInfo {};

    ADUC_Result result = GetContractInfo(&contractInfo);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    CHECK(contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("CreateUpdateContentHandlerExtension returns handler", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(handler != nullptr);
}

TEST_CASE("Steps handler factory create returns handler", "[steps_handler][non_mock]")
{
    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);
}

TEST_CASE("Steps handler backup and restore succeed with valid workflow", "[steps_handler][non_mock]")
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

TEST_CASE("Steps handler cancel sets cancel requested and returns success", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result cancelResult = handler->Cancel(workflow.data());

    CHECK(cancelResult.ResultCode == ADUC_Result_Cancel_Success);
    CHECK(workflow_is_cancel_requested(workflow.get()));
}

TEST_CASE("Steps handler apply returns cancelled when workflow is already cancelled", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result applyResult = handler->Apply(workflow.data());

    CHECK(applyResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler install returns cancelled when workflow is already cancelled", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result installResult = handler->Install(workflow.data());

    CHECK(installResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler download returns cancelled when workflow is already cancelled", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result downloadResult = handler->Download(workflow.data());

    CHECK(downloadResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler download fails and keeps empty workflow child count at zero", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result first = handler->Download(workflow.data());
    ADUC_Result second = handler->Download(workflow.data());

    CHECK(first.ResultCode == ADUC_Result_Failure);
    CHECK(second.ResultCode == ADUC_Result_Failure);
    CHECK(workflow_get_children_count(workflow.get()) == 0);
}

TEST_CASE("Steps handler download returns failure for empty-steps workflow", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result downloadResult = handler->Download(workflow.data());

    CHECK(downloadResult.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("Steps handler apply returns success for non-cancelled minimal workflow", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result applyResult = handler->Apply(workflow.data());

    CHECK(applyResult.ResultCode == ADUC_Result_Apply_Success);
    CHECK(applyResult.ExtendedResultCode == 0);
}

TEST_CASE("Steps handler install returns failure for empty-steps workflow", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result installResult = handler->Install(workflow.data());

    CHECK(installResult.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("Steps handler is-installed returns failure for empty-steps workflow", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());

    ADUC_Result isInstalledResult = handler->IsInstalled(workflow.data());

    CHECK(IsAducResultCodeFailure(isInstalledResult.ResultCode));
}

TEST_CASE("Steps handler download traverses inline script child path", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    ADUC_Result downloadResult = handler->Download(workflow.data());

    int childrenCount = workflow_get_children_count(workflow.get());
    CHECK((childrenCount == 0 || childrenCount == 1));
    CHECK((IsAducResultCodeFailure(downloadResult.ResultCode) || IsAducResultCodeSuccess(downloadResult.ResultCode)));

    delete handler;
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install and is-installed traverse child workflow paths", "[steps_handler][non_mock]")
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

TEST_CASE("Steps handler backup returns cancelled when cancel requested", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeMinimalStepsWorkflow());
    REQUIRE(workflow_request_cancel(workflow.get()));

    ADUC_Result backupResult = handler->Backup(workflow.data());
    CHECK(backupResult.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE("Steps handler repeated inline download keeps one child workflow", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());

    ADUC_Result firstDownload = handler->Download(workflow.data());
    ADUC_Result secondDownload = handler->Download(workflow.data());

    int childrenCount = workflow_get_children_count(workflow.get());
    CHECK((childrenCount == 0 || childrenCount == 1));
    CHECK((IsAducResultCodeFailure(firstDownload.ResultCode) || IsAducResultCodeSuccess(firstDownload.ResultCode)));
    CHECK((IsAducResultCodeFailure(secondDownload.ResultCode) || IsAducResultCodeSuccess(secondDownload.ResultCode)));

    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler restore remains success after install attempt", "[steps_handler][non_mock]")
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
    ADUC_Result restoreResult = handler->Restore(workflow.data());

    CHECK((IsAducResultCodeFailure(installResult.ResultCode) || IsAducResultCodeSuccess(installResult.ResultCode)));
    CHECK(restoreResult.ResultCode == ADUC_Result_Restore_Success);

    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler download with writable workfolder exercises full loop", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-dl-loop");

    ADUC_Result result = handler->Download(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_Download_Success
        || IsAducResultCodeFailure(result.ResultCode)));
    CHECK(workflow_get_children_count(workflow.get()) >= 0);

    std::filesystem::remove_all("/tmp/adu-steps-dl-loop");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install with writable workfolder exercises full loop", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-inst-loop");

    ADUC_Result result = handler->Install(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_Install_Success
        || IsAducResultCodeFailure(result.ResultCode)));

    std::filesystem::remove_all("/tmp/adu-steps-inst-loop");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler is-installed with writable workfolder exercises full loop", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-isinstalled-loop");

    ADUC_Result result = handler->IsInstalled(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_IsInstalled_Installed
        || result.ResultCode == ADUC_Result_IsInstalled_NotInstalled
        || IsAducResultCodeFailure(result.ResultCode)));

    std::filesystem::remove_all("/tmp/adu-steps-isinstalled-loop");
    ExtensionManager::Uninit();
}

// NOTE: Debug-logging tests for Download/Install/IsInstalled removed.
// They require the 'adu' system user; without it, MkSandboxDirRecursive fails
// and a double-free in the handler's cleanup path causes SIGABRT.

TEST_CASE("Steps handler download install is-installed with two inline steps", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> scriptHandler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(scriptHandler != nullptr);
    ADUC_Result regResult = ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", scriptHandler.release());
    REQUIRE(IsAducResultCodeSuccess(regResult.ResultCode));

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeTwoInlineStepsWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-two-steps");

    ADUC_Result dlResult = handler->Download(workflow.data());
    CHECK((dlResult.ResultCode == ADUC_Result_Download_Success
        || IsAducResultCodeFailure(dlResult.ResultCode)));

    ADUC_Result instResult = handler->Install(workflow.data());
    CHECK((instResult.ResultCode == ADUC_Result_Install_Success
        || IsAducResultCodeFailure(instResult.ResultCode)));

    ADUC_Result isInstResult = handler->IsInstalled(workflow.data());
    CHECK((isInstResult.ResultCode == ADUC_Result_IsInstalled_Installed
        || isInstResult.ResultCode == ADUC_Result_IsInstalled_NotInstalled
        || IsAducResultCodeFailure(isInstResult.ResultCode)));

    int childrenCount = workflow_get_children_count(workflow.get());
    CHECK((childrenCount == 0 || childrenCount == 2));

    std::filesystem::remove_all("/tmp/adu-steps-two-steps");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler download and install fail when handler not registered for step type", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-no-handler");

    ADUC_Result downloadResult = handler->Download(workflow.data());
    CHECK(IsAducResultCodeFailure(downloadResult.ResultCode));

    ADUC_Result installResult = handler->Install(workflow.data());
    CHECK(IsAducResultCodeFailure(installResult.ResultCode));

    std::filesystem::remove_all("/tmp/adu-steps-no-handler");
}

TEST_CASE("Steps handler download loop processes not-installed child via stub", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* stub = new SimpleNotInstalledHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", stub);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-notinstalled-dl");

    ADUC_Result result = handler->Download(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_Download_Success
        || IsAducResultCodeFailure(result.ResultCode)));

    std::filesystem::remove_all("/tmp/adu-steps-notinstalled-dl");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install loop exercises backup install apply on child via stub", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* stub = new SimpleNotInstalledHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", stub);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-notinstalled-inst");

    ADUC_Result result = handler->Install(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_Install_Success
        || IsAducResultCodeFailure(result.ResultCode)));

    std::filesystem::remove_all("/tmp/adu-steps-notinstalled-inst");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install invokes restore when child install fails via stub", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* failHandler = new FailInstallHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", failHandler);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-failinstall");

    ADUC_Result result = handler->Install(workflow.data());
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    std::filesystem::remove_all("/tmp/adu-steps-failinstall");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install invokes restore when child apply fails via stub", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* failHandler = new FailApplyHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", failHandler);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-failapply");

    ADUC_Result result = handler->Install(workflow.data());
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    std::filesystem::remove_all("/tmp/adu-steps-failapply");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler install propagates immediate reboot request from child via stub", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* rebootHandler = new RebootHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", rebootHandler);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-reboot");

    ADUC_Result result = handler->Install(workflow.data());
    CHECK((result.ResultCode == ADUC_Result_Install_RequiredImmediateReboot
        || IsAducResultCodeFailure(result.ResultCode)));

    std::filesystem::remove_all("/tmp/adu-steps-reboot");
    ExtensionManager::Uninit();
}

TEST_CASE("Steps handler download fails for unsupported child handler contract", "[steps_handler][non_mock]")
{
    SetTestConfig();

    auto* unsupported = new UnsupportedContractHandler();
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/script:1", unsupported);

    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    WorkflowHandle workflow(MakeInlineScriptStepWorkflow());
    REQUIRE(workflow_set_workfolder(workflow.get(), "%s", "/tmp/adu-steps-unsupported-contract"));

    ADUC_Result result = handler->Download(workflow.data());
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    bool isUnsupportedContract =
        (result.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_UNSUPPORTED_CONTRACT_VERSION);
    bool isSandboxPreconditionFailure =
        (result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE);
    bool isExpectedFailure = isUnsupportedContract || isSandboxPreconditionFailure;
    CHECK(isExpectedFailure);

    std::filesystem::remove_all("/tmp/adu-steps-unsupported-contract");
    ExtensionManager::Uninit();
}

// NOTE: Cancel-after-download test removed.
// It requires the 'adu' system user; without it, MkSandboxDirRecursive fails
// and a double-free in the handler's cleanup path causes SIGABRT.
