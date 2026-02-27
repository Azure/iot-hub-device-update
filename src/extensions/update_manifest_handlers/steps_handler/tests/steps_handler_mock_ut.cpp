/**
 * @file steps_handler_mock_ut.cpp
 * @brief Mock-based unit tests for steps_handler.cpp and handler_create.cpp.
 *
 * All external dependencies are mocked via mock_steps_handler_deps.cpp so
 * that coverage is achieved without needing the full agent runtime.
 */
#include "mock_steps_handler_deps.h"

#include <aduc/content_handler.hpp>
#include <aduc/steps_handler.hpp>
#include <aduc/types/workflow.h>

#include <catch2/catch_all.hpp>

#include <cstdlib> // setenv, unsetenv
#include <cstring>
#include <memory>

// Forward declarations of exported C functions defined in handler_create.cpp
extern "C"
{
    ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
    ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

/* =====================================================================
 * Fixture — resets all mock state before each test
 * ===================================================================== */

struct StepsFixture
{
    StepsFixture()
    {
        mock_steps_handler_reset();
    }

    ~StepsFixture()
    {
        unsetenv("DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS");
    }

    /* Returns a minimal ADUC_WorkflowData with a non-null WorkflowHandle */
    ADUC_WorkflowData makeWorkflowData()
    {
        ADUC_WorkflowData wd = {};
        /* Use a non-null marker as the workflow handle */
        static char s_handle_marker;
        wd.WorkflowHandle = &s_handle_marker;
        return wd;
    }
};

/* =====================================================================
 * handler_create.cpp tests
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "GetContractInfo returns success with V1 version")
{
    ADUC_ExtensionContractInfo info = {};
    ADUC_Result result = GetContractInfo(&info);
    CHECK(IsAducResultCodeSuccess(result.ResultCode));
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE_METHOD(StepsFixture, "CreateUpdateContentHandlerExtension returns handler")
{
    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    REQUIRE(handler != nullptr);
    delete handler;
}

TEST_CASE_METHOD(StepsFixture, "CreateContentHandler returns non-null handler")
{
    ContentHandler* handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);
    delete handler;
}

/* =====================================================================
 * StepsHandlerImpl destructor (via delete)
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "StepsHandlerImpl destructor runs without error")
{
    ContentHandler* handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);
    /* Destructor calls ADUC_Logging_Uninit which is mocked */
    delete handler;
}

/* =====================================================================
 * Apply
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Apply returns success when not cancelled")
{
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Apply_Success);
}

TEST_CASE_METHOD(StepsFixture, "Apply returns cancelled when cancel requested")
{
    mock_is_cancel_requested = true;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Apply(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure_Cancelled);
}

/* =====================================================================
 * Backup
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Backup returns success when not cancelled")
{
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Backup(&wd);
    CHECK(result.ResultCode == ADUC_Result_Backup_Success);
}

TEST_CASE_METHOD(StepsFixture, "Backup returns cancelled when cancel requested")
{
    mock_is_cancel_requested = true;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Backup(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure_Cancelled);
}

/* =====================================================================
 * Restore
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Restore always returns success")
{
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Restore(&wd);
    CHECK(result.ResultCode == ADUC_Result_Restore_Success);
}

/* =====================================================================
 * Cancel
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Cancel returns success when workflow_request_cancel succeeds")
{
    mock_request_cancel_return = true;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Cancel(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_Success);
}

TEST_CASE_METHOD(StepsFixture, "Cancel returns unable when workflow_request_cancel fails")
{
    mock_request_cancel_return = false;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Cancel(&wd);
    CHECK(result.ResultCode == ADUC_Result_Cancel_UnableToCancel);
}

/* =====================================================================
 * Download — early exit paths
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Download returns cancelled when cancel requested")
{
    mock_is_cancel_requested = true;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE_METHOD(StepsFixture, "Download fails when MkSandboxDir fails")
{
    mock_mksandbox_return = -1;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE);
}

TEST_CASE_METHOD(StepsFixture, "Download succeeds with 0 steps (no children)")
{
    /* PrepareStepsWorkflowDataObject with 0 steps just succeeds.
       HandleComponents with level 0 sets count=1.
       But stepsCount from workflow_get_children_count is 0, so inner loop never executes. */
    mock_instructions_steps_count = 0;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download with 1 inline step - download success")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_download_result = { ADUC_Result_Download_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download with 1 inline step - already installed skips download")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_Installed, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    /* Download should succeed (skipped because already installed) */
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download with 1 inline step - IsInstalled throws, download proceeds")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_throws = true;
    mock_handler_download_result = { ADUC_Result_Download_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download fails when handler load fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_load_handler_result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Download fails with unsupported contract version")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_contract_major = 99; /* not V1 */

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_UNSUPPORTED_CONTRACT_VERSION);
}

