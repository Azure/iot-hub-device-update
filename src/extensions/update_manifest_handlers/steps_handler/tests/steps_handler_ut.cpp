#include <catch2/catch_all.hpp>

#include <aduc/agent_workflow.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/workflow_utils.h>
#include <aducpal/stdlib.h>

#include <cstdlib>
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

void SetTestConfig()
{
    std::string testDataFolder = ADUC_TEST_DATA_FOLDER;
    std::string configFolder = testDataFolder + "/script_handler_test_config";
    ADUCPAL_setenv("ADUC_CONFIG_FOLDER_ENV", configFolder.c_str(), 1);
}

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
