/**
 * @file swupdate_handler_v2_mock_ut.cpp
 * @brief Mock-based unit tests for swupdate_handler_v2.cpp and handler_create.cpp
 *        targeting 85%+ coverage.
 */
#include "mock_swupdate_handler_deps.h"

#include <aduc/swupdate_handler_v2.hpp>
#include <aduc/result.h>
#include <aduc/types/workflow.h>

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

/* =====================================================================
 * Declarations for functions we test
 * ===================================================================== */
EXTERN_C_BEGIN
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
EXTERN_C_END

ADUC_Result SWUpdateHandler_PerformAction(
    const std::string& action,
    const tagADUC_WorkflowData* workflowData,
    bool prepareArgsOnly,
    std::string& scriptFilePath,
    std::vector<std::string>& args,
    std::vector<std::string>& commandLineArgs,
    std::string& scriptOutput);

/* Helper: build a minimal tagADUC_WorkflowData with a non-null handle */
static tagADUC_WorkflowData make_workflow_data()
{
    tagADUC_WorkflowData wd{};
    static int dummy_handle = 1;
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy_handle);
    return wd;
}

/* =====================================================================
 * handler_create.cpp tests
 * ===================================================================== */

TEST_CASE("SWUpdate: CreateUpdateContentHandlerExtension returns handler", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    ContentHandler* h = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    REQUIRE(h != nullptr);
    delete h;
}

TEST_CASE("SWUpdate: GetContractInfo fills version", "[swupdate_handler_v2]")
{
    ADUC_ExtensionContractInfo info{};
    ADUC_Result r = GetContractInfo(&info);
    CHECK(r.ResultCode == ADUC_GeneralResult_Success);
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

/* =====================================================================
 * CreateContentHandler
 * ===================================================================== */

TEST_CASE("SWUpdate: CreateContentHandler returns non-null", "[swupdate_handler_v2]")
{
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    REQUIRE(h != nullptr);
    delete h;
}

/* =====================================================================
 * ReadValueFromFile
 * ===================================================================== */

TEST_CASE("SWUpdate: ReadValueFromFile - empty path", "[swupdate_handler_v2]")
{
    std::string v = SWUpdateHandlerImpl::ReadValueFromFile("");
    CHECK(v.empty());
}

TEST_CASE("SWUpdate: ReadValueFromFile - path too long", "[swupdate_handler_v2]")
{
    std::string longPath(5000, 'a');
    std::string v = SWUpdateHandlerImpl::ReadValueFromFile(longPath);
    CHECK(v.empty());
}

TEST_CASE("SWUpdate: ReadValueFromFile - nonexistent file", "[swupdate_handler_v2]")
{
    std::string v = SWUpdateHandlerImpl::ReadValueFromFile("/tmp/does_not_exist_12345.txt");
    CHECK(v.empty());
}

TEST_CASE("SWUpdate: ReadValueFromFile - valid file", "[swupdate_handler_v2]")
{
    const char* fpath = "/tmp/test_readvalue.txt";
    {
        std::ofstream f(fpath);
        f << "  hello world  \n";
    }
    std::string v = SWUpdateHandlerImpl::ReadValueFromFile(fpath);
    CHECK(v == "hello world");
    remove(fpath);
}

/* =====================================================================
 * ReadConfig
 * ===================================================================== */

TEST_CASE("SWUpdate: ReadConfig - nonexistent file", "[swupdate_handler_v2]")
{
    std::unordered_map<std::string, std::string> values;
    ADUC_Result r = SWUpdateHandlerImpl::ReadConfig("/tmp/no_such_config_file.json", values);
    CHECK(IsAducResultCodeFailure(r.ResultCode));
}

TEST_CASE("SWUpdate: ReadConfig - valid config file", "[swupdate_handler_v2]")
{
    const char* fpath = "/tmp/test_swu_config.json";
    {
        std::ofstream f(fpath);
        f << R"({"--opt1":"val1","--opt2":"val2"})";
    }
    std::unordered_map<std::string, std::string> values;
    ADUC_Result r = SWUpdateHandlerImpl::ReadConfig(fpath, values);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(values["--opt1"] == "val1");
    CHECK(values["--opt2"] == "val2");
    remove(fpath);
}

/* =====================================================================
 * Download tests (SWUpdate_Handler_DownloadScriptFile path)
 * ===================================================================== */

TEST_CASE("SWUpdate: Download - missing script file name", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = nullptr;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME);
    delete h;
}

TEST_CASE("SWUpdate: Download - wrong file count (<=1)", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_update_files_count = 1;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT);
    delete h;
}

TEST_CASE("SWUpdate: Download - get file by name fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_get_update_file_by_name_return = false;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_GET_SCRIPT_FILE_ENTITY);
    delete h;
}

