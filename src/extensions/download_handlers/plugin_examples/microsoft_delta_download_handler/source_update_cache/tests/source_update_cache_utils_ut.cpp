/**
 * @file source_update_cache_utils_ut.cpp
 * @brief Unit Tests for source_update_cache_utils
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/source_update_cache_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;
#include <aduc/aduc_inode.h> // ADUC_INODE_SENTINEL_VALUE
#include <aduc/auto_dir.hpp> // aduc::AutoDir
#include <aduc/auto_workflowhandle.hpp> // aduc::AutoWorkflowHandle
#include <aduc/c_utils.h> // EXTERN_C_BEGIN, EXTERN_C_END
#include <aduc/file_utils.hpp> // aduc::findFilesInDir
#include <aduc/free_deleter.hpp> // aduc::FreeDeleter<T>
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/system_utils.h> // SystemUtils_IsFile
#include <aduc/workflow_internal.h> // ADUC_Workflow
#include <aduc/workflow_utils.h> // workflow_*, ADUC_WorkflowHandle
#include <algorithm> // std::find
#include <fstream> // std::ofstream
#include <memory> // std::unique_ptr
#include <parson.h> // json_*
#include <regex> // std::regex, std::regex_replace
#include <string>
#include <sys/stat.h> // struct stat, off_t

#define PROVIDER_NAME "TestProvider"

#define TEST_DIR "/tmp/adutest/source_update_cache_utils_ut"

#define TEST_SANDBOX_BASE_PATH TEST_DIR "/test_sandbox"

#define TEST_CACHE_BASE_PATH TEST_DIR "/test_cache"

#define TEST_CACHE_BASE_PROVIDER_PATH TEST_CACHE_BASE_PATH "/" PROVIDER_NAME

#define UPDATE_PAYLOAD_FILENAME "full_target_update.swu"

#define UPDATE_PAYLOAD_ABS_CACHE_PATH TEST_CACHE_BASE_PATH "/" PROVIDER_NAME "/" UPDATE_PAYLOAD_FILENAME

#define NON_PAYLOAD_ABS_CACHE_PATH             \
    TEST_CACHE_BASE_PATH "/" PROVIDER_NAME "/" \
                         "nonPayloadSourceUpdate.swu"

#define WORKFLOW_PROPERTY_FIELD_WORKFOLDER "_workFolder"
#define TEST_WORKFLOW_ID "6f4d0e93-de4d-b3ef-85f0-ba0cfe1030f3"

using Deleter = aduc::FreeDeleter<char>;
using AutoDir = aduc::AutoDir;

// fwd decls
EXTERN_C_BEGIN
bool workflow_set_string_property(ADUC_WorkflowHandle handle, const char* property, const char* value);
EXTERN_C_END

const std::string updateContentData = "hello\n";

// Setup update manifest in workflow handle
const std::string updateManifest{
    R"( {                                                                          )"
    R"(    "updateId": {                                                           )"
    R"(        "provider": "TestProvider",                                         )"
    R"(        "name": "updateName1",                                              )"
    R"(        "version": "1.0"                                                    )"
    R"(    },                                                                      )"
    R"(    "files": {                                                              )"
    R"(        "fileId1": {                                                        )"
    R"(            "fileName": "full_target_update.swu",                           )"
    R"(            "hashes": {                                                     )"
    R"(                "sha256": "hash1/+=="                                       )"
    R"(            },                                                              )"
    R"(            "sizeInBytes": 6,                                               )"
    R"(            "properties": {                                                 )"
    R"(            }                                                               )"
    R"(        }                                                                   )"
    R"(    }                                                                       )"
    R"( }                                                                          )"
};

const std::string desiredTemplate{
    R"( {                                                                        )"
    R"(     "fileUrls": {                                                        )"
    R"(         "fileId1": "http://hostname:port/path/to/full_target_update.swu" )"
    R"(     },                                                                   )"
    R"(     "updateManifest": "UPDATE_MANIFEST",                                 )"
    R"(     "updateManifestSignature": "SIGNATURE",                              )"
    R"(     "workflow": {                                                        )"
    R"(         "action": 3,                                                     )"
    R"(         "id": "WORKFLOW_ID_GUID"                                         )"
    R"(     }                                                                    )"
    R"( }                                                                        )"
};

TEST_CASE("ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath")
{
    const char* test_provider = "test_Provider/1234";
    const char* test_hash = "0123+abcxyz/ABCXYZ==";
    const char* test_alg = "test_Alg_1024";

    {
        //
        // Act
        //
        STRING_HANDLE testSourceUpdateCachePath = ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
            test_provider, test_hash, test_alg, NULL /* updateCacheBasePath */);
        REQUIRE_FALSE(IsNullOrEmpty(STRING_c_str(testSourceUpdateCachePath)));

        //
        // Assert
        //
        const char* expectedPath = "/var/lib/adu/sdc/test_Provider_1234/test_Alg_1024-0123_2Babcxyz_2FABCXYZ_3D_3D";
        CHECK_THAT(STRING_c_str(testSourceUpdateCachePath), Equals(expectedPath));
        STRING_delete(testSourceUpdateCachePath);
    }

    {
        //
        // Act
        //
        STRING_HANDLE testSourceUpdateCachePath = ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
            test_provider, test_hash, test_alg, TEST_CACHE_BASE_PATH);
        REQUIRE_FALSE(IsNullOrEmpty(STRING_c_str(testSourceUpdateCachePath)));

        //
        // Assert
        //
        const char* expectedPath2 =
            TEST_CACHE_BASE_PATH "/test_Provider_1234/test_Alg_1024-0123_2Babcxyz_2FABCXYZ_3D_3D";
        CHECK_THAT(STRING_c_str(testSourceUpdateCachePath), Equals(expectedPath2));
        STRING_delete(testSourceUpdateCachePath);
    }
}

