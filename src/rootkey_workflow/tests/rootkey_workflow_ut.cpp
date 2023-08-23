/**
 * @file rootkey_workflow_ut.cpp
 * @brief Unit Tests for rootkey_workflow library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h" // RootKeyWorkflow_UpdateRootKeys
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <catch2/catch.hpp>
#include <root_key_util.h> // RootKeyUtility_GetReportingErc
#include <random> // std::mt19937, std::random_device

using Catch::Matchers::Equals;

#define TEST_BASE_PATH "/tmp/rootkeyworkflow_ut"

#define TEST_DOWNLOAD_DIR TEST_BASE_PATH "/test_downloads"

#define TEST_ROOTKEY_STORE_DIR_PATH TEST_BASE_PATH "/test_rootkey_store_path"

#define TEST_ROOTKEY_STORE_PKG_FILE_PATH TEST_ROOTKEY_STORE_DIR_PATH "/test_rootkeypkg.json"

static std::string get_test_rootkeypkg_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkey_workflow/test_rootkeypkg.json";
    return path;
};

static void ParseAndSerializeJsonFile(const char* src, const char* tgt)
{
    auto root_value = json_parse_file(src);
    REQUIRE(root_value != NULL);

    auto json_res = json_serialize_to_file(root_value, tgt);
    REQUIRE(json_res == JSONSuccess);
}

static ADUC_Result MockDownload(const char* rootKeyPkgUrl, const char* targetFilePath)
{
    ParseAndSerializeJsonFile(get_test_rootkeypkg_path(), targetFilePath)
    return { ADUC_GeneralResult_Success, 0 };
}

TEST_CASE("RootKeyWorkflow_UpdateRootKeys")
{
    SECTION("rootkeyutil reporting erc set on failure")
    {
        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(nullptr /* workflowId */, nullptr /* rootKeyPkgUrl */);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_INVALIDARG);

        ADUC_Result_t ercForReporting = RootKeyUtility_GetReportingErc();
        CHECK(ercForReporting == result.ExtendedResultCode);
    }

    SECTION("rootkey success flow - upd store NOT needed")
    {
        const ADUC_RootKeyPkgDownloaderInfo downloaderInfo{
            /* .name = */ "test-rootkeypkg-downloader",
            /* .downloadFn = */ MockDownload,
            /* .downloadBaseDir = */ TEST_DOWNLOAD_DIR,
        };

        // Ensure rootkey pkg store file exists so that update store is not needed
        ParseAndSerializeJsonFile(get_test_rootkeypkg_path(), targetFilePath);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(
            "cf6dbc7c-98fb-4479-bcbf-16c79efd0d79",
            "http://localhost.knoxjonesi.pizza:70/rootkeypkg.json",
            TEST_ROOTKEY_STORE_DIR_PATH, // rootKeyStoreDirPath
            TEST_ROOTKEY_STORE_PKG_FILE_PATH, // rootKeyStorePkgFilePath
            &downloaderInfo);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ResultCode == ADUC_Result_RootKey_Continue);
        CHECK(result.ExtendedResultCode == ADUC_ERC_ROOTKEY_PKG_UNCHANGED);
    }

    SECTION("rootkey success flow - upd store *is* needed")
    {
        const ADUC_RootKeyPkgDownloaderInfo downloaderInfo{
            /* .name = */ "test-rootkeypkg-downloader",
            /* .downloadFn = */ MockDownload,
            /* .downloadBaseDir = */ TEST_DOWNLOAD_DIR,
        };

        const char* path_to_pkg_store_file = TEST_ROOTKEY_STORE_PKG_FILE_PATH;

        // Ensure rootkey pkg store file is removed so that update store *is* needed
        if(remove(path_to_pkg_store_file) != 0)
        {
            std::mt19937 rg(std::random_device{});
            char chars[] = "1234wxyz";
            std::shuffle(std::begin(chars), std::end(chars), rg);

            path_to_pkg_store_file += chars;
            path_to_pkg_store_file += ".json";
        }

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(
            "cf6dbc7c-98fb-4479-bcbf-16c79efd0d79",
            "http://localhost.knoxjonesi.pizza:70/rootkeypkg.json",
            TEST_ROOTKEY_STORE_DIR_PATH, // rootKeyStoreDirPath
            TEST_ROOTKEY_STORE_PKG_FILE_PATH, // rootKeyStorePkgFilePath
            &downloaderInfo);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ResultCode == ADUC_GeneralResult_Success);
        CHECK(result.ExtendedResultCode == 0);
    }
}