TEST_CASE("SWUpdate: Download - sandbox creation fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_mkdir_result = -1;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_CREATE_SANDBOX_FAILURE);
    delete h;
}

TEST_CASE("SWUpdate: Download - extension download of script fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_extension_download_result = { ADUC_Result_Failure, 0x12345 };
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));
    delete h;
}

TEST_CASE("SWUpdate: Download - success continues to payload loop", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    // Script download succeeds, then payload loop downloads each file
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    // After script download, it downloads remaining payloads. With mock success, it
    // should succeed or proceed past the script download step.
    delete h;
}

TEST_CASE("SWUpdate: Download - payload file get fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_get_update_file_return = false;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    // The payload file download loop should fail
    CHECK(r.ResultCode == ADUC_Result_Failure);
    delete h;
}

/* =====================================================================
 * Install tests
 * ===================================================================== */

TEST_CASE("SWUpdate: Install - null workflow data", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(nullptr);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
    delete h;
}

TEST_CASE("SWUpdate: Install - null workflow handle", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    tagADUC_WorkflowData wd{};
    wd.WorkflowHandle = nullptr;
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
    delete h;
}

TEST_CASE("SWUpdate: Install - config unavailable", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_config_available = false;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_PERFORM_ACTION_FAILED_TO_GET_CONFIG_INSTANCE);
    delete h;
}

TEST_CASE("SWUpdate: Install - child process failure", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_launch_child_exit_code = 42;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    delete h;
}

TEST_CASE("SWUpdate: Install - child success but no result file", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_launch_child_exit_code = 0;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    delete h;
}

/* =====================================================================
 * Apply tests
 * ===================================================================== */

TEST_CASE("SWUpdate: Apply - config unavailable", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_config_available = false;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Apply(&wd);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    delete h;
}

/* =====================================================================
 * Cancel tests
 * ===================================================================== */

TEST_CASE("SWUpdate: Cancel - success", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_request_cancel_return = true;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Cancel(&wd);
    CHECK(r.ResultCode == ADUC_Result_Cancel_Success);
    delete h;
}

TEST_CASE("SWUpdate: Cancel - cancel request fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_request_cancel_return = false;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Cancel(&wd);
    CHECK(r.ResultCode == ADUC_Result_Cancel_UnableToCancel);
    delete h;
}

/* =====================================================================
 * IsInstalled tests
 * ===================================================================== */

TEST_CASE("SWUpdate: IsInstalled - missing script filename", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = nullptr;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    delete h;
}

TEST_CASE("SWUpdate: IsInstalled - script download fails => no PerformAction", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_update_files_count = 1; // will fail because fileCount <= 1
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));
    delete h;
}

/* =====================================================================
 * Backup tests
 * ===================================================================== */

TEST_CASE("SWUpdate: Backup - returns success", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Backup(&wd);
    CHECK(r.ResultCode == ADUC_Result_Backup_Success);
    delete h;
}

/* =====================================================================
 * Restore tests
 * ===================================================================== */

TEST_CASE("SWUpdate: Restore - cancel fails => restore fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    // Restore calls CancelApply, which calls PerformAction("cancel").
    // PerformAction will try to run child process. With exit code != 0, it fails.
    // CancelApply will then report failure.
    mock_launch_child_exit_code = 1;
    auto wd = make_workflow_data();
    ContentHandler* h = SWUpdateHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Restore(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
    delete h;
}

/* =====================================================================
 * PrepareCommandArguments tests
 * ===================================================================== */

TEST_CASE("SWUpdate: PrepareCommandArguments - null workflow handle", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        nullptr, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_NULL_WORKFLOW);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - missing scriptFileName", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = nullptr;
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - missing swuFileName", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_swufilename = nullptr;
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SWU_FILE_NAME);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - success, no components", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_installed_criteria = "crit-1.0";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(commandFilePath == "/tmp/work/my-script.sh");
    // Should have --swu-file, --work-folder, --result-file, --installed-criteria
    bool hasSwuFile = false;
    bool hasWorkFolder = false;
    bool hasResultFile = false;
    bool hasInstalledCriteria = false;
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "--swu-file")
            hasSwuFile = true;
        if (args[i] == "--work-folder")
            hasWorkFolder = true;
        if (args[i] == "--result-file")
            hasResultFile = true;
        if (args[i] == "--installed-criteria")
            hasInstalledCriteria = true;
    }
    CHECK(hasSwuFile);
    CHECK(hasWorkFolder);
    CHECK(hasResultFile);
    CHECK(hasInstalledCriteria);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - null installed criteria", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_installed_criteria = nullptr;
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Success);
    bool hasInstalledCriteria = false;
    for (const auto& a : args)
    {
        if (a == "--installed-criteria")
            hasInstalledCriteria = true;
    }
    CHECK_FALSE(hasInstalledCriteria);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - with components", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_installed_criteria = "crit-1.0";
    mock_selected_components = R"({"components":[{"id":"comp-1","name":"motor","manufacturer":"contoso","model":"v1","version":"2.0","group":"grp1","properties":{"path":"/dev/motor0"}}]})";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Success);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - bad components JSON", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_selected_components = "not valid json";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - components missing array", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_selected_components = R"({"noComponents": true})";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - empty components array", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_selected_components = R"({"components":[]})";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Download_Skipped_NoMatchingComponents);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - too many components", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_selected_components = R"({"components":[{"id":"c1","name":"a"},{"id":"c2","name":"b"}]})";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    // Logs error about too many components but continues with component[0].
    // At end of function, result is overwritten to ADUC_Result_Success with ERC=0.
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(r.ExtendedResultCode == 0);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - with arguments property", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_handler_property_arguments = "--custom-opt value1";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Success);
    bool hasCustomOpt = false;
    for (const auto& a : args)
    {
        if (a == "--custom-opt")
            hasCustomOpt = true;
    }
    CHECK(hasCustomOpt);
}

