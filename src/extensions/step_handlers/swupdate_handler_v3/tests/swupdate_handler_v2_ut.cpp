/**
 * @file swupdate_handler_v3_ut.cpp
 * @brief SWUpdate handler v3 unit tests - focuses on U-Boot variable management and boot health
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/process_utils.hpp"
#include "aduc/swupdate_handler_v3.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;
using Catch::Matchers::ContainsSubstring;

#include <sstream>
#include <string>

EXTERN_C_BEGIN

EXPORTED_METHOD ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

EXTERN_C_END

ADUC_Result SWUpdateHandler_PerformAction(
    const std::string& action,
    const tagADUC_WorkflowData* workflowData,
    bool prepareArgsOnly,
    std::string& scriptFilePath,
    std::vector<std::string>& args,
    std::vector<std::string>& commandLineArgs,
    std::string& scriptOutput);

ADUC_Result PrepareStepsWorkflowDataObject(ADUC_WorkflowHandle handle);

// clang-format off
// Test workflow using microsoft/swupdate:3 handler
const char* v3_basic_workflow =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "test-swupdate-v3-workflow-id" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Device\",\"version\":\"2.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:3\",\"files\":[\"swufile001\"],\"handlerProperties\":{\"installedCriteria\":\"2.0\",\"swuFileName\":\"test-update.swu\"}}]},\"files\":{\"swufile001\":{\"fileName\":\"test-update.swu\",\"sizeInBytes\":1024,\"hashes\":{\"sha256\":\"dGVzdGhhc2g=\"}}},\"createdDateTime\":\"2025-01-05T00:00:00.0000000Z\"}", )"
    R"(     "updateManifestSignature": "test_signature", )"
    R"(     "fileUrls": { )"
    R"(         "swufile001": "http://test.example.com/test-update.swu" )"
    R"(      }  )"
    R"( } )";

// clang-format on

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/swupdate_handler_v3_test_config";
    setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

// Mock U-Boot environment variable storage for testing
static std::map<std::string, std::string> mock_uboot_env;

// Test helper: Set mock U-Boot variable
void SetMockUBootVar(const std::string& name, const std::string& value)
{
    mock_uboot_env[name] = value;
}

// Test helper: Get mock U-Boot variable
std::string GetMockUBootVar(const std::string& name)
{
    auto it = mock_uboot_env.find(name);
    return (it != mock_uboot_env.end()) ? it->second : "";
}

// Test helper: Clear mock U-Boot environment
void ClearMockUBootEnv()
{
    mock_uboot_env.clear();
}

TEST_CASE("SWUpdate Prepare Arguments Test")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 2);

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    size_t childCount = workflow_get_children_count(handle);
    CHECK(childCount == 1);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "install", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptFilePath,
        Equals("/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh"));
    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-install --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-apply --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-cancel --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-is-installed --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate Prepare Arguments Test v2.1")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_apiver_2_1, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 2);

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    size_t childCount = workflow_get_children_count(handle);
    CHECK(childCount == 1);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "install", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptFilePath,
        Equals("/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh"));
    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh" --target-options --action --target-options "install" --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh" --target-options --action --target-options "apply" --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh" --target-options --action --target-options "cancel" --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh" --target-options --action --target-options "is-installed" --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --work-folder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate sample script --action-is-installed")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    ADUC_SystemUtils_RmDirRecursive("/tmp/adu/testdata/test-device");

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_2, false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, "/tmp/adu/testdata/swupdate_filecopy");

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-is-installed --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --work-folder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 901);
    CHECK(result.ExtendedResultCode == 806359140); // (0x30101064)

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate sample script --action-download")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_2, false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, "/tmp/adu/testdata/swupdate_filecopy");

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "download", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-download --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --work-folder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 500);
    CHECK(result.ExtendedResultCode == 0);

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate sample script --action-install", "[.hide][functional_test]")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_2, false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, "/tmp/adu/testdata/swupdate_filecopy");

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "install", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"(--config-folder "/tmp/adu/testdata/swupdate_handler_v2_test_config" --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-install --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --work-folder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    printf("Output:\n%s", output.c_str());

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 600);
    CHECK(result.ExtendedResultCode == 0);

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate sample script --action-apply")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_2, false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, "/tmp/adu/testdata/swupdate_filecopy");

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    printf("Output:\n%s", output.c_str());

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 700);
    CHECK(result.ExtendedResultCode == 0);

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate sample script --action-cancel")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    CHECK(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    CHECK(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    // Create test workflow data.
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(filecopy_workflow_2, false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, "/tmp/adu/testdata/swupdate_filecopy");

    result = PrepareStepsWorkflowDataObject(handle);
    CHECK(result.ResultCode != 0);

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    CHECK(stepHandle != nullptr);

    // Dummy workflow to hold a childHandle.
    ADUC_WorkflowData stepWorkflow = {};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    printf("Output:\n%s", output.c_str());

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 801);
    CHECK(result.ExtendedResultCode == 0);

    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}