TEST_CASE("ADUC_SourceUpdateCacheUtils_MoveToUpdateCache")
{
    ADUC_Result result = {};

    //
    // Arrange
    //
    auto autoFreeWorkflowHandle = std::unique_ptr<ADUC_Workflow>{ new ADUC_Workflow };
    REQUIRE(autoFreeWorkflowHandle.get() != nullptr);
    ADUC_WorkflowHandle handle = reinterpret_cast<ADUC_WorkflowHandle>(&autoFreeWorkflowHandle);

    // auto rmdir on scope exit
    AutoDir testBaseDir(TEST_DIR);
    AutoDir testCache(TEST_CACHE_BASE_PATH);
    AutoDir testSandbox(TEST_SANDBOX_BASE_PATH);

    // Remove and recreate test dirs.
    REQUIRE(testBaseDir.RemoveDir());
    // explicitly do not create cache dir as it should get created.
    REQUIRE(testSandbox.CreateDir());

    // Write file to test sandbox dir
    const std::string payloadFilePath = std::string(testSandbox.GetDir().c_str()) + "/" UPDATE_PAYLOAD_FILENAME;

    {
        std::ofstream outStream{ payloadFilePath };
        outStream << updateContentData;
    }

    JSON_Value* updateManifestJson = json_parse_string(updateManifest.c_str());
    REQUIRE(updateManifestJson != nullptr);

    char* serialized = json_serialize_to_string(updateManifestJson);
    REQUIRE(serialized != nullptr);

    std::string serializedUpdateManifest = serialized;
    json_free_serialized_string(serialized);
    serialized = nullptr;

    serializedUpdateManifest = std::regex_replace(serializedUpdateManifest, std::regex("\""), "\\\"");
    std::string desired = std::regex_replace(desiredTemplate, std::regex("UPDATE_MANIFEST"), serializedUpdateManifest);
    desired = std::regex_replace(desired, std::regex("WORKFLOW_ID_GUID"), TEST_WORKFLOW_ID);
    result = workflow_init(desired.c_str(), false, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    // Override work folder to be test sandbox dir
    REQUIRE(workflow_set_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER, testSandbox.GetDir().c_str()));

    //
    // Act
    //
    result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(handle, TEST_CACHE_BASE_PATH);
    CHECK(result.ResultCode == ADUC_Result_Success);

    //
    // Assert
    //
    const char* expectedFileInUpdateCache = TEST_CACHE_BASE_PATH "/TestProvider/sha256-hash1_2F_2B_3D_3D";
    CHECK_FALSE(SystemUtils_IsFile(payloadFilePath.c_str(), nullptr)); // should no longer be in sandbox
    CHECK(SystemUtils_IsFile(expectedFileInUpdateCache, nullptr)); // and should've been moved to update cache

    workflow_uninit(handle);
}

