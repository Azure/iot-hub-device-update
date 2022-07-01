/**
 * @file swupdate_handler_v2_ut.cpp
 * @brief SWUpdate handler unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/extension_manager.hpp"
#include "aduc/process_utils.hpp"
#include "aduc/swupdate_handler_v2.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

EXTERN_C_BEGIN

ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

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

const char* filecopy_workflow_2 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"This is swupdate filecopy test version 1.0\",\"arguments\":\"--software-version-file /tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt\",\"scriptFileName\":\"example-du-swupdate-script.sh\",\"swuFileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
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
    result->ResultCode = json_object_get_number(actionResultObject, "resultCode");
    result->ExtendedResultCode = json_object_get_number(actionResultObject, "extendedResultCode");
    json_value_free(actionResultValue);
    return true;
}

TEST_CASE("SWUpdate Prepare Arguments Test")
{
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

    int childCount = workflow_get_children_count(handle);
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
        "--action-install", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptFilePath,
        Equals("/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh"));
    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-install --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --workfolder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "--action-apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-apply --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --workfolder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "--action-cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-cancel --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --workfolder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "--action-is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh" --target-options --action-is-installed --target-options --swu-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --workfolder --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8" --target-options --result-file --target-options "/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json" --target-options --installed-criteria --target-options "grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt")"));
    args.clear();

    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate sample script --action-is-installed")
{
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
        "--action-is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-is-installed --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --workfolder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 901);
    CHECK(result.ExtendedResultCode == 806359140); // (0x30101064)

    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate sample script --action-download")
{
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
        "--action-download", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-download --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --workfolder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile("/tmp/adu/testdata/swupdate_filecopy/aduc_result.json", &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 500);
    CHECK(result.ExtendedResultCode == 0);

    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate sample script --action-install", "[!hide][functional_test]")
{
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
        "--action-install", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            R"( --update-type "microsoft/script" --update-action "execute" --target-data "/tmp/adu/testdata/swupdate_filecopy/example-du-swupdate-script.sh" --target-options --action-install --target-options --swu-file --target-options "/tmp/adu/testdata/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu" --target-options --software-version-file --target-options "/tmp/adu/testdata/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt" --target-options --workfolder --target-options "/tmp/adu/testdata/swupdate_filecopy" --target-options --result-file --target-options "/tmp/adu/testdata/swupdate_filecopy/aduc_result.json" --target-options --installed-criteria --target-options "This is swupdate filecopy test version 1.0")"));

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
}

TEST_CASE("SWUpdate sample script --action-apply")
{
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
        "--action-apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
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
}

TEST_CASE("SWUpdate sample script --action-cancel")
{
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
        "--action-cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);
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
}
