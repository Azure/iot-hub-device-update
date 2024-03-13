/**
 * @file url_utils_ut.cpp
 * @brief Unit Tests for url_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/url_utils.h>

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

TEST_CASE("ADUC_UrlUtils_GetPathFileName - non empty file and intermediate path segments")
{
    STRING_HANDLE urlPathFileName = nullptr;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetPathFileName("http://somehost.com/path/to/file-v1.1.json", &urlPathFileName).ResultCode));
    CHECK_THAT(STRING_c_str(urlPathFileName), Equals("file-v1.1.json"));
    STRING_delete(urlPathFileName);
}

TEST_CASE("ADUC_UrlUtils_GetPathFileName - non empty file and no intermediate path segments")
{
    STRING_HANDLE urlPathFileName = nullptr;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetPathFileName("http://somehost.com/file-v1.1.json", &urlPathFileName).ResultCode));
    CHECK_THAT(STRING_c_str(urlPathFileName), Equals("file-v1.1.json"));
    STRING_delete(urlPathFileName);
}

TEST_CASE("ADUC_UrlUtils_GetPathFileName - empty file")
{
    STRING_HANDLE urlPathFileName = nullptr;
    ADUC_Result result = ADUC_UrlUtils_GetPathFileName("http://somehost.com/", &urlPathFileName);
    REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    CHECK(urlPathFileName == nullptr);
    STRING_delete(urlPathFileName);
}

TEST_CASE("ADUC_UrlUtils_GetPathFileName - empty file, no trailing slash")
{
    STRING_HANDLE urlPathFileName = nullptr;
    ADUC_Result result = ADUC_UrlUtils_GetPathFileName("http://somehost.com", &urlPathFileName);
    REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    CHECK(urlPathFileName == nullptr);
    STRING_delete(urlPathFileName);
}

TEST_CASE("ADUC_UrlUtils_GetPathFileName - non empty file with query string")
{
    STRING_HANDLE urlPathFileName = nullptr;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetPathFileName("http://somehost.com/path/to/file-v1.1.json?a=1&b=2", &urlPathFileName)
            .ResultCode));
    CHECK_THAT(STRING_c_str(urlPathFileName), Equals("file-v1.1.json"));
    STRING_delete(urlPathFileName);
}

TEST_CASE("ADUC_UrlUtils_GetPathFileName - non empty file with query string and octothorpe anchor")
{
    STRING_HANDLE urlPathFileName = nullptr;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetPathFileName("http://somehost.com/path/to/file-v1.1.json#anchor", &urlPathFileName)
            .ResultCode));
    CHECK_THAT(STRING_c_str(urlPathFileName), Equals("file-v1.1.json"));
    STRING_delete(urlPathFileName);
}