// TODO: re-activate this section after this resolves: Bug 40886231: Arm32 UT ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache failed
// TEST_CASE("ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache")
// {
//     //
//     // Arrange (common)
//     //
//     AutoDir testBaseDir(TEST_DIR);
//     AutoDir testCache(TEST_CACHE_BASE_PROVIDER_PATH);

//     off_t nonPayloadFileSize = 0;

//     const std::string NonPayloadFileContents = "old source update data\n";

//     auto setupCacheFilesFn = [&](bool createUpdatePayloadFile, bool createNonUpdatePayloadFile) {
//         testCache.RemoveDir();
//         testCache.CreateDir();

//         if (createUpdatePayloadFile)
//         {
//             // cache dir file that is part of update payload
//             const std::string PayloadFilePath = UPDATE_PAYLOAD_ABS_CACHE_PATH;
//             std::ofstream outStream{ PayloadFilePath };
//             outStream << updateContentData;
//         }

//         if (createNonUpdatePayloadFile)
//         {
//             // cache dir file that is NOT part of update payload
//             {
//                 std::ofstream outStream{ NON_PAYLOAD_ABS_CACHE_PATH };
//                 outStream << NonPayloadFileContents;
//             }

//             struct stat st
//             {
//             };
//             nonPayloadFileSize = (stat(NON_PAYLOAD_ABS_CACHE_PATH, &st) == 0) ? st.st_size : 0;
//             REQUIRE(nonPayloadFileSize > 0);
//         }
//     };

//     auto setupWorkflowHandleFn = [&](ADUC_WorkflowHandle* handle) {
//         JSON_Value* updateManifestJson = json_parse_string(updateManifest.c_str());
//         REQUIRE(updateManifestJson != nullptr);

//         char* serialized = json_serialize_to_string(updateManifestJson);
//         REQUIRE(serialized != nullptr);

//         std::string serializedUpdateManifest = serialized;
//         json_free_serialized_string(serialized);
//         serialized = nullptr;

//         serializedUpdateManifest = std::regex_replace(serializedUpdateManifest, std::regex("\""), "\\\"");
//         std::string desired =
//             std::regex_replace(desiredTemplate, std::regex("UPDATE_MANIFEST"), serializedUpdateManifest);
//         desired = std::regex_replace(desired, std::regex("WORKFLOW_ID_GUID"), TEST_WORKFLOW_ID);
//         ADUC_Result result = workflow_init(desired.c_str(), false, handle);
//         REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
//     };

//     SECTION("no cache files exist")
//     {
//         //
//         // Arrange
//         //
//         setupCacheFilesFn(false /* createUpdatePayloadFile */, false /* createNonUpdatePayloadFile */);

//         ADUC_WorkflowHandle handle = nullptr;
//         setupWorkflowHandleFn(&handle);
//         REQUIRE(handle != nullptr);
//         aduc::AutoWorkflowHandle autoWorkflowHandle{ handle };
//         ADUC_Workflow* workflow = reinterpret_cast<ADUC_Workflow*>(handle);

//         //
//         // Act
//         //
//         int res = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
//             handle, 1 /* totalSize */, testCache.GetDir().c_str() /* updateCacheBasePath */);
//         REQUIRE(res == 0);

//         //
//         // Assert
//         //
//         std::vector<std::string> filesInDir;
//         aduc::findFilesInDir(testCache.GetDir(), &filesInDir);
//         CHECK(filesInDir.size() == 0);
//     }

//     SECTION("No cache files that are not update payload")
//     {
//         //
//         // Arrange
//         //
//         setupCacheFilesFn(true /* createUpdatePayloadFile */, false /* createNonUpdatePayloadFile */);

//         ADUC_WorkflowHandle handle = nullptr;
//         setupWorkflowHandleFn(&handle);
//         REQUIRE(handle != nullptr);
//         aduc::AutoWorkflowHandle autoWorkflowHandle{ handle };
//         ADUC_Workflow* workflow = reinterpret_cast<ADUC_Workflow*>(handle);

