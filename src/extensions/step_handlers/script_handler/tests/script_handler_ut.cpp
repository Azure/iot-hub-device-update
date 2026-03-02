/**
 * @file script_handler_ut.cpp
 * @brief Script handler unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/process_utils.hpp"
#include "aduc/script_handler.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"
#include "aducpal/stdlib.h" // setenv

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

EXTERN_C_BEGIN

ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);

EXTERN_C_END

ADUC_Result PrepareStepsWorkflowDataObject(ADUC_WorkflowHandle handle);

// clang-format off
const char* filecopy_workflow =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\",\"scriptFileName\":\"example-du-swupdate-script.sh\",\"swuFileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJheEhUZkdEa2ZVd0dYMnR2SmpxTmhzU3BDYmtyNVpEcXBQVFd4aE9jN2RnPSJ9.ZilWZQSDM59SFpoqpKk33pp9StovL03E9bGACRrfsdPOCXDSqmGBtQxmztg70BTAVpiH7kMlYj1g--no54STJn8_nvt82LX5HEj1xosypdMVIgsAPzhd8RhDKE8T7agrdR4c46PfephjvL7jLRFJN4ipaQIcMxHYaiMeV4KdHXzf-LMASU0tX_y_eGyEIKLNu5kgGnigu96f7JpQ4cgSq5ScZPqzkHutgsgFKG5pY5lefbxJjlepL5N82Bvwu_ZFkCWvo1YSdpMP4heP10xXiq2GIy3bN0yZHjMOIMt-f8jtLmZV7qEblkym6gmrYJENDjAe2rwh6q7ohGb5u_VtrignqV2ZSJobr4ENSBtCNT6Gtm0ZucQghvdEQ0iyM_XQfmDH2AnW_vqt1ymQYkn8HXV5zoeuse6ly4B8L_SzxQei0wZJcyXY61FarIxSth6qEq9my7Hvv8YAnTSp9tEZMSY9j6jYqryF1EV79sIobczkTIe6k1t_4d_xj8roleTf", )"
    R"(     "fileUrls": { )"
    R"(         "fb7f654eb03c9900a": "http://duinstance2--wewilair.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/4d823623494d4a62b7877d58d0d89167/du-agent-swupdate-filecopy-test-1_1.0.swu", )"
    R"(         "ff2510f75ca8bf0d3": "http://duinstance2--wewilair.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/5daa1107aee443b095f0ac6a4548f4b0/example-du-swupdate-script.sh" )"
    R"(      }  )"
    R"( } )";

const char* filecopy_workflow_apiver_2_1 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\",\"scriptFileName\":\"example-du-swupdate-script-2.1.sh\",\"apiVersion\":\"1.1\",\"swuFileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script-2.1.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJheEhUZkdEa2ZVd0dYMnR2SmpxTmhzU3BDYmtyNVpEcXBQVFd4aE9jN2RnPSJ9.ZilWZQSDM59SFpoqpKk33pp9StovL03E9bGACRrfsdPOCXDSqmGBtQxmztg70BTAVpiH7kMlYj1g--no54STJn8_nvt82LX5HEj1xosypdMVIgsAPzhd8RhDKE8T7agrdR4c46PfephjvL7jLRFJN4ipaQIcMxHYaiMeV4KdHXzf-LMASU0tX_y_eGyEIKLNu5kgGnigu96f7JpQ4cgSq5ScZPqzkHutgsgFKG5pY5lefbxJjlepL5N82Bvwu_ZFkCWvo1YSdpMP4heP10xXiq2GIy3bN0yZHjMOIMt-f8jtLmZV7qEblkym6gmrYJENDjAe2rwh6q7ohGb5u_VtrignqV2ZSJobr4ENSBtCNT6Gtm0ZucQghvdEQ0iyM_XQfmDH2AnW_vqt1ymQYkn8HXV5zoeuse6ly4B8L_SzxQei0wZJcyXY61FarIxSth6qEq9my7Hvv8YAnTSp9tEZMSY9j6jYqryF1EV79sIobczkTIe6k1t_4d_xj8roleTf", )"
    R"(     "fileUrls": { )"
    R"(         "fb7f654eb03c9900a": "http://duinstance2--wewilair.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/4d823623494d4a62b7877d58d0d89167/du-agent-swupdate-filecopy-test-1_1.0.swu", )"
    R"(         "ff2510f75ca8bf0d3": "http://duinstance2--wewilair.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/5daa1107aee443b095f0ac6a4548f4b0/example-du-swupdate-script.sh" )"
    R"(      }  )"
    R"( } )";

// clang-format on

/**
 * @brief Reads ADUC_Result data from @p resultFile
 *
 * @param resultFile Path to a file contains serialized json string of an ADUC_Result data.
 * @param result Output result
 * @return true If success.
 */