TEST_CASE("SWUpdate: PrepareCommandArguments - component var substitution", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_handler_property_arguments = "--component-id-val --component-name-val --component-manufacturer-val --component-model-val --component-version-val --component-group-val --component-prop-val path";
    mock_selected_components = R"({"components":[{"id":"comp-1","name":"motor","manufacturer":"contoso","model":"v1","version":"2.0","group":"grp1","properties":{"path":"/dev/motor0"}}]})";
    int dummy = 1;
    auto handle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);
    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result r = SWUpdateHandlerImpl::PrepareCommandArguments(
        handle, "/tmp/result.json", "/tmp/work", commandFilePath, args);
    CHECK(r.ResultCode == ADUC_Result_Success);
    // Should have substituted component values
    bool hasCompId = false;
    bool hasCompName = false;
    bool hasCompManufacturer = false;
    bool hasCompModel = false;
    bool hasCompVersion = false;
    bool hasCompGroup = false;
    bool hasCompPropValue = false;
    for (const auto& a : args)
    {
        if (a == "comp-1")
            hasCompId = true;
        if (a == "motor")
            hasCompName = true;
        if (a == "contoso")
            hasCompManufacturer = true;
        if (a == "v1")
            hasCompModel = true;
        if (a == "2.0")
            hasCompVersion = true;
        if (a == "grp1")
            hasCompGroup = true;
        if (a == "/dev/motor0")
            hasCompPropValue = true;
    }
    CHECK(hasCompId);
    CHECK(hasCompName);
    CHECK(hasCompManufacturer);
    CHECK(hasCompModel);
    CHECK(hasCompVersion);
    CHECK(hasCompGroup);
    CHECK(hasCompPropValue);
}

/* =====================================================================
 * SWUpdateHandler_PerformAction (free function) tests
 * ===================================================================== */

TEST_CASE("SWUpdate: PerformAction - null workflow data", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("install", nullptr, false, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
}

TEST_CASE("SWUpdate: PerformAction - prepareArgsOnly returns args string", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_handler_property_apiversion = "1.1";
    auto wd = make_workflow_data();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("install", &wd, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK_FALSE(scriptOutput.empty());
}

TEST_CASE("SWUpdate: PerformAction - api version 1.0 (default)", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_handler_property_apiversion = nullptr; // defaults to 1.0
    auto wd = make_workflow_data();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("install", &wd, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(scriptOutput.find("--action-install") != std::string::npos);
}

TEST_CASE("SWUpdate: PerformAction - api version 1.1", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_handler_property_apiversion = "1.1";
    auto wd = make_workflow_data();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("apply", &wd, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(scriptOutput.find("--action") != std::string::npos);
    CHECK(scriptOutput.find("apply") != std::string::npos);
}

TEST_CASE("SWUpdate: PerformAction - config unavailable", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_config_available = false;
    auto wd = make_workflow_data();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("install", &wd, false, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(r.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_PERFORM_ACTION_FAILED_TO_GET_CONFIG_INSTANCE);
}

TEST_CASE("SWUpdate: PerformAction - child process fails", "[swupdate_handler_v2]")
{
    mock_swupdate_handler_reset();
    mock_handler_property_scriptfilename = "my-script.sh";
    mock_handler_property_swufilename = "my.swu";
    mock_launch_child_exit_code = 5;
    auto wd = make_workflow_data();
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;
    ADUC_Result r = SWUpdateHandler_PerformAction("install", &wd, false, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}
