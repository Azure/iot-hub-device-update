/**
 * @file extension_manager_download_test_case.cpp
 * @brief Unit Tests for extension manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "extension_manager_download_test_case.hpp"
#include <aduc/auto_workflowhandle.hpp> // aduc::AutoWorkflowHandle
#include <aduc/calloc_wrapper.hpp> // ADUC::StringUtils::cstr_wrapper
#include <aduc/extension_manager.hpp>
#include <aduc/auto_file_entity.hpp>
#include <aduc/result.h> // ADUC_Result, ADUC_Result_t
#include <aduc/system_utils.h> // ADUC_SystemUtils_GetTemporaryPathName
#include <aduc/types/workflow.h>
#include <aduc/workflow_internal.h>
#include <aduc/workflow_utils.h>
#include <aducpal/stdio.h> // remove
#include <aducpal/unistd.h> // UNREFERENCED_PARAMETER
#include <catch2/catch.hpp>
#include <fstream>
#include <memory>
#include <parson.h>
#include <stdexcept>
#include <stdio.h>
#include <string>

struct json_value_deleter
{
    void operator()(JSON_Value* val)
    {
        json_value_free(val);
    }
};

class MockLib
{
};
MockLib mockLib;
ADUC_ExtensionContractInfo mockContractInfo{ 1, 0 };

const int32_t FailureERC = 0xD0070070;
char mockTargetFilename[] = "mock_update_payload.txt";
char mockPayloadContent[] = "hello";

const std::string testWorkfolder = std::string{ ADUC_TEST_DATA_FOLDER } + "/extension_manager";
const std::string pnpMsgPath = testWorkfolder + "/pnpMsg.json";
const std::string updateManifestPath = testWorkfolder + "/testUpdateManifest.json";
const std::string downloaded_file_path = testWorkfolder + "/" + mockTargetFilename;

using unique_json_value = std::unique_ptr<JSON_Value, json_value_deleter>;

static ADUC_Result MockDownloadSuccessProc(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    UNREFERENCED_PARAMETER(entity);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(timeoutInSeconds);
    UNREFERENCED_PARAMETER(downloadProgressCallback);

    std::ofstream fileStream;
    fileStream.open(downloaded_file_path.c_str(), std::ios::out | std::ios::trunc);
    fileStream << mockPayloadContent;

    ADUC_Result result{ 1, 0 };
    return result;
}

static ADUC_Result MockDownloadFailureProc(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    UNREFERENCED_PARAMETER(entity);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(timeoutInSeconds);
    UNREFERENCED_PARAMETER(downloadProgressCallback);

    ADUC_Result result{ 0, FailureERC };
    return result;
}

static DownloadProc mockDownloadSuccessProcResolver(void* lib)
{
    UNREFERENCED_PARAMETER(lib);
    return MockDownloadSuccessProc;
}

static DownloadProc mockDownloadFailureProcResolver(void* lib)
{
    UNREFERENCED_PARAMETER(lib);
    return MockDownloadFailureProc;
}

static void setupWorkflowHandle(const char* msgJson, ADUC_WorkflowHandle* outWorkflowHandle)
{
    ADUC_Result result{ workflow_init(msgJson, false /* validateManifest */, outWorkflowHandle) };
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    // override the sandbox folder to be test dir
    auto wf = reinterpret_cast<ADUC_Workflow*>(*outWorkflowHandle);
    REQUIRE(JSONSuccess == json_object_set_string(wf->PropertiesObject, "_workFolder", testWorkfolder.c_str()));
}

/**
 * @brief Runs the download test case scenario.
 *
 */
void ExtensionManagerDownloadTestCase::RunScenario()
{
    InitCommon();

    switch (download_scenario)
    {
    case DownloadTestScenario::BasicDownloadSuccess:
        mockProcResolver = mockDownloadSuccessProcResolver;
        expected_result.ResultCode = 1;
        expected_result.ExtendedResultCode = 0;
        break;

    case DownloadTestScenario::BasicDownloadFailure:
        mockProcResolver = mockDownloadFailureProcResolver;
        expected_result.ResultCode = 0;
        expected_result.ExtendedResultCode = FailureERC;
        break;

    default:
        throw std::invalid_argument("invalid scenario");
    }

    RunCommon();
}

void ExtensionManagerDownloadTestCase::InitCommon()
{
    ExtensionManager::SetContentDownloaderLibrary(&mockLib);
    ExtensionManager::SetContentDownloaderContractVersion(mockContractInfo);

    unique_json_value msgValue{ json_parse_file(pnpMsgPath.c_str()) };
    unique_json_value updateManifestValue{ json_parse_file(updateManifestPath.c_str()) };

    REQUIRE(msgValue.get() != nullptr);
    REQUIRE(updateManifestValue.get() != nullptr);

    JSON_Object* msgObj = json_value_get_object(msgValue.get());
    REQUIRE(json_object_has_value(msgObj, "updateManifest"));

    ADUC::StringUtils::cstr_wrapper serializedUpdateManifest{ json_serialize_to_string(updateManifestValue.get()) };
    REQUIRE(JSONSuccess == json_object_dotset_string(msgObj, "updateManifest", serializedUpdateManifest.get()));

    ADUC::StringUtils::cstr_wrapper serialized_value{ json_serialize_to_string_pretty(msgValue.get()) };
    setupWorkflowHandle(serialized_value.get(), &workflowHandle);
}

void ExtensionManagerDownloadTestCase::RunCommon()
{
    AutoFileEntity fileEntity;
    REQUIRE(workflow_get_update_file(workflowHandle, 0, &fileEntity));

    ExtensionManager_Download_Options downloadOptions{ 1 /*timeoutInMinutes*/ };
    actual_result = ExtensionManager::Download(
        &fileEntity,
        workflowHandle,
        &downloadOptions,
        nullptr, // downloadProgressCallback
        mockProcResolver);
}

void ExtensionManagerDownloadTestCase::Cleanup()
{
    remove(downloaded_file_path.c_str());
    workflow_free(workflowHandle);
}
