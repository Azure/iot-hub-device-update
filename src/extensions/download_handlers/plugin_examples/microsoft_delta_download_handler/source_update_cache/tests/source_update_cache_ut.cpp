/**
 * @file source_update_cache_ut.cpp
 * @brief Non-mock unit tests for source_update_cache.c.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include <aduc/source_update_cache.h>
#include <aduc/source_update_cache_utils.h>
}

#include <aducpal/unistd.h>
#include <aducpal/sys_stat.h>

#include <azure_c_shared_utility/strings.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
const std::string kTestBaseDir = "/tmp/adutest/source_update_cache_nonmock";
const char* kProvider = "contoso";
const char* kHash = "abc123/+==";
const char* kAlg = "sha256";
} // namespace

TEST_CASE("Lookup returns cache-miss when file does not exist")
{
    std::filesystem::remove_all(kTestBaseDir);
    std::filesystem::create_directories(kTestBaseDir);

    STRING_HANDLE outPath = nullptr;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup(kProvider, kHash, kAlg, kTestBaseDir.c_str(), &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(outPath == nullptr);
}

TEST_CASE("Lookup returns success when cache file exists and is readable")
{
    std::filesystem::remove_all(kTestBaseDir);
    std::filesystem::create_directories(kTestBaseDir);

    STRING_HANDLE expectedPath =
        ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(kProvider, kHash, kAlg, kTestBaseDir.c_str());
    REQUIRE(expectedPath != nullptr);

    std::filesystem::path filePath{ STRING_c_str(expectedPath) };
    std::filesystem::create_directories(filePath.parent_path());

    {
        std::ofstream out(filePath.string());
        REQUIRE(out.good());
        out << "cached update payload";
    }

    STRING_HANDLE outPath = nullptr;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup(kProvider, kHash, kAlg, kTestBaseDir.c_str(), &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outPath != nullptr);
    CHECK(std::string(STRING_c_str(outPath)) == std::string(STRING_c_str(expectedPath)));

    STRING_delete(outPath);
    STRING_delete(expectedPath);
}

TEST_CASE("Lookup returns failure with LOOKUP_CREATE_PATH when provider is null")
{
    STRING_HANDLE outPath = nullptr;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup(nullptr, kHash, kAlg, kTestBaseDir.c_str(), &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_LOOKUP_CREATE_PATH);
    CHECK(outPath == nullptr);
}

TEST_CASE("Lookup returns cache-miss when cached file is unreadable")
{
    std::filesystem::remove_all(kTestBaseDir);
    std::filesystem::create_directories(kTestBaseDir);

    STRING_HANDLE expectedPath =
        ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(kProvider, kHash, kAlg, kTestBaseDir.c_str());
    REQUIRE(expectedPath != nullptr);

    std::filesystem::path filePath{ STRING_c_str(expectedPath) };
    std::filesystem::create_directories(filePath.parent_path());

    {
        std::ofstream out(filePath.string());
        REQUIRE(out.good());
        out << "cached-but-unreadable";
    }

    REQUIRE(chmod(filePath.c_str(), 0) == 0);

    STRING_HANDLE outPath = nullptr;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup(kProvider, kHash, kAlg, kTestBaseDir.c_str(), &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(outPath == nullptr);

    REQUIRE(chmod(filePath.c_str(), S_IRUSR | S_IWUSR) == 0);
    std::filesystem::remove_all(kTestBaseDir);
    STRING_delete(expectedPath);
}

TEST_CASE("Move succeeds when workflow has no payload files")
{
    ADUC_Result result =
        ADUC_SourceUpdateCache_Move(nullptr /* workflowHandle */, "/tmp/adutest/source_update_cache_move_cache");

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK((result.ExtendedResultCode == ADUC_ERC_MOVE_PREPURGE || result.ExtendedResultCode == ADUC_ERC_MOVE_POSTPURGE));
}
