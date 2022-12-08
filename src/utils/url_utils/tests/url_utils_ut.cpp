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

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - non empty file and intermediate path segments")
{
    STRING_HANDLE lastPathSegment = NULL;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com/path/to/file-v1.1.json", &lastPathSegment)
            .ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));
    STRING_delete(lastPathSegment);
}

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - non empty file and no intermediate path segments")
{
    STRING_HANDLE lastPathSegment = NULL;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com/file-v1.1.json", &lastPathSegment).ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));
    STRING_delete(lastPathSegment);
}

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - empty file")
{
    STRING_HANDLE lastPathSegment = NULL;
    ADUC_Result result = ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com/", &lastPathSegment);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals(""));
    STRING_delete(lastPathSegment);
}

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - empty file, no trailing slash")
{
    STRING_HANDLE lastPathSegment = NULL;
    ADUC_Result result = ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com", &lastPathSegment);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals(""));
    STRING_delete(lastPathSegment);
}

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - non empty file with query string")
{
    STRING_HANDLE lastPathSegment = NULL;
    REQUIRE(IsAducResultCodeSuccess(ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com/path/to/file-v1.1.json?a=1&b=2", &lastPathSegment).ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));
    STRING_delete(lastPathSegment);
}

TEST_CASE("ADUC_UrlUtils_GetLastPathSegmentOfUrl - non empty file with query string and octothorpe jump")
{
    STRING_HANDLE lastPathSegment = NULL;
    REQUIRE(IsAducResultCodeSuccess(
        ADUC_UrlUtils_GetLastPathSegmentOfUrl("http://somehost.com/path/to/file-v1.1.json#jumptosection?a=1&b=2", &lastPathSegment)
            .ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));
    STRING_delete(lastPathSegment);
}