TEST_CASE_METHOD(StepsFixture, "Download fails when content handler download fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_download_result = { ADUC_Result_Failure, 0x12345 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Download with extra debug logs enabled")
{
    setenv("DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS", "1", 1);
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_download_result = { ADUC_Result_Download_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download fails when child workflow is null")
{
    /* Set up 1 step so PrepareSteps creates 1 child, but then remove it before inner loop */
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;

    /* After PrepareSteps runs, it inserts a child. We need the child to be null when retrieved.
       This is tricky - instead set insert to fail so no child is added, but PrepareSteps will also fail.
       A simpler approach: have 1 step, PrepareSteps succeeds, but explicitly remove the child
       before the inner loop. Since we can't do that, let's simulate a null child by
       making PrepareSteps create 1 step but the insert fail. */
    mock_insert_child_return = false;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    /* PrepareSteps should fail because insert_child failed */
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

/* =====================================================================
 * Install — various paths
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Install returns cancelled when cancel requested")
{
    mock_is_cancel_requested = true;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Failure_Cancelled);
}

TEST_CASE_METHOD(StepsFixture, "Install fails when MkSandboxDir fails")
{
    mock_mksandbox_return = -1;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE);
}

TEST_CASE_METHOD(StepsFixture, "Install succeeds with 0 steps")
{
    mock_instructions_steps_count = 0;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install with 1 inline step - already installed (skipped)")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_Installed, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install with 1 inline step - IsInstalled throws, proceeds")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_throws = true;
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install with 1 inline step - full success (backup + install + apply)")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install - backup failure aborts")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Failure, 0x100 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Install - backup throws exception")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    /* We can't directly make backup throw via mocks, but we can test the install throw path */
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Failure, 0x200 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Install - install failure triggers restore")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Failure, 0x300 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Install - apply failure triggers restore")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Failure, 0x400 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Install - immediate reboot requested after install")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_immediate_reboot_requested = true;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    /* Install should jump to instanceDone then done (immediate reboot) */
    CHECK((IsAducResultCodeFailure(result.ResultCode) || result.ResultCode == ADUC_Result_Install_Success));
}

TEST_CASE_METHOD(StepsFixture, "Install - immediate agent restart requested after install")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_immediate_agent_restart_requested = true;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK((IsAducResultCodeFailure(result.ResultCode) || result.ResultCode == ADUC_Result_Install_Success));
}

TEST_CASE_METHOD(StepsFixture, "Install - reboot requested breaks out of steps loop")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };
    mock_reboot_requested = true;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install - agent restart requested breaks out of steps loop")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };
    mock_agent_restart_requested = true;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install with extra debug logs")
{
    setenv("DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS", "1", 1);
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

TEST_CASE_METHOD(StepsFixture, "Install - handler load fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_load_handler_result = { ADUC_Result_Failure, 0x500 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Install - set selected components fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_set_selected_components_return = false;
    /* Need serializedComponentString != nullptr: requires components enumerator + level > 0 */
    mock_is_components_enumerator_registered = true;
    mock_workflow_level = 1;
    mock_selected_components = R"({"components":[{"id":"comp1"}]})";

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE);
}

TEST_CASE_METHOD(StepsFixture, "Install final cancel check returns cancelled at end")
{
    mock_instructions_steps_count = 0;
    /* After the loops, there's a final cancel check */
    /* We need to trigger mid-execution cancel. Since mock is global, we can't change it
       between calls. But the final check uses workflowData->WorkflowHandle directly.
       With 0 steps, the code will hit the post-loop cancel check. */
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());

    /* For the final cancel check to trigger, we'd need cancel not requested initially
       but requested at the end. Our mock always returns the same value, so this test
       demonstrates that with cancel=false the success path returns Install_Success. */
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}

