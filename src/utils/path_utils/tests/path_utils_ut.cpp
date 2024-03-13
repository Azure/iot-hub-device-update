/**
 * @file path_utils_ut.cpp
 * @brief Unit Tests for path_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/calloc_wrapper.hpp>
#include <aduc/path_utils.h>
#include <aduc/string_handle_wrapper.hpp>
#include <azure_c_shared_utility/strings.h>
#include <catch2/catch.hpp>
using ADUC::StringUtils::cstr_wrapper;
using ADUC::StringUtils::STRING_HANDLE_wrapper;
using Catch::Matchers::Equals;

TEST_CASE("PathUtils_ConcatenateDirAndFolderPaths")
{
    SECTION("NULL dirPath")
    {
        const char* folderPath = "folder";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(nullptr, folderPath) };
        CHECK(concatenated.get() == nullptr);
    }
    SECTION("Null filename dirPath")
    {
        const char* dirPath = "/tmp";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, nullptr) };
        CHECK(concatenated.get() == nullptr);
    }
    SECTION("Too long dirpath")
    {
        const char* dirPath =
            "/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fence/tmp/this/is/a/very/long/path/that/should/cause/concatenate/to/fail/a/quick/brown/fox/jumps/over/the/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/very/new/fencey/very/new/fence";
        const char* filePath = "somefilepath";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK(concatenated.get() == nullptr);
    }
    SECTION("Too long filename")
    {
        const char* dirPath = "/tmp/";
        const char* filePath =
            "tmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfenc";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK(concatenated.get() == nullptr);
    }
    SECTION("Too long filename")
    {
        const char* dirPath = "/tmp";
        const char* filePath =
            "tmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfencetmpthisisaverylongpaththatshouldcauseconcatenatetofailaquickbrownfoxjumpsovertheveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverynewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfencenewfenceyverynewfenc";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK(concatenated.get() == nullptr);
    }
    SECTION("Successful concatenation trailing slash")
    {
        const char* dirPath = "/tmp/";
        const char* filePath = "tmpfile";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK_THAT(concatenated.get(), Equals("/tmp/tmpfile"));
    }
    SECTION("Successful concatenation no trailing slash")
    {
        const char* dirPath = "/tmp";
        const char* filePath = "tmpfile";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK_THAT(concatenated.get(), Equals("/tmp/tmpfile"));
    }
    SECTION("Successful concatenation root")
    {
        const char* dirPath = "/";
        const char* filePath = "tmpfile";
        cstr_wrapper concatenated{ PathUtils_ConcatenateDirAndFolderPaths(dirPath, filePath) };
        CHECK_THAT(concatenated.get(), Equals("/tmpfile"));
    }
}

TEST_CASE("PathUtils_SanitizePathSegment")
{
    SECTION("NULL unsanitized")
    {
        STRING_HANDLE sanitized = PathUtils_SanitizePathSegment(nullptr);
        CHECK(sanitized == nullptr);
    }

    SECTION("Empty unsanitized")
    {
        STRING_HANDLE sanitized = PathUtils_SanitizePathSegment("");
        CHECK(sanitized == nullptr);
    }

    SECTION("all good chars")
    {
        STRING_HANDLE_wrapper sanitized_single{ PathUtils_SanitizePathSegment("g") };
        CHECK_THAT(sanitized_single.c_str(), Equals("g"));

        STRING_HANDLE_wrapper sanitized_multi{ PathUtils_SanitizePathSegment("gg") };
        CHECK_THAT(sanitized_multi.c_str(), Equals("gg"));

        STRING_HANDLE_wrapper sanitized_alphanum{ PathUtils_SanitizePathSegment(
            "abcdefghijklmnopqrstuvwxyz0123456789") };
        CHECK_THAT(sanitized_alphanum.c_str(), Equals("abcdefghijklmnopqrstuvwxyz0123456789"));
    }

    SECTION("replace separator")
    {
        STRING_HANDLE_wrapper sanitized_separator{ PathUtils_SanitizePathSegment("/") };
        CHECK_THAT(sanitized_separator.c_str(), Equals("_"));

        STRING_HANDLE_wrapper sanitized_separator_begin{ PathUtils_SanitizePathSegment("/a") };
        CHECK_THAT(sanitized_separator_begin.c_str(), Equals("_a"));

        STRING_HANDLE_wrapper sanitized_separator_middle{ PathUtils_SanitizePathSegment("a/b") };
        CHECK_THAT(sanitized_separator_middle.c_str(), Equals("a_b"));

        STRING_HANDLE_wrapper sanitized_separator_end{ PathUtils_SanitizePathSegment("a/") };
        CHECK_THAT(sanitized_separator_end.c_str(), Equals("a_"));
    }

    SECTION("omit replace hyphen")
    {
        STRING_HANDLE_wrapper sanitized_hyphen{ PathUtils_SanitizePathSegment("a-b") };
        CHECK_THAT(sanitized_hyphen.c_str(), Equals("a-b"));
    }

    SECTION("replace non-alphanumeric")
    {
        STRING_HANDLE_wrapper sanitized_nonalphanum{ PathUtils_SanitizePathSegment("a@b") };
        CHECK_THAT(sanitized_nonalphanum.c_str(), Equals("a_b"));
    }

    SECTION("replace multiple sequence")
    {
        STRING_HANDLE_wrapper sanitized_multi_seq{ PathUtils_SanitizePathSegment("a@bc_-!0123$") };
        CHECK_THAT(sanitized_multi_seq.c_str(), Equals("a_bc_-_0123_"));
    }
}
