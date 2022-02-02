/**
 * @file path_utils_ut.cpp
 * @brief Unit Tests for path_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/path_utils.h>
#include <aduc/string_handle_wrapper.hpp>
#include <azure_c_shared_utility/strings.h>
#include <catch2/catch.hpp>
using ADUC::StringUtils::STRING_HANDLE_wrapper;
using Catch::Matchers::Equals;

TEST_CASE("SanitizePathSegment")
{
    SECTION("NULL unsanitized")
    {
        STRING_HANDLE sanitized = SanitizePathSegment(nullptr);
        CHECK(sanitized == nullptr);
    }

    SECTION("Empty unsanitized")
    {
        STRING_HANDLE sanitized = SanitizePathSegment("");
        CHECK(sanitized == nullptr);
    }

    SECTION("all good chars")
    {
        STRING_HANDLE_wrapper sanitized_single{ SanitizePathSegment("g") };
        CHECK_THAT(sanitized_single.c_str(), Equals("g"));

        STRING_HANDLE_wrapper sanitized_multi{ SanitizePathSegment("gg") };
        CHECK_THAT(sanitized_multi.c_str(), Equals("gg"));

        STRING_HANDLE_wrapper sanitized_alphanum{ SanitizePathSegment("abcdefghijklmnopqrstuvwxyz0123456789") };
        CHECK_THAT(sanitized_alphanum.c_str(), Equals("abcdefghijklmnopqrstuvwxyz0123456789"));
    }

    SECTION("replace separator")
    {
        STRING_HANDLE_wrapper sanitized_separator{ SanitizePathSegment("/") };
        CHECK_THAT(sanitized_separator.c_str(), Equals("_"));

        STRING_HANDLE_wrapper sanitized_separator_begin{ SanitizePathSegment("/a") };
        CHECK_THAT(sanitized_separator_begin.c_str(), Equals("_a"));

        STRING_HANDLE_wrapper sanitized_separator_middle{ SanitizePathSegment("a/b") };
        CHECK_THAT(sanitized_separator_middle.c_str(), Equals("a_b"));

        STRING_HANDLE_wrapper sanitized_separator_end{ SanitizePathSegment("a/") };
        CHECK_THAT(sanitized_separator_end.c_str(), Equals("a_"));
    }

    SECTION("omit replace hyphen")
    {
        STRING_HANDLE_wrapper sanitized_hyphen{ SanitizePathSegment("a-b") };
        CHECK_THAT(sanitized_hyphen.c_str(), Equals("a-b"));
    }

    SECTION("replace non-alphanumeric")
    {
        STRING_HANDLE_wrapper sanitized_nonalphanum{ SanitizePathSegment("a@b") };
        CHECK_THAT(sanitized_nonalphanum.c_str(), Equals("a_b"));
    }

    SECTION("replace multiple sequence")
    {
        STRING_HANDLE_wrapper sanitized_multi_seq{ SanitizePathSegment("a@bc_-!0123$") };
        CHECK_THAT(sanitized_multi_seq.c_str(), Equals("a_bc_-_0123_"));
    }
}
