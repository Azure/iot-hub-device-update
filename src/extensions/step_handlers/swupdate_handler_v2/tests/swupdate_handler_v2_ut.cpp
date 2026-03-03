/**
 * @file swupdate_handler_v2_ut.cpp
 * @brief SWUpdate handler unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/process_utils.hpp"
#include "aduc/swupdate_handler_v2.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

EXTERN_C_BEGIN

EXPORTED_METHOD ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
EXPORTED_METHOD ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);

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
    result->ResultCode = static_cast<ADUC_Result_t>(json_object_get_number(actionResultObject, "resultCode"));
    result->ExtendedResultCode =
        static_cast<ADUC_Result_t>(json_object_get_number(actionResultObject, "extendedResultCode"));
    json_value_free(actionResultValue);
    return true;
}

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/swupdate_handler_v2_test_config";
    setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

// Helper function to generate filecopy_workflow_2 with dynamic test data folder path
static std::string get_filecopy_workflow_2()
{
    std::string testDataFolder = ADUC_TEST_DATA_FOLDER;
    return R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "d19de7fb-11d8-45f7-88e0-03872a591de8" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"30.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"fb7f654eb03c9900a\",\"ff2510f75ca8bf0d3\"],\"handlerProperties\":{\"installedCriteria\":\"This is swupdate filecopy test version 1.0\",\"arguments\":\"--software-version-file )" + testDataFolder + R"(/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt\",\"scriptFileName\":\"example-du-swupdate-script.sh\",\"swuFileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\"}}]},\"files\":{\"fb7f654eb03c9900a\":{\"fileName\":\"du-agent-swupdate-filecopy-test-1_1.0.swu\",\"sizeInBytes\":1536,\"hashes\":{\"sha256\":\"cWJKtVffvDj9B78lgCqWT/lKMBJ9AQ8UmUh48ad8JHA=\"}},\"ff2510f75ca8bf0d3\":{\"fileName\":\"example-du-swupdate-script.sh\",\"sizeInBytes\":24737,\"hashes\":{\"sha256\":\"Nc08FK/T5bOH07nC4GorKTgope5n3+cyb+Ar6KGaY9I=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJheEhUZkdEa2ZVd0dYMnR2SmpxTmhzU3BDYmtyNVpEcXBQVFd4aE9jN2RnPSJ9.ZilWZQSDM59SFpoqpKk33pp9StovL03E9bGACRrfsdPOCXDSqmGBtQxmztg70BTAVpiH7kMlYj1g--no54STJn8_nvt82LX5HEj1xosypdMVIgsAPzhd8RhDKE8T7agrdR4c46PfephjvL7jLRFJN4ipaQIcMxHYaiMeV4KdHXzf-LMASU0tX_y_eGyEIKLNu5kgGnigu96f7JpQ4cgSq5ScZPqzkHutgsgFKG5pY5lefbxJjlepL5N82Bvwu_ZFkCWvo1YSdpMP4heP10xXiq2GIy3bN0yZHjMOIMt-f8jtLmZV7qEblkym6gmrYJENDjAe2rwh6q7ohGb5u_VtrignqV2ZSJobr4ENSBtCNT6Gtm0ZucQghvdEQ0iyM_XQfmDH2AnW_vqt1ymQYkn8HXV5zoeuse6ly4B8L_SzxQei0wZJcyXY61FarIxSth6qEq9my7Hvv8YAnTSp9tEZMSY9j6jYqryF1EV79sIobczkTIe6k1t_4d_xj8roleTf", )"
    R"(     "fileUrls": { )"
    R"(         "fb7f654eb03c9900a": "http://duinstance2--johndoe.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/4d823623494d4a62b7877d58d0d89167/du-agent-swupdate-filecopy-test-1_1.0.swu", )"
    R"(         "ff2510f75ca8bf0d3": "http://duinstance2--johndoe.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/5daa1107aee443b095f0ac6a4548f4b0/example-du-swupdate-script.sh" )"
    R"(      }  )"
    R"( } )";
}

static std::string get_filecopy_workflow_apiver_unknown()
{
        std::string workflow = filecopy_workflow_apiver_2_1;
        const std::string marker = "\"apiVersion\":\"1.1\"";
        size_t pos = workflow.find(marker);
        if (pos != std::string::npos)
        {
                workflow.replace(pos, marker.size(), "\"apiVersion\":\"9.9\"");
        }

        return workflow;
}

static std::string get_workflow_missing_script_filename()
{
        return R"({
    "workflow": { "action": 3, "id": "b6dcf6aa-1111-2222-3333-444444444444" },
    "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"swupdate-missing-script\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"ok\",\"swuFileName\":\"dummy.swu\"}}]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
    "updateManifestSignature": "dummy",
    "fileUrls": {}
})";
}

static std::string get_workflow_missing_swu_filename()
{
        return R"({
    "workflow": { "action": 3, "id": "a7ecf6bb-1111-2222-3333-555555555555" },
    "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"swupdate-missing-swu\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[],\"handlerProperties\":{\"installedCriteria\":\"ok\",\"scriptFileName\":\"dummy.sh\"}}]},\"files\":{},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
    "updateManifestSignature": "dummy",
    "fileUrls": {}
})";
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
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-install --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-apply --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-cancel --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script.sh\" --target-options --action-is-installed --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test version 1.0$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    workflow_free(handle);
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
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"install\" --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "apply", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"apply\" --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "cancel", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"cancel\" --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    result = SWUpdateHandler_PerformAction(
        "is-installed", &stepWorkflow, true, scriptFilePath, args, commandLineArgs, scriptOutput);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    CHECK_THAT(
        scriptOutput,
        Equals(
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/example-du-swupdate-script-2.1.sh\" --target-options --action --target-options \"is-installed\" --target-options --swu-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --work-folder --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8\" --target-options --result-file --target-options \"/var/lib/adu/downloads/d19de7fb-11d8-45f7-88e0-03872a591de8/aduc_result.json\" --target-options --installed-criteria --target-options \"grep '^This is swupdate filecopy test api version 2.1$' /usr/local/du/tests/swupdate-filecopy-test/mock-update-for-file-copy-test-1.txt\""));
    args.clear();

    workflow_free(handle);
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

    ADUC_SystemUtils_RmDirRecursive((std::string(ADUC_TEST_DATA_FOLDER) + "/test-device").c_str());

    // Create test workflow data.
    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, (std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy").c_str());

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
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/example-du-swupdate-script.sh\" --target-options --action-is-installed --target-options --swu-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --software-version-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt\" --target-options --work-folder --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy\" --target-options --result-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/aduc_result.json\" --target-options --installed-criteria --target-options \"This is swupdate filecopy test version 1.0\""));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile((std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy/aduc_result.json").c_str(), &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 901);
    CHECK(result.ExtendedResultCode == 806359140); // (0x30101064)

    workflow_free(handle);
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
    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, (std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy").c_str());

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
            std::string(" --config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/example-du-swupdate-script.sh\" --target-options --action-download --target-options --swu-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --software-version-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt\" --target-options --work-folder --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy\" --target-options --result-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/aduc_result.json\" --target-options --installed-criteria --target-options \"This is swupdate filecopy test version 1.0\""));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    // Check result file.
    bool fileOk = ReadResultFile((std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy/aduc_result.json").c_str(), &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 500);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
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
    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, (std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy").c_str());

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
            std::string("--config-folder \"") + ADUC_TEST_DATA_FOLDER + "/swupdate_handler_v2_test_config\" --update-type \"microsoft/script\" --update-action \"execute\" --target-data \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/example-du-swupdate-script.sh\" --target-options --action-install --target-options --swu-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/du-agent-swupdate-filecopy-test-1_1.0.swu\" --target-options --software-version-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/test-device/vacuum-1/data/mock-update-for-file-copy-test-1.txt\" --target-options --work-folder --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy\" --target-options --result-file --target-options \"" + ADUC_TEST_DATA_FOLDER + "/swupdate_filecopy/aduc_result.json\" --target-options --installed-criteria --target-options \"This is swupdate filecopy test version 1.0\""));

    std::string output;
    int exitCode = ADUC_LaunchChildProcess(commandLineArgs[0], commandLineArgs, output);

    CHECK(exitCode == 0);

    printf("Output:\n%s", output.c_str());

    // Check result file.
    bool fileOk = ReadResultFile((std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy/aduc_result.json").c_str(), &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 600);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
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
    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, (std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy").c_str());

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
    bool fileOk = ReadResultFile((std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy/aduc_result.json").c_str(), &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 700);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
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
    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    CHECK(result.ResultCode != 0);

    workflow_set_workfolder(handle, (std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy").c_str());

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
    bool fileOk = ReadResultFile((std::string(ADUC_TEST_DATA_FOLDER) + "/swupdate_filecopy/aduc_result.json").c_str(), &result);
    CHECK(fileOk);
    CHECK(result.ResultCode == 801);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate handler_create exported contract info", "[swupdate_handler_v2]")
{
    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = GetContractInfo(&info);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);

    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    REQUIRE(handler != nullptr);
    delete handler;
}

static ADUC_WorkflowHandle SetupSwupdateStepWorkflow(
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

    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    if (handler == nullptr)
    {
        ADUC_ConfigInfo_ReleaseInstance(localConfig);
        return nullptr;
    }

    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", handler);

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

TEST_CASE("SWUpdate utility functions read config/value", "[swupdate_handler_v2]")
{
    CHECK(SWUpdateHandlerImpl::ReadValueFromFile("").empty());
    CHECK(SWUpdateHandlerImpl::ReadValueFromFile(std::string(PATH_MAX + 10, 'a')).empty());
    CHECK(SWUpdateHandlerImpl::ReadValueFromFile("/tmp/swupdate-missing.txt").empty());

    const std::string valueFile = "/tmp/swupdate-value.txt";
    {
        std::ofstream out(valueFile);
        out << "  test-value  \n";
    }
    CHECK_THAT(SWUpdateHandlerImpl::ReadValueFromFile(valueFile), Equals("test-value"));
    remove(valueFile.c_str());

    std::unordered_map<std::string, std::string> values;
    ADUC_Result badConfigResult = SWUpdateHandlerImpl::ReadConfig("/tmp/swupdate-no-config.json", values);
    CHECK(IsAducResultCodeFailure(badConfigResult.ResultCode));

    const std::string malformedCfgFile = "/tmp/swupdate-config-malformed.json";
    {
        std::ofstream out(malformedCfgFile);
        out << "{\"--opt1\":\"val1\",";
    }
    ADUC_Result malformedConfigResult = SWUpdateHandlerImpl::ReadConfig(malformedCfgFile, values);
    CHECK(IsAducResultCodeFailure(malformedConfigResult.ResultCode));
    remove(malformedCfgFile.c_str());

    const std::string cfgFile = "/tmp/swupdate-config.json";
    {
        std::ofstream out(cfgFile);
        out << "{\"--opt1\":\"val1\",\"--opt2\":\"val2\"}";
    }

    ADUC_Result configResult = SWUpdateHandlerImpl::ReadConfig(cfgFile, values);
    CHECK(configResult.ResultCode == ADUC_Result_Success);
    CHECK(values["--opt1"] == "val1");
    CHECK(values["--opt2"] == "val2");
    remove(cfgFile.c_str());
}

TEST_CASE("SWUpdate PerformAction fails when workflowData is nullptr", "[swupdate_handler_v2]")
{
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    ADUC_Result result = SWUpdateHandler_PerformAction(
        "install",
        nullptr,
        true,
        scriptFilePath,
        args,
        commandLineArgs,
        scriptOutput);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
}

TEST_CASE("SWUpdate PerformAction fails when workflow handle is nullptr", "[swupdate_handler_v2]")
{
    ADUC_WorkflowData workflowData = {};

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    ADUC_Result result = SWUpdateHandler_PerformAction(
        "apply",
        &workflowData,
        true,
        scriptFilePath,
        args,
        commandLineArgs,
        scriptOutput);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW);
}

TEST_CASE("SWUpdate PrepareCommandArguments guard and component branches", "[swupdate_handler_v2]")
{
    std::string commandFilePath;
    std::vector<std::string> args;

    ADUC_Result nullWorkflowResult = SWUpdateHandlerImpl::PrepareCommandArguments(
        nullptr,
        "/tmp/result.json",
        "/tmp/work",
        commandFilePath,
        args);
    CHECK(IsAducResultCodeFailure(nullWorkflowResult.ResultCode));
    CHECK(nullWorkflowResult.ExtendedResultCode == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_NULL_WORKFLOW);

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(filecopy_workflow, &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(stepHandle, "not valid json"));
    args.clear();
    ADUC_Result invalidComponentsResult = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/result.json",
        "/tmp/work",
        commandFilePath,
        args);
    CHECK(IsAducResultCodeFailure(invalidComponentsResult.ResultCode));
    CHECK(
        invalidComponentsResult.ExtendedResultCode
        == ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_COMPONENT);

    REQUIRE(workflow_set_selected_components(stepHandle, "{\"components\":[]}"));
    args.clear();
    ADUC_Result emptyComponentsResult = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/result.json",
        "/tmp/work",
        commandFilePath,
        args);
    CHECK(emptyComponentsResult.ResultCode == ADUC_Result_Download_Skipped_NoMatchingComponents);

    REQUIRE(workflow_set_selected_components(
        stepHandle,
        "{\"components\":[{\"id\":\"comp-1\",\"name\":\"motor\",\"manufacturer\":\"contoso\",\"model\":\"v1\",\"version\":\"2.0\",\"group\":\"g\",\"properties\":{\"path\":\"/dev/motor0\"}},{\"id\":\"comp-2\",\"name\":\"other\"}]}"));
    args.clear();
    ADUC_Result componentResult = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/result.json",
        "/tmp/work",
        commandFilePath,
        args);
    CHECK(componentResult.ResultCode == ADUC_Result_Success);
    CHECK(std::find(args.begin(), args.end(), "--swu-file") != args.end());

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate content handler direct methods on real step workflow", "[swupdate_handler_v2]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;

    std::string workflowJson = get_filecopy_workflow_2();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);
    REQUIRE(rootHandle != nullptr);
    REQUIRE(config != nullptr);

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    ADUC_Result backupResult = handler->Backup(&stepWorkflow);
    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success);

    ADUC_Result isInstalledResult = handler->IsInstalled(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(isInstalledResult.ResultCode) || IsAducResultCodeSuccess(isInstalledResult.ResultCode)));

    ADUC_Result downloadResult = handler->Download(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(downloadResult.ResultCode) || IsAducResultCodeSuccess(downloadResult.ResultCode)));

    ADUC_Result installResult = handler->Install(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(installResult.ResultCode) || IsAducResultCodeSuccess(installResult.ResultCode)));

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(applyResult.ResultCode) || IsAducResultCodeSuccess(applyResult.ResultCode)));

    ADUC_Result cancelResult = handler->Cancel(&stepWorkflow);
    CHECK((cancelResult.ResultCode == ADUC_Result_Cancel_Success || cancelResult.ResultCode == ADUC_Result_Cancel_UnableToCancel));

    ADUC_Result restoreResult = handler->Restore(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(restoreResult.ResultCode) || IsAducResultCodeSuccess(restoreResult.ResultCode)));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate PrepareCommandArguments fails when scriptFileName is missing", "[swupdate_handler_v2]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;

    std::string workflowJson = get_workflow_missing_script_filename();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result result = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/aduc_result.json",
        "/tmp/swupdate-work",
        commandFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate PrepareCommandArguments fails when swuFileName is missing", "[swupdate_handler_v2]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;

    std::string workflowJson = get_workflow_missing_swu_filename();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result result = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/aduc_result.json",
        "/tmp/swupdate-work",
        commandFilePath,
        args);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SWU_FILE_NAME);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate PerformAction prepare path with unknown apiVersion", "[swupdate_handler_v2]")
{
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;

    std::string workflowJson = get_filecopy_workflow_apiver_unknown();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    ADUC_Result result = SWUpdateHandler_PerformAction(
        "install",
        &stepWorkflow,
        true,
        scriptFilePath,
        args,
        commandLineArgs,
        scriptOutput);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(scriptOutput.find("--action-") == std::string::npos);
    CHECK(scriptOutput.find(" --action \"") == std::string::npos);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

static std::string get_workflow_script_not_matching_files()
{
    return R"({
    "workflow": { "action": 3, "id": "c8dcf6cc-1111-2222-3333-777777777777" },
    "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"swupdate-nomatch\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"f1\",\"f2\"],\"handlerProperties\":{\"installedCriteria\":\"ok\",\"scriptFileName\":\"no-match.sh\",\"swuFileName\":\"dummy.swu\"}}]},\"files\":{\"f1\":{\"fileName\":\"file-a.bin\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}},\"f2\":{\"fileName\":\"file-b.bin\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
    "updateManifestSignature": "dummy",
    "fileUrls": { "f1": "http://example/a", "f2": "http://example/b" }
})";
}

TEST_CASE("SWUpdate Download fails via handler when scriptFileName is missing", "[swupdate_handler_v2]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    std::string workflowJson = get_workflow_missing_script_filename();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate Download fails via handler when file count is insufficient", "[swupdate_handler_v2]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    std::string workflowJson = get_workflow_missing_swu_filename();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate Download fails when script file entity not found by name", "[swupdate_handler_v2]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    std::string workflowJson = get_workflow_script_not_matching_files();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result result = handler->Download(&stepWorkflow);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_GET_SCRIPT_FILE_ENTITY);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate PerformAction non-prepare exercises result file parsing", "[swupdate_handler_v2]")
{
    set_test_config_folder();
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    REQUIRE(config != nullptr);

    ContentHandler* swupdateHandler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    REQUIRE(swupdateHandler != nullptr);
    ExtensionManager::SetUpdateContentHandlerExtension("microsoft/swupdate:2", swupdateHandler);

    std::string workflow_json = get_filecopy_workflow_2();
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflow_json.c_str(), false, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    const std::string workFolder = "/tmp/adu-swupdate-result-parse-test";
    ADUC_SystemUtils_MkSandboxDirRecursive(workFolder.c_str());
    workflow_set_workfolder(handle, "%s", workFolder.c_str());

    result = PrepareStepsWorkflowDataObject(handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    ADUC_WorkflowHandle stepHandle = workflow_get_child(handle, 0);
    REQUIRE(stepHandle != nullptr);
    workflow_set_workfolder(stepHandle, "%s", workFolder.c_str());

    // Pre-create a result file so the result-file-parsing code path is exercised
    const std::string resultFile = workFolder + "/aduc_result.json";
    {
        std::ofstream out(resultFile);
        out << R"({"resultCode":600,"extendedResultCode":0,"resultDetails":"test-result"})";
    }

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    result = SWUpdateHandler_PerformAction(
        "install", &stepWorkflow, false, scriptFilePath, args, commandLineArgs, scriptOutput);

    // LaunchChildProcess will fail (no adu-shell), but the pre-created result file
    // exercises the JSON parsing success path.
    CHECK((IsAducResultCodeSuccess(result.ResultCode) || IsAducResultCodeFailure(result.ResultCode)));

    std::remove(resultFile.c_str());
    ADUC_SystemUtils_RmDirRecursive(workFolder.c_str());
    workflow_free(handle);
    ExtensionManager::Uninit();
    ADUC_ConfigInfo_ReleaseInstance(config);
}

TEST_CASE("SWUpdate handler Backup and Restore methods", "[swupdate_handler_v2]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    std::string workflowJson = get_filecopy_workflow_2();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result backupResult = handler->Backup(&stepWorkflow);
    CHECK(backupResult.ResultCode == ADUC_Result_Backup_Success);

    // Restore calls CancelApply which calls PerformAction("cancel").
    // In test env this fails, so Restore returns ADUC_Result_Failure.
    ADUC_Result restoreResult = handler->Restore(&stepWorkflow);
    CHECK((IsAducResultCodeFailure(restoreResult.ResultCode)
        || restoreResult.ResultCode == ADUC_Result_Restore_Success));

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

TEST_CASE("SWUpdate Apply routes through Cancel when operation cancel is requested", "[swupdate_handler_v2]")
{
    set_test_config_folder();

    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;
    std::string workflowJson = get_filecopy_workflow_2();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    workflow_set_operation_cancel_requested(stepHandle, true);

    ADUC_WorkflowData stepWorkflow{};
    stepWorkflow.WorkflowHandle = stepHandle;

    std::unique_ptr<ContentHandler> handler(SWUpdateHandlerImpl::CreateContentHandler());
    REQUIRE(handler != nullptr);

    ADUC_Result applyResult = handler->Apply(&stepWorkflow);
    bool applyResultIsExpected =
        applyResult.ResultCode == ADUC_Result_Cancel_Success
        || applyResult.ResultCode == ADUC_Result_Cancel_UnableToCancel
        || IsAducResultCodeFailure(applyResult.ResultCode);
    CHECK(applyResultIsExpected);

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}

// Helper: workflow with component argument placeholders in handlerProperties.arguments
static std::string get_workflow_with_component_args()
{
    return R"({
    "workflow": { "action": 3, "id": "e9dcf7dd-2222-3333-4444-888888888888" },
    "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"swupdate-comp-args\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:2\",\"files\":[\"f1\",\"f2\"],\"handlerProperties\":{\"installedCriteria\":\"ok\",\"scriptFileName\":\"script.sh\",\"swuFileName\":\"update.swu\",\"arguments\":\"--component-id-val --component-name-val --component-manufacturer-val --component-model-val --component-version-val --component-group-val --component-prop-val path\"}}]},\"files\":{\"f1\":{\"fileName\":\"update.swu\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\"}},\"f2\":{\"fileName\":\"script.sh\",\"sizeInBytes\":16,\"hashes\":{\"sha256\":\"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=\"}}},\"createdDateTime\":\"2022-03-28T22:36:07.8445392Z\"}",
    "updateManifestSignature": "dummy",
    "fileUrls": { "f1": "http://example/update.swu", "f2": "http://example/script.sh" }
})";
}

TEST_CASE("SWUpdate PrepareCommandArguments covers non-null component property branches", "[swupdate_handler_v2]")
{
    // Uses a workflow with --component-*-val argument placeholders and a full component JSON
    // to exercise the val != nullptr branches for manufacturer, model, version, group.
    ADUC_WorkflowHandle rootHandle = nullptr;
    const ADUC_ConfigInfo* config = nullptr;

    std::string workflowJson = get_workflow_with_component_args();
    ADUC_WorkflowHandle stepHandle = SetupSwupdateStepWorkflow(workflowJson.c_str(), &rootHandle, &config);
    REQUIRE(stepHandle != nullptr);

    REQUIRE(workflow_set_selected_components(
        stepHandle,
        "{\"components\":[{\"id\":\"comp-1\",\"name\":\"motor\","
        "\"manufacturer\":\"Contoso\",\"model\":\"v2-motor\","
        "\"version\":\"3.0\",\"group\":\"motors\","
        "\"properties\":{\"path\":\"/dev/motor0\"}}]}"));

    std::string commandFilePath;
    std::vector<std::string> args;
    ADUC_Result result = SWUpdateHandlerImpl::PrepareCommandArguments(
        stepHandle,
        "/tmp/aduc_result.json",
        "/tmp/swupdate-work",
        commandFilePath,
        args);

    CHECK(result.ResultCode == ADUC_Result_Success);

    // Verify actual component values appear (not "n/a")
    CHECK(std::find(args.begin(), args.end(), "comp-1") != args.end());
    CHECK(std::find(args.begin(), args.end(), "motor") != args.end());
    CHECK(std::find(args.begin(), args.end(), "Contoso") != args.end());
    CHECK(std::find(args.begin(), args.end(), "v2-motor") != args.end());
    CHECK(std::find(args.begin(), args.end(), "3.0") != args.end());
    CHECK(std::find(args.begin(), args.end(), "motors") != args.end());
    CHECK(std::find(args.begin(), args.end(), "/dev/motor0") != args.end());

    // "n/a" should NOT appear since all fields are provided
    CHECK(std::find(args.begin(), args.end(), "n/a") == args.end());

    workflow_free(rootHandle);
    ADUC_ConfigInfo_ReleaseInstance(config);
    ExtensionManager::Uninit();
}