//         // set inode for payload
//         struct stat st
//         {
//         };
//         ino_t inodeUpdatePayloadInCache =
//             (stat(UPDATE_PAYLOAD_ABS_CACHE_PATH, &st) == 0) ? st.st_ino : ADUC_INODE_SENTINEL_VALUE;
//         REQUIRE(inodeUpdatePayloadInCache != ADUC_INODE_SENTINEL_VALUE);
//         REQUIRE(workflow_set_update_file_inode(handle, 0, inodeUpdatePayloadInCache));
//         REQUIRE(workflow_get_update_file_inode(handle, 0) == st.st_ino);

//         //
//         // Act
//         //
//         int res = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
//             handle, 1 /* totalSize */, testCache.GetDir().c_str() /* updateCacheBasePath */);
//         REQUIRE(res == 0);

//         //
//         // Assert
//         //
//         std::vector<std::string> filesInDir;
//         aduc::findFilesInDir(testCache.GetDir(), &filesInDir);
//         REQUIRE(filesInDir.size() == 1);
//         CHECK_THAT(filesInDir[0], Equals(UPDATE_PAYLOAD_ABS_CACHE_PATH));
//     }

//     SECTION("not current update payload deleted when < totalSize")
//     {
//         //
//         // Arrange
//         //
//         setupCacheFilesFn(true /* createUpdatePayloadFile */, true /* createNonUpdatePayloadFile */);

//         ADUC_WorkflowHandle handle = nullptr;
//         setupWorkflowHandleFn(&handle);
//         REQUIRE(handle != nullptr);
//         aduc::AutoWorkflowHandle autoWorkflowHandle{ handle };
//         ADUC_Workflow* workflow = reinterpret_cast<ADUC_Workflow*>(handle);

//         // set inode for payload
//         struct stat st
//         {
//         };
//         ino_t inodeUpdatePayloadInCache =
//             (stat(UPDATE_PAYLOAD_ABS_CACHE_PATH, &st) == 0) ? st.st_ino : ADUC_INODE_SENTINEL_VALUE;
//         REQUIRE(inodeUpdatePayloadInCache != ADUC_INODE_SENTINEL_VALUE);
//         REQUIRE(workflow_set_update_file_inode(handle, 0, inodeUpdatePayloadInCache));
//         REQUIRE(workflow_get_update_file_inode(handle, 0) == st.st_ino);

//         // ensure upfront that there are 2 files in the test cache, the update payload and the non-payload file.
//         {
//             std::vector<std::string> filesInDir;
//             aduc::findFilesInDir(testCache.GetDir(), &filesInDir);

//             CHECK(filesInDir.size() == 2);

//             bool foundUpdatePayloadInCache = filesInDir.end()
//                 != std::find_if(filesInDir.begin(), filesInDir.end(), [](const std::string& filePath) {
//                                                  return filePath == UPDATE_PAYLOAD_ABS_CACHE_PATH;
//                                              });
//             bool foundNonPayloadInCache = filesInDir.end()
//                 != std::find_if(filesInDir.begin(), filesInDir.end(), [](const std::string& filePath) {
//                                               return filePath == NON_PAYLOAD_ABS_CACHE_PATH;
//                                           });

//             auto item0 = filesInDir[0];
//             auto item1 = filesInDir[1];

//             // pre-condition
//             auto cond = item0 == NON_PAYLOAD_ABS_CACHE_PATH && item1 == UPDATE_PAYLOAD_ABS_CACHE_PATH
//                 || item0 == UPDATE_PAYLOAD_ABS_CACHE_PATH && item1 == NON_PAYLOAD_ABS_CACHE_PATH;
//             CHECK(cond);
//         }

//         //
//         // Act
//         //
//         int res = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
//             handle, nonPayloadFileSize + 1 /* totalSize */, testCache.GetDir().c_str() /* updateCacheBasePath */);
//         REQUIRE(res == 0);

//         //
//         // Assert
//         //
//         {
//             std::vector<std::string> filesInDir;
//             aduc::findFilesInDir(testCache.GetDir(), &filesInDir);
//             CHECK(filesInDir.size() == 1);
//             CHECK_THAT(filesInDir[0], Equals(UPDATE_PAYLOAD_ABS_CACHE_PATH));
//         }
//     }
// }