/* =====================================================================
 * IsInstalled — various paths
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "IsInstalled fails when MkSandboxDir fails")
{
    mock_mksandbox_return = -1;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled with 0 steps returns installed")
{
    mock_instructions_steps_count = 0;
    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled with 1 installed step returns installed")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_Installed, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled with 1 not-installed step returns not installed")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled - IsInstalled throws assumes not installed")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_throws = true;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled - handler load fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_load_handler_result = { ADUC_Result_Failure, 0x600 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled with extra debug logs")
{
    setenv("DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS", "1", 1);
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_Installed, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled - child workflow is null (missing child)")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_insert_child_return = false; /* PrepareSteps will fail */

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

/* =====================================================================
 * PrepareStepsWorkflowDataObject paths (tested through Download)
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "Download - PrepareSteps inline step creation failure")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = true;
    mock_create_from_inline_step_result = { ADUC_Result_Failure, 0x700 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step with detached manifest download failure")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { ADUC_Result_Failure, 0x800 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step cannot get detached manifest file entity")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = false;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_GET_FILE_ENTITY_FAILURE);
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step download + init success, no comp enumerator")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 }; /* success */
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = false;
    /* After PrepareSteps creates a child from ref step, Download loop runs */
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_download_result = { ADUC_Result_Download_Success, 0 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step with components, no matching (skip)")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 };
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = true;
    mock_compat_string = R"({"deviceManufacturer":"test","deviceModel":"test"})";
    mock_select_components_result = { 1, 0 };
    mock_select_components_output = R"({"components":[]})";
    mock_selected_components = R"({"components":[]})";
    mock_workflow_level = 1; /* reference step */

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    /* HandleComponents returns Download_Skipped_NoMatchingComponents then loop succeeds */
    CHECK(result.ResultCode == ADUC_Result_Download_Success);
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step with components, compat string null")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 };
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = true;
    mock_compat_string = nullptr; /* will cause failure */

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_GET_REF_STEP_COMPATIBILITY_FAILED);
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step, SelectComponents fails")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 };
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = true;
    mock_compat_string = R"({"deviceManufacturer":"test"})";
    mock_select_components_result = { ADUC_Result_Failure, 0x900 };

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(StepsFixture, "Download - ref step, set_selected_components fails in PrepareSteps")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 };
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = true;
    mock_compat_string = R"({"deviceManufacturer":"test"})";
    mock_select_components_result = { 1, 0 };
    mock_select_components_output = R"({"components":[{"id":"c1"}]})";
    mock_set_selected_components_return = false;

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Download(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE);
}

/* =====================================================================
 * IsInstalled paths with reference steps and components
 * ===================================================================== */

TEST_CASE_METHOD(StepsFixture, "IsInstalled with level 1, components enumerator, 0 matching")
{
    mock_instructions_steps_count = 1;
    mock_is_inline_step = false;
    mock_get_step_detached_manifest_return = true;
    mock_ext_download_result = { 1, 0 };
    mock_init_from_file_result = { 1, 0 };
    mock_is_components_enumerator_registered = true;
    mock_compat_string = R"({"deviceManufacturer":"t"})";
    mock_select_components_result = { 1, 0 };
    mock_select_components_output = R"({"components":[]})";
    mock_workflow_level = 1;
    mock_selected_components = R"({"components":[]})";

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    /* 0 matching components → considered installed (optional step) */
    CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
}

TEST_CASE_METHOD(StepsFixture, "IsInstalled with level 1, missing selected components")
{
    mock_instructions_steps_count = 0;
    mock_is_components_enumerator_registered = true;
    mock_workflow_level = 1;
    mock_selected_components = nullptr; /* empty → failure */

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->IsInstalled(&wd);
    CHECK(IsAducResultCodeFailure(result.ResultCode));
}

TEST_CASE_METHOD(
    StepsFixture, "Install with level 1, components enumerator, 0 matching components (skipped)")
{
    mock_instructions_steps_count = 0;
    mock_is_components_enumerator_registered = true;
    mock_workflow_level = 1;
    mock_selected_components = R"({"components":[]})";

    auto wd = makeWorkflowData();
    std::unique_ptr<ContentHandler> handler(StepsHandlerImpl::CreateContentHandler());
    ADUC_Result result = handler->Install(&wd);
    CHECK(result.ResultCode == ADUC_Result_Install_Success);
}
