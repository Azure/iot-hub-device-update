/**
 * @file string_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
#include <vector>
using Catch::Matchers::Equals;

#include "aduc/string_utils.hpp"

using ADUC::StringUtils::Split;
using ADUC::StringUtils::Trim;
using ADUC::StringUtils::TrimLeading;
using ADUC::StringUtils::TrimTrailing;

TEST_CASE("TrimLeading")
{
    SECTION("Empty String")
    {
        std::string empty{};
        TrimLeading(empty);
        CHECK_THAT(empty, Equals(""));
    }

    SECTION("All spaces")
    {
        std::string spaces{ "   \t  " };
        TrimLeading(spaces);
        CHECK_THAT(spaces, Equals(""));
    }

    SECTION("No spaces")
    {
        std::string noSpace{ "abc" };
        TrimLeading(noSpace);
        CHECK_THAT(noSpace, Equals(noSpace));
    }

    SECTION("Leading spaces")
    {
        std::string space1{ " abc" };
        TrimLeading(space1);
        CHECK_THAT(space1, Equals("abc"));

        std::string space3{ " \t abc" };
        TrimLeading(space3);
        CHECK_THAT(space3, Equals("abc"));
    }

    SECTION("Trailing spaces")
    {
        std::string space1{ "abc " };
        TrimLeading(space1);
        CHECK_THAT(space1, Equals("abc "));

        std::string space3{ "abc \t " };
        TrimLeading(space3);
        CHECK_THAT(space3, Equals("abc \t "));
    }

    SECTION("Leading and trailing spaces")
    {
        std::string space1{ " abc " };
        TrimLeading(space1);
        CHECK_THAT(space1, Equals("abc "));

        std::string space3{ " \t abc \t " };
        TrimLeading(space3);
        CHECK_THAT(space3, Equals("abc \t "));
    }
}

TEST_CASE("TrimTrailing")
{
    SECTION("Empty String")
    {
        std::string empty{};
        TrimTrailing(empty);
        CHECK_THAT(empty, Equals(""));
    }

    SECTION("All spaces")
    {
        std::string spaces{ "   \t  " };
        TrimTrailing(spaces);
        CHECK_THAT(spaces, Equals(""));
    }

    SECTION("No spaces")
    {
        std::string noSpace{ "abc" };
        TrimTrailing(noSpace);
        CHECK_THAT(noSpace, Equals(noSpace));
    }

    SECTION("Leading spaces")
    {
        std::string space1{ " abc" };
        TrimTrailing(space1);
        CHECK_THAT(space1, Equals(" abc"));

        std::string space3{ " \t abc" };
        TrimTrailing(space3);
        CHECK_THAT(space3, Equals(" \t abc"));
    }

    SECTION("Trailing spaces")
    {
        std::string space1{ "abc " };
        TrimTrailing(space1);
        CHECK_THAT(space1, Equals("abc"));

        std::string space3{ "abc \t " };
        TrimTrailing(space3);
        CHECK_THAT(space3, Equals("abc"));
    }

    SECTION("Leading and trailing spaces")
    {
        std::string space1{ " abc " };
        TrimTrailing(space1);
        CHECK_THAT(space1, Equals(" abc"));

        std::string space3{ " \t abc \t " };
        TrimTrailing(space3);
        CHECK_THAT(space3, Equals(" \t abc"));
    }
}

TEST_CASE("Trim")
{
    SECTION("Empty String")
    {
        std::string empty{};
        Trim(empty);
        CHECK_THAT(empty, Equals(""));
    }

    SECTION("All spaces")
    {
        std::string spaces{ "   \t  " };
        Trim(spaces);
        CHECK_THAT(spaces, Equals(""));
    }

    SECTION("No spaces")
    {
        std::string noSpace{ "abc" };
        Trim(noSpace);
        CHECK_THAT(noSpace, Equals(noSpace));
    }

    SECTION("Leading spaces")
    {
        std::string space1{ " abc" };
        Trim(space1);
        CHECK_THAT(space1, Equals("abc"));

        std::string space3{ " \t abc" };
        Trim(space3);
        CHECK_THAT(space3, Equals("abc"));
    }

    SECTION("Trailing spaces")
    {
        std::string space1{ "abc " };
        Trim(space1);
        CHECK_THAT(space1, Equals("abc"));

        std::string space3{ "abc \t " };
        Trim(space3);
        CHECK_THAT(space3, Equals("abc"));
    }

    SECTION("Leading and trailing spaces")
    {
        std::string space1{ " abc " };
        Trim(space1);
        CHECK_THAT(space1, Equals("abc"));

        std::string space3{ " \t abc \t " };
        Trim(space3);
        CHECK_THAT(space3, Equals("abc"));
    }
}

TEST_CASE("Split")
{
    SECTION("SplitIntoTwoElements")
    {
        std::string s{ "abc:def" };
        std::vector<std::string> v = Split(s, ':');
        CHECK_THAT(v[0], Equals("abc"));
        CHECK_THAT(v[1], Equals("def"));
    }

    SECTION("SplitIntoOneElements")
    {
        std::string s{ "abcdef" };
        std::vector<std::string> v = Split(s, ':');
        CHECK_THAT(v[0], Equals("abcdef"));
    }

    SECTION("SplitIntoMoreThanTwoElements")
    {
        std::string s{ "abc:def:ghi" };
        std::vector<std::string> v = Split(s, ':');
        CHECK_THAT(v[0], Equals("abc"));
        CHECK_THAT(v[1], Equals("def"));
        CHECK_THAT(v[2], Equals("ghi"));
    }

    SECTION("EmptyString")
    {
        std::string s;
        std::vector<std::string> v = Split(s, ':');
        CHECK(v.empty());
    }

    SECTION("DifferentSeperators")
    {
        std::string s{ "abc def,ghi/jkl.mno" };
        std::vector<std::string> v1 = Split(s, ' ');
        CHECK_THAT(v1[0], Equals("abc"));
        CHECK_THAT(v1[1], Equals("def,ghi/jkl.mno"));

        std::vector<std::string> v2 = Split(s, ',');
        CHECK_THAT(v2[0], Equals("abc def"));
        CHECK_THAT(v2[1], Equals("ghi/jkl.mno"));

        std::vector<std::string> v3 = Split(s, '/');
        CHECK_THAT(v3[0], Equals("abc def,ghi"));
        CHECK_THAT(v3[1], Equals("jkl.mno"));

        std::vector<std::string> v4 = Split(s, '.');
        CHECK_THAT(v4[0], Equals("abc def,ghi/jkl"));
        CHECK_THAT(v4[1], Equals("mno"));
    }

    SECTION("EmptyElementCount")
    {
        std::string s1{ ":a::" };
        std::vector<std::string> v1 = Split(s1, ':');
        CHECK(v1.size() == 4);
        CHECK(v1[0].empty());
        CHECK_THAT(v1[1], Equals("a"));
        CHECK(v1[2].empty());
        CHECK(v1[3].empty());

        std::string s2{ ":::" };
        std::vector<std::string> v2 = Split(s2, ':');
        CHECK(v2.size() == 4);
        CHECK(v2[0].empty());
        CHECK(v2[1].empty());
        CHECK(v2[2].empty());
        CHECK(v2[3].empty());
    }
}

TEST_CASE("RemoveSurrounding")
{
    const char c = '\'';

    SECTION("empty")
    {
        std::string s{ "" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals(""));
    }

    SECTION("not surrounded")
    {
        std::string s{ "abc" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals("abc"));
    }

    SECTION("leading")
    {
        std::string s{ "'abc" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals("'abc"));
    }

    SECTION("trailing")
    {
        std::string s{ "abc'" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals("abc'"));
    }

    SECTION("leading and trailing")
    {
        std::string s{ "'abc'" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals("abc"));
    }

    SECTION("nested")
    {
        std::string s{ "''abc''" };
        ADUC::StringUtils::RemoveSurrounding(s, c);
        CHECK_THAT(s, Equals("'abc'"));
    }
}
