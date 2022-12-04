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

TEST_CASE("GetLastPathSegmentOfUrl")
{
    STRING_HANDLE lastPathSegment = NULL;
    REQUIRE(IsAducResultCodeSuccess(GetLastPathSegmentOfUrl("http://somehost.com/path/to/file-v1.1.json", &lastPathSegment).ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));

    REQUIRE(IsAducResultCodeSuccess(GetLastPathSegmentOfUrl("http://somehost.com/file-v1.1.json", &lastPathSegment).ResultCode));
    CHECK_THAT(STRING_c_str(lastPathSegment), Equals("file-v1.1.json"));

    ADUC_Result result = GetLastPathSegmentOfUrl("http://somehost.com/", &lastPathSegment);
    REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_URL_CREATE);
}