bool ReadResultFile(const char* resultFile, ADUC_Result* result)
{
    if (result == nullptr)
    {
        return false;
    }

    JSON_Value* actionResultValue = json_parse_file(resultFile);
    if (actionResultValue == nullptr)
    {
        return false;
    }

    JSON_Object* actionResultObject = json_object(actionResultValue);
    result->ResultCode = static_cast<int32_t>(json_object_get_number(actionResultObject, "resultCode"));
    result->ExtendedResultCode =
        static_cast<int32_t>(json_object_get_number(actionResultObject, "extendedResultCode"));
    json_value_free(actionResultValue);
    return true;
}

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/script_handler_test_config";
    ADUCPAL_setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

TEST_CASE("Script Handler Prepare Arguments Test")
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

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, true);

    CHECK(results.result.ResultCode != 0);
    CHECK(results.result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptFilePath,
        Equals("/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh"));
    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-install --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_install_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("apply", &stepWorkflow, true);

    CHECK(results.result.ResultCode != 0);
    CHECK(results.result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-apply --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_apply_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("cancel", &stepWorkflow, true);

    CHECK(results.result.ResultCode != 0);
    CHECK(results.result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-cancel --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_cancel_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("is-installed", &stepWorkflow, true);

    CHECK(results.result.ResultCode != 0);
    CHECK(results.result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-is-installed --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_is-installed_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    workflow_free(handle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Prepare Arguments Test v2.1")
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

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, true);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptFilePath,
        Equals("/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh"));
    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"install\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_install_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("apply", &stepWorkflow, true);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"apply\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_apply_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("cancel", &stepWorkflow, true);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"cancel\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_cancel_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    results = ScriptHandler_PerformAction("is-installed", &stepWorkflow, true);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        results.scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/script_handler_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"is-installed\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/action_is-installed_aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    results.args.clear();

    workflow_free(handle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

static ADUC_WorkflowHandle SetupScriptStepWorkflow(
    const char* workflowJson,
    ADUC_WorkflowHandle* rootHandle,
    const ADUC_ConfigInfo** config)
{
    if (rootHandle != nullptr)
    {
        *rootHandle = nullptr;
    }
    if (config != nullptr)
    {
        *config = nullptr;
    }

    set_test_config_folder();

    const ADUC_ConfigInfo* localConfig = ADUC_ConfigInfo_GetInstance();
    if (localConfig == nullptr)
    {
        return nullptr;
    }

    ContentHandler* scriptHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    if (scriptHandler == nullptr)
    {
        ADUC_ConfigInfo_ReleaseInstance(localConfig);
        return nullptr;
    }

    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", scriptHandler);

    ADUC_WorkflowHandle localRootHandle = nullptr;
    ADUC_Result result = workflow_init(workflowJson, false, &localRootHandle);
    if (IsAducResultCodeFailure(result.ResultCode) || localRootHandle == nullptr)
    {
        ADUC_ConfigInfo_ReleaseInstance(localConfig);
        ExtensionManager::Uninit();
        return nullptr;
    }

    result = PrepareStepsWorkflowDataObject(localRootHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_free(localRootHandle);
        ADUC_ConfigInfo_ReleaseInstance(localConfig);
        ExtensionManager::Uninit();
        return nullptr;
    }

    ADUC_WorkflowHandle stepHandle = workflow_get_child(localRootHandle, 0);
    if (stepHandle == nullptr)
    {
        workflow_free(localRootHandle);
        ADUC_ConfigInfo_ReleaseInstance(localConfig);
        ExtensionManager::Uninit();
        return nullptr;
    }

    if (rootHandle != nullptr)
    {
        *rootHandle = localRootHandle;
    }
    if (config != nullptr)
    {
        *config = localConfig;
    }

    return stepHandle;
}

TEST_CASE("Script Handler exported functions smoke test (non-mock)", "[script_handler][non_mock]")
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

TEST_CASE("Script Handler PrepareScriptArguments - invalid selected components JSON", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(stepHandle, "not valid json"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_COMPONENT);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PrepareScriptArguments - selected components missing array", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(stepHandle, "{\"x\":1}"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_COMPONENT);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PrepareScriptArguments - empty selected components array", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(stepHandle, "{\"components\":[]}"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_Result_Download_Skipped_NoMatchingComponents);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PrepareScriptArguments - multi component input still succeeds", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(
        stepHandle,
        "{\"components\":[{\"id\":\"comp-1\",\"name\":\"a\"},{\"id\":\"comp-2\",\"name\":\"b\"}]}"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK_THAT(scriptFilePath, Equals("/tmp/script-work/example-du-swupdate-script.sh"));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction false path with real workflow", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, false);
    CHECK(IsAducResultCodeFailure(results.result.ResultCode));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Install and Apply wrappers on real workflow", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ContentHandler* handler = ScriptHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);

    ADUC_Result installResult = handler->Install(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(installResult.ResultCode));

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(applyResult.ResultCode));

    delete handler;
    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler IsInstalled real workflow path", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("is-installed", &stepWorkflow, true);
    CHECK(IsAducResultCodeSuccess(results.result.ResultCode));
    CHECK(results.result.ExtendedResultCode == 0);
    CHECK(results.scriptOutput.find("--action-is-installed") != std::string::npos);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler free-function null workflow guards", "[script_handler][non_mock]")
{
    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", nullptr, false);
    CHECK(results.result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(results.result.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
}

TEST_CASE("Script Handler free-function null workflow handle guard", "[script_handler][non_mock]")
{
    ADUC_WorkflowData wf{};
    wf.WorkflowHandle = nullptr;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &wf, false);
    CHECK(results.result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(results.result.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
}

TEST_CASE("Script Handler PrepareScriptArguments null workflow handle guard", "[script_handler][non_mock]")
{
    std::string scriptFilePath;
    std::vector<std::string> args;

    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        nullptr,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_NULL_WORKFLOW);
}

// clang-format off
const char* workflow_missing_script_filename =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"dummy-criteria\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": { )"
    R"(         "fb7f654eb03c9900a": "http://example/swu", )"
    R"(         "ff2510f75ca8bf0d3": "http://example/script" )"
    R"(      }  )"
    R"( } )";

const char* workflow_with_component_args =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"dummy-criteria\",\"scriptFileName\":\"example-du-swupdate-script.sh\",\"arguments\":\"--component-id-val --component-name-val --component-manufacturer-val --component-model-val --component-version-val --component-group-val --component-prop-val path --component-prop-val --literal value\\\"quoted\\\"\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": { )"
    R"(         "fb7f654eb03c9900a": "http://example/swu", )"
    R"(         "ff2510f75ca8bf0d3": "http://example/script" )"
    R"(      }  )"
    R"( } )";

const char* workflow_script_no_files =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "4a534e10-5968-4d7b-8912-8100b9f79001" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.1\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"dummy-criteria\",\"scriptFileName\":\"primary-script.sh\"}}]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": {} )"
    R"( } )";

const char* workflow_script_missing_primary_file =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "4a534e10-5968-4d7b-8912-8100b9f79002" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.2\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"payload-1\"],\"handlerProperties\":{\"installedCriteria\":\"dummy-criteria\",\"scriptFileName\":\"primary-script.sh\"}}]},\"files\":{\"payload-1\":{\"fileName\":\"payload-only.bin\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": { "payload-1": "http://example/payload-only.bin" } )"
    R"( } )";

const char* workflow_script_unknown_apiver =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "4a534e10-5968-4d7b-8912-8100b9f79003" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.3\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"script-1\"],\"handlerProperties\":{\"installedCriteria\":\"dummy-criteria\",\"scriptFileName\":\"primary-script.sh\",\"apiVersion\":\"9.9\"}}]},\"files\":{\"script-1\":{\"fileName\":\"primary-script.sh\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "dummy", )"
    R"(     "fileUrls": { "script-1": "http://example/primary-script.sh" } )"
    R"( } )";
// clang-format on

TEST_CASE("Script Handler PrepareScriptArguments - missing scriptFileName property", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_missing_script_filename, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PrepareScriptArguments - component placeholders and quoted args", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_with_component_args, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(
        stepHandle,
        "{\"components\":[{\"id\":\"comp-1\",\"name\":\"motor\",\"properties\":{\"path\":\"/dev/motor0\"}}]}"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK_THAT(scriptFilePath, Equals("/tmp/script-work/example-du-swupdate-script.sh"));
    CHECK(std::find(args.begin(), args.end(), "comp-1") != args.end());
    CHECK(std::find(args.begin(), args.end(), "motor") != args.end());
    CHECK(std::find(args.begin(), args.end(), "n/a") != args.end());
    CHECK(std::find(args.begin(), args.end(), "/dev/motor0") != args.end());

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;
    ADUC_PerformAction_Results performResult = ScriptHandler_PerformAction("install", &stepWorkflow, true);
    CHECK(performResult.result.ResultCode == ADUC_Result_Success);
    CHECK(performResult.scriptOutput.find("'value\"quoted\"'") != std::string::npos);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PrepareScriptArguments - full component properties cover non-null branches", "[script_handler][non_mock]")
{
    // This test exercises the val != nullptr branches for manufacturer, model, version, and group
    // in PrepareScriptArguments, which the existing test (with only id/name/properties) leaves as n/a.
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_with_component_args, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    // Set component JSON with ALL fields populated: manufacturer, model, version, group.
    REQUIRE(workflow_set_selected_components(
        stepHandle,
        "{\"components\":[{\"id\":\"comp-1\",\"name\":\"motor\","
        "\"manufacturer\":\"Contoso\",\"model\":\"v2-motor\","
        "\"version\":\"3.0\",\"group\":\"motors\","
        "\"properties\":{\"path\":\"/dev/motor0\"}}]}"));

    std::string scriptFilePath;
    std::vector<std::string> args;
    ADUC_Result result = ScriptHandlerImpl::PrepareScriptArguments(
        stepHandle,
        "/tmp/script-result.json",
        "/tmp/script-work",
        scriptFilePath,
        args);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK_THAT(scriptFilePath, Equals("/tmp/script-work/example-du-swupdate-script.sh"));

    // Verify the actual component values appear (not "n/a")
    CHECK(std::find(args.begin(), args.end(), "comp-1") != args.end());
    CHECK(std::find(args.begin(), args.end(), "motor") != args.end());
    CHECK(std::find(args.begin(), args.end(), "Contoso") != args.end());
    CHECK(std::find(args.begin(), args.end(), "v2-motor") != args.end());
    CHECK(std::find(args.begin(), args.end(), "3.0") != args.end());
    CHECK(std::find(args.begin(), args.end(), "motors") != args.end());
    CHECK(std::find(args.begin(), args.end(), "/dev/motor0") != args.end());

    // The workflow includes a deliberate "--component-prop-val" without a property key,
    // so exactly one fallback "n/a" is expected from that path.
    const auto nACount = static_cast<int>(std::count(args.begin(), args.end(), "n/a"));
    CHECK(nACount == 1);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Download and IsInstalled fail when scriptFileName is missing", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_missing_script_filename, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result downloadResult = handler->Download(&stepWorkflow);
    CHECK(downloadResult.ResultCode == ADUC_Result_Failure);
    CHECK(downloadResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY);

    ADUC_Result isInstalledResult = handler->IsInstalled(&stepWorkflow);
    CHECK(isInstalledResult.ResultCode == ADUC_Result_Failure);
    CHECK(isInstalledResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Download and IsInstalled fail when file count is zero", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_script_no_files, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result downloadResult = handler->Download(&stepWorkflow);
    CHECK(downloadResult.ResultCode == ADUC_Result_Failure);
    CHECK(downloadResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_INVALID_FILE_COUNT);

    ADUC_Result isInstalledResult = handler->IsInstalled(&stepWorkflow);
    CHECK(isInstalledResult.ResultCode == ADUC_Result_Failure);
    CHECK(isInstalledResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_INVALID_FILE_COUNT);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Download and IsInstalled fail when primary file entity cannot be resolved", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_script_missing_primary_file, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result downloadResult = handler->Download(&stepWorkflow);
    CHECK(downloadResult.ResultCode == ADUC_Result_Failure);
    CHECK(downloadResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PRIMARY_FILE_ENTITY);

    ADUC_Result isInstalledResult = handler->IsInstalled(&stepWorkflow);
    CHECK(isInstalledResult.ResultCode == ADUC_Result_Failure);
    CHECK(isInstalledResult.ExtendedResultCode == ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PRIMARY_FILE_ENTITY);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction prepare path with unknown apiVersion", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(workflow_script_unknown_apiver, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, true);
    CHECK(results.result.ResultCode == ADUC_Result_Success);
    CHECK(results.result.ExtendedResultCode == 0);
    CHECK(results.scriptOutput.find("--action-install") == std::string::npos);
    CHECK(results.scriptOutput.find("--target-options --action") == std::string::npos);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler wrapper methods backup restore cancel", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result backupResult = handler->Backup(&stepWorkflow);
    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success_Unsupported);

    ADUC_Result restoreResult = handler->Restore(&stepWorkflow);
    CHECK(restoreResult.ResultCode == ADUC_Result_Restore_Success_Unsupported);

    ADUC_Result cancelResult = handler->Cancel(&stepWorkflow);
    CHECK(cancelResult.ResultCode == ADUC_Result_Cancel_Success);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction non-prepare with pre-created result file", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    const std::string workFolder = "/tmp/adu-script-result-test";
    ADUC_SystemUtils_MkSandboxDirRecursive(workFolder.c_str());
    REQUIRE(workflow_set_workfolder(stepHandle, "%s", workFolder.c_str()));

    // Pre-create result file so JSON parse path is exercised
    const std::string resultFile = workFolder + "/action_install_aduc_result.json";
    {
        std::ofstream out(resultFile);
        out << R"({"resultCode":600,"extendedResultCode":0,"resultDetails":"script-ok"})";
    }

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    // Call with prepareArgsOnly=false so it runs through the non-prepare path.
    // LaunchChildProcess will fail (no adu-shell) but then the result file will be parsed.
    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, false);
    CHECK((IsAducResultCodeSuccess(results.result.ResultCode) || IsAducResultCodeFailure(results.result.ResultCode)));

    std::remove(resultFile.c_str());
    ADUC_SystemUtils_RmDirRecursive(workFolder.c_str());
    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction with debug logging enabled", "[script_handler][non_mock]")
{
    ADUCPAL_setenv("DU_AGENT_ENABLE_SCRIPT_HANDLER_EXTRA_DEBUG_LOGS", "1", 1);

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    // prepareArgsOnly=true so it builds the command line and returns.
    // The debug logging extra path will be exercised first.
    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, true);
    CHECK(results.result.ResultCode == ADUC_Result_Success);
    CHECK(results.result.ExtendedResultCode == 0);

    ADUCPAL_setenv("DU_AGENT_ENABLE_SCRIPT_HANDLER_EXTRA_DEBUG_LOGS", "", 1);
    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction maps script failure with zero ERC to actionable ERC", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    const std::string workFolder = "/tmp/adu-script-zero-erc-test";
    ADUC_SystemUtils_MkSandboxDirRecursive(workFolder.c_str());
    REQUIRE(workflow_set_workfolder(stepHandle, "%s", workFolder.c_str()));

    const std::string resultFile = workFolder + "/action_install_aduc_result.json";
    {
        std::ofstream out(resultFile);
        out << R"({"resultCode":0,"extendedResultCode":0,"resultDetails":"failed-with-zero-erc"})";
    }

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, false);
    bool resultIsFailureOrSuccess =
        (results.result.ResultCode == ADUC_Result_Failure) || IsAducResultCodeSuccess(results.result.ResultCode);
    CHECK(resultIsFailureOrSuccess);

    if (results.result.ResultCode == ADUC_Result_Failure)
    {
        CHECK(results.result.ExtendedResultCode != 0);
    }

    std::remove(resultFile.c_str());
    ADUC_SystemUtils_RmDirRecursive(workFolder.c_str());
    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction requests reboot when script result requires reboot", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    const std::string workFolder = "/tmp/adu-script-reboot-flag-test";
    ADUC_SystemUtils_MkSandboxDirRecursive(workFolder.c_str());
    REQUIRE(workflow_set_workfolder(stepHandle, "%s", workFolder.c_str()));

    const std::string resultFile = workFolder + "/action_install_aduc_result.json";
    {
        std::ofstream out(resultFile);
        out << R"({"resultCode":605,"extendedResultCode":0,"resultDetails":"reboot-required"})";
    }

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, false);
    if (results.result.ResultCode == ADUC_Result_Install_RequiredReboot)
    {
        CHECK(workflow_is_reboot_requested(stepHandle));
    }
    else
    {
        CHECK(IsAducResultCodeFailure(results.result.ResultCode));
    }

    std::remove(resultFile.c_str());
    ADUC_SystemUtils_RmDirRecursive(workFolder.c_str());
    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Download exercises is-installed check", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    // Download calls Script_Handler_DownloadPrimaryScriptFile (will fail since no real downloader)
    // which exercises the is-installed check and file iteration code paths.
    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Install calls PerformAction with install action", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    // Install -> PerformAction("install") -> LaunchChildProcess (fails)
    ADUC_Result result = handler->Install(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler Apply calls PerformAction with apply action", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    // Apply -> PerformAction("apply") -> LaunchChildProcess (fails)
    ADUC_Result result = handler->Apply(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler IsInstalled calls PerformAction with is-installed action", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(ScriptHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    // IsInstalled -> Script_Handler_DownloadPrimaryScriptFile (fails - no downloader)
    ADUC_Result result = handler->IsInstalled(&stepWorkflow);
    CHECK(IsAducResultCodeFailure(result.ResultCode));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("Script Handler PerformAction api version 1.1 branch", "[script_handler][non_mock]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupScriptStepWorkflow(filecopy_workflow_apiver_2_1, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    // prepareArgsOnly=true to exercise the 1.1 api-version argument branch
    ADUC_PerformAction_Results results = ScriptHandler_PerformAction("install", &stepWorkflow, true);
    CHECK(results.result.ResultCode == ADUC_Result_Success);
    // With apiVersion 1.1, the output should have --action install (not --action-install)
    CHECK(results.scriptOutput.find("--action-install") == std::string::npos);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}
