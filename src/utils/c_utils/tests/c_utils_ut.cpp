/**
 * @file c_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include "aduc/c_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"

#include "aduc/calloc_wrapper.hpp"
using ADUC::StringUtils::cstr_wrapper;

#include <cstring>
#include <fstream>

class TemporaryTestFile
{
public:
    TemporaryTestFile(const TemporaryTestFile&) = delete;
    TemporaryTestFile& operator=(const TemporaryTestFile&) = delete;
    TemporaryTestFile(TemporaryTestFile&&) = delete;
    TemporaryTestFile& operator=(TemporaryTestFile&&) = delete;

    explicit TemporaryTestFile(const std::vector<std::string>& content)
    {
        // Generate a unique filename.
        ADUC_SystemUtils_MkTemp(_filePath);

        (void)std::remove(Filename());
        std::ofstream file{ Filename() };
        for (const std::string& line : content)
        {
            file << line << std::endl;
        }
    }

    ~TemporaryTestFile()
    {
        REQUIRE(std::remove(Filename()) == 0);
    }

    const char* Filename() const
    {
        return _filePath;
    }

private:
    char _filePath[ARRAY_SIZE("/tmp/tmpfileXXXXXX")] = "/tmp/tmpfileXXXXXX";
};

TEST_CASE("ARRAY_SIZE")
{
    SECTION("Inferred array size")
    {
        int array1[] = { 1 };
        int array3[] = { 1, 2, 3 };
        CHECK(ARRAY_SIZE(array1) == 1);
        CHECK(ARRAY_SIZE(array3) == 3);
    }

    SECTION("Explicit array size")
    {
        int array1[1];
        int array3[3];
        CHECK(ARRAY_SIZE(array1) == 1);
        CHECK(ARRAY_SIZE(array3) == 3);
    }

    SECTION("Inferred string size")
    {
        char stringEmpty[] = "";
        char stringABC[] = "abc";
        CHECK(ARRAY_SIZE(stringEmpty) == 1);
        CHECK(ARRAY_SIZE(stringABC) == 4);
    }

    SECTION("String literal size")
    {
        CHECK(ARRAY_SIZE("") == 1);
        CHECK(ARRAY_SIZE("abc") == 4);
    }
}

TEST_CASE("ADUC_StringUtils_Trim")
{
    SECTION("Null arg")
    {
        const char* result = ADUC_StringUtils_Trim(nullptr);
        CHECK(result == nullptr);
    }

    SECTION("Empty string")
    {
        char str[] = "";
        const char* result = ADUC_StringUtils_Trim(str);
        CHECK_THAT(result, Equals(""));
    }

    SECTION("String already trimmed")
    {
        char str[] = "abc";
        const char* result = ADUC_StringUtils_Trim(str);
        CHECK_THAT(result, Equals("abc"));
    }

    SECTION("Trim leading")
    {
        char str1[] = " abc";
        const char* result = ADUC_StringUtils_Trim(str1);
        CHECK_THAT(result, Equals("abc"));

        char str2[] = "  abc";
        result = ADUC_StringUtils_Trim(str2);
        CHECK_THAT(result, Equals("abc"));

        char str3[] = "  a b c";
        result = ADUC_StringUtils_Trim(str3);
        CHECK_THAT(result, Equals("a b c"));

        char str4[] = "\tabc";
        result = ADUC_StringUtils_Trim(str4);
        CHECK_THAT(result, Equals("abc"));
    }

    SECTION("Trim trailing")
    {
        char str1[] = "abc ";
        const char* result = ADUC_StringUtils_Trim(str1);
        CHECK_THAT(result, Equals("abc"));

        char str2[] = "abc  ";
        result = ADUC_StringUtils_Trim(str2);
        CHECK_THAT(result, Equals("abc"));

        char str3[] = "a b c ";
        result = ADUC_StringUtils_Trim(str3);
        CHECK_THAT(result, Equals("a b c"));

        char str4[] = "abc\t";
        result = ADUC_StringUtils_Trim(str4);
        CHECK_THAT(result, Equals("abc"));
    }

    SECTION("Trim leading and trailing")
    {
        char str1[] = " abc ";
        const char* result = ADUC_StringUtils_Trim(str1);
        CHECK_THAT(result, Equals("abc"));

        char str2[] = "  abc  ";
        result = ADUC_StringUtils_Trim(str2);
        CHECK_THAT(result, Equals("abc"));

        char str3[] = " a b c ";
        result = ADUC_StringUtils_Trim(str3);
        CHECK_THAT(result, Equals("a b c"));

        char str4[] = "\tabc\t";
        result = ADUC_StringUtils_Trim(str4);
        CHECK_THAT(result, Equals("abc"));
    }
}

TEST_CASE("ADUC_ParseUpdateType and atoui")
{
    SECTION("Empty String")
    {
        const char* updateType = "";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Missing Update Name")
    {
        const char* updateType = ":";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Missing Version Number")
    {
        const char* updateType = "microsoft/apt:";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Missing Delimiter")
    {
        const char* updateType = "microsoft/apt.1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Negative Number")
    {
        const char* updateType = "microsoft/apt:-1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Ransome Negative Number")
    {
        const char* updateType = "microsoft/apt:-1123";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Zero")
    {
        const char* updateType = "microsoft/apt:0";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 0);
        free(updateTypeName);
    }

    SECTION("Positive Number")
    {
        const char* updateType = "microsoft/apt:1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 1);
        free(updateTypeName);
    }

    SECTION("Positive Large Number")
    {
        const char* updateType = "microsoft/apt:4294967294";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 4294967294);
        free(updateTypeName);
    }

    SECTION("Positive UINT MAX")
    {
        const char* updateType = "microsoft/apt:4294967295";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Positive Larger Than UINT MAX")
    {
        const char* updateType = "microsoft/apt:4294967296";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Positive ULONG MAX")
    {
        const char* updateType = "microsoft/apt:18446744073709551615";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Version contains space")
    {
        const char* updateType = "microsoft/apt: 1 ";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }

    SECTION("Decimal version")
    {
        const char* updateType = "microsoft/apt:1.2";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        free(updateTypeName);
    }
}

TEST_CASE("atoul")
{
    SECTION("Empty String")
    {
        const char* str = "";
        unsigned long val = 0;
        CHECK(!atoul(str, &val));
    }

    SECTION("Invalid Character")
    {
        const char* str = "*";
        unsigned long val = 0;
        CHECK(!atoul(str, &val));
    }

    SECTION("Invalid number")
    {
        const char* str = "500*";
        unsigned long val = 0;
        CHECK(!atoul(str, &val));
    }

    SECTION("Positive number")
    {
        const char* str = "500";
        unsigned long val = 0;
        CHECK(atoul(str, &val));
        CHECK(val == 500);
    }

    SECTION("Positive Large Number")
    {
        const char* str = "4294967294";
        unsigned long val = 0;
        CHECK(atoul(str, &val));
        CHECK(val == 4294967294);
    }

    SECTION("Negative number")
    {
        const char* str = "-123";
        unsigned long val = 0;
        CHECK(!atoul(str, &val));
    }

    SECTION("Zero")
    {
        const char* str = "0";
        unsigned long val = 0;
        CHECK(atoul(str, &val));
        CHECK(val == 0);
    }
}

TEST_CASE("ADUC_StrNLen")
{
    SECTION("Check string in bounds")
    {
        std::string testStr = "foobar";

        CHECK(ADUC_StrNLen(testStr.c_str(), 10) == testStr.length());
    }
    SECTION("Check null string")
    {
        const char* testStr = nullptr;

        CHECK(ADUC_StrNLen(testStr, 10) == 0);
    }
    SECTION("Check empty string")
    {
        const char* testStr = "";

        CHECK(ADUC_StrNLen(testStr, 10) == 0);
    }
    SECTION("Check string out of bounds")
    {
        std::string testStr = "foobar";
        size_t max = 2;

        CHECK(ADUC_StrNLen(testStr.c_str(), max) == max);
    }
    SECTION("Check duck emoji codepoint")
    {
        // The "duck" emoji is unicode codepoint: U+1F986
        // which falls in the range of U+10000 - U+10FFFF, so in utf-8 it is encoded in the 4-byte form:
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        CHECK(2 == ADUC_StrNLen("🦆", 2));
        CHECK(3 == ADUC_StrNLen("🦆", 3));
        CHECK(4 == ADUC_StrNLen("🦆", 4));
        CHECK(4 == ADUC_StrNLen("🦆", 5));
    }
}

TEST_CASE("ADUC_StringFormat")
{
    SECTION("Create Formatted String")
    {
        const std::string expectedRetVal{ "Host=Local,Port=10,Token=asdfg" };
        const cstr_wrapper retval{ ADUC_StringFormat("Host=%s,Port=%i,Token=%s", "Local", 10, "asdfg") };

        CHECK(expectedRetVal == retval.get());
    }

    SECTION("nullptr Format")
    {
        const cstr_wrapper retval{ ADUC_StringFormat(nullptr, "Foo", "Bar") };

        CHECK(retval.get() == nullptr);
    }

    SECTION("Input strings are larger than 4096")
    {
        const std::string tooLongInput(4097, 'a');

        const std::string fmt{ "Token=%s" };

        const cstr_wrapper retval{ ADUC_StringFormat(fmt.c_str(), tooLongInput.c_str()) };

        CHECK(retval.get() == nullptr);
    }
}

TEST_CASE("ADUC_Safe_StrCopyN properly copies strings") {
    char dest[10] = "foo";

    const auto reset_dest = [&dest] {
        dest[0] = 'f';
        dest[1] = '\0';
    };

    // Edge cases

    SECTION("Handle NULL source") {
        reset_dest();
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(dest, sizeof(dest), NULL, 0));
        CHECK_THAT(dest, Equals("f")); // did not write to dest when NULL arg
    }

    SECTION("Handle NULL destination") {
        reset_dest();
        const char* src = "test";
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(NULL, sizeof(dest), src, 4));
        CHECK_THAT(dest, Equals("f")); // did not write to dest when NULL arg
    }

    SECTION("Handle zero size") {
        reset_dest();
        const char* src = "test";
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(dest, 0 /* destByteLen */, src, 4 /* srcByteLen */));
        CHECK_THAT(dest, Equals("f")); // did not write to dest when told dest len is 0
    }

    // mainline cases

    SECTION("src chars to cpy < dest capacity should succeed") {
        reset_dest();
        const char* src = "short";
        REQUIRE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 5));
        CHECK_THAT(dest, Equals("short"));
    }

    SECTION("Error when truncation would be needed") {
        reset_dest();
        const char* src = "12345678901234"; // 14 + 1
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 14));
        CHECK_THAT(dest, Equals(""));
    }

    SECTION("Handle subset of longer source string that is still longer than dest") {
        reset_dest();
        const char* src = "12345678901234"; // 14 + 1
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 11));
        CHECK_THAT(dest, Equals(""));

        reset_dest();
        REQUIRE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 9));
        CHECK_THAT(dest, Equals("123456789"));
    }

    SECTION("Handle subset of longer source string, exactly as long as dest buffer - 1") {
        reset_dest();
        const char* src = "12345678901234"; // 14 + 1
        REQUIRE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 9));
        CHECK_THAT(dest, Equals("123456789"));
    }

    SECTION("Handle subset of longer source string, that is less-than dest buffer - 1") {
        reset_dest();
        const char* src = "12345678901234"; // 14 + 1
        REQUIRE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, 8));
        CHECK_THAT(dest, Equals("12345678"));
    }

    SECTION("numSrcCharsToCopy > len of src str")
    {
        reset_dest();
        const char* src = "123";
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(dest, sizeof(dest), src, strlen(src) + 1));
        CHECK_THAT(dest, Equals(""));
    }

    SECTION("Can copy a duck (ADUC) emoji")
    {
        char target[5];
        memset(&target[0], 0, 4);

        const char* src = "🦆"; // 4-byte utf-8 sequence, so target must be size 5 for nul-term.
        REQUIRE(ADUC_Safe_StrCopyN(target, sizeof(target), src, strlen(src)));
        CHECK_THAT(target, Equals("🦆"));
    }

    SECTION("Not enough space for a duck")
    {
        char target[4]; // needs to be 5 for nul-term
        memset(&target[0], 0, 4);

        const char* src = "🦆"; // 4-byte utf-8 sequence, so target must be size 5 for nul-term.
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(target, sizeof(target), src, strlen(src)));
        CHECK_THAT(target, Equals(""));
    }

    SECTION("dest has insufficient space for src + nul-term should fail")
    {
        char target[4];
        memset(target, 0, 4);

        const char* src = "1234";
        REQUIRE_FALSE(ADUC_Safe_StrCopyN(target, sizeof(target), src, strlen(src)));
        CHECK_THAT(target, Equals(""));
    }
}

TEST_CASE("ADUC_AllocAndStrCopyN")
{
    SECTION("returns false for invalid arg")
    {
        CHECK_FALSE(ADUC_AllocAndStrCopyN(NULL /* dest */, "foo", 3));

        char* target = NULL;
        CHECK_FALSE(ADUC_AllocAndStrCopyN(&target, "foo", 0));

        CHECK_FALSE(ADUC_AllocAndStrCopyN(&target, nullptr, 3));
    }

    SECTION("returns true on success")
    {
        {
            ADUC::StringUtils::cstr_wrapper target;
            REQUIRE(ADUC_AllocAndStrCopyN(target.address_of(), "foo", 3));
            CHECK_THAT(target.get(), Equals("foo"));
        }

        {
            ADUC::StringUtils::cstr_wrapper target;
            REQUIRE(ADUC_AllocAndStrCopyN(target.address_of(), "foo", 2));
            CHECK_THAT(target.get(), Equals("fo"));
        }

        {
            ADUC::StringUtils::cstr_wrapper target;
            REQUIRE(ADUC_AllocAndStrCopyN(target.address_of(), "foo", 1));
            CHECK_THAT(target.get(), Equals("f"));
        }
    }
}
