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
        int result = mkstemp(_filePath);
        REQUIRE(result != -1);

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

TEST_CASE("ReadDelimitedValueFromFile")
{
    const unsigned int valueLen = 20;
    char value[valueLen];

    SECTION("Valid file")
    {
        // clang-format off
        const std::vector<std::string> content{
            "Key1=Value1",
            "NotAKeyValuePair",
            "Key2=Value2",
            "=DoesntHaveKey",
            "TwentyCharacterValue=12345678901234567890",
            "Key3=Value3",
            "NineteenCharacterValue=1234567890123456789",
        };
        // clang-format on
        TemporaryTestFile file{ content };

        // Valid keys
        CHECK(ReadDelimitedValueFromFile(file.Filename(), "Key1", value, valueLen));
        CHECK(std::strcmp(value, "Value1") == 0);
        CHECK(ReadDelimitedValueFromFile(file.Filename(), "Key2", value, valueLen));
        CHECK(std::strcmp(value, "Value2") == 0);
        CHECK(ReadDelimitedValueFromFile(file.Filename(), "Key3", value, valueLen));
        CHECK(std::strcmp(value, "Value3") == 0);

        // Long values (around valueLen in size)
        CHECK(ReadDelimitedValueFromFile(file.Filename(), "NineteenCharacterValue", value, valueLen));
        CHECK(std::strcmp(value, "1234567890123456789") == 0);
        CHECK(!ReadDelimitedValueFromFile(file.Filename(), "TwentyCharacterValue", value, valueLen));

        // Invalid key
        CHECK(!ReadDelimitedValueFromFile(file.Filename(), "NotAKeyValuePair", value, valueLen));
    }

    SECTION("Missing or empty file")
    {
        const std::vector<std::string> content;
        TemporaryTestFile file{ content };

        // Missing file
        CHECK(!ReadDelimitedValueFromFile("/tmp/__filenamedoesntexist__", "Key1", value, valueLen));

        // Empty file
        CHECK(!ReadDelimitedValueFromFile(file.Filename(), "Key1", value, valueLen));
    }
}

TEST_CASE("ARRAY_SIZE")
{
    SECTION("Inferred array size")
    {
        int array0[] = {};
        int array3[] = { 1, 2, 3 };
        CHECK(ARRAY_SIZE(array0) == 0);
        CHECK(ARRAY_SIZE(array3) == 3);
    }

    SECTION("Explicit array size")
    {
        int array0[0];
        int array3[3];
        CHECK(ARRAY_SIZE(array0) == 0);
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
    }

    SECTION("Missing Update Name")
    {
        const char* updateType = ":";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Missing Version Number")
    {
        const char* updateType = "microsoft/apt:";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Missing Delimiter")
    {
        const char* updateType = "microsoft/apt.1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Negative Number")
    {
        const char* updateType = "microsoft/apt:-1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Ransome Negative Number")
    {
        const char* updateType = "microsoft/apt:-1123";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Zero")
    {
        const char* updateType = "microsoft/apt:0";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 0);
    }

    SECTION("Positive Number")
    {
        const char* updateType = "microsoft/apt:1";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 1);
    }

    SECTION("Positive Large Number")
    {
        const char* updateType = "microsoft/apt:4294967294";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
        CHECK_THAT(updateTypeName, Equals("microsoft/apt"));
        CHECK(updateTypeVersion == 4294967294);
    }

    SECTION("Positive UINT MAX")
    {
        const char* updateType = "microsoft/apt:4294967295";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Positive Larger Than UINT MAX")
    {
        const char* updateType = "microsoft/apt:4294967296";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Positive ULONG MAX")
    {
        const char* updateType = "microsoft/apt:18446744073709551615";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Version contains space")
    {
        const char* updateType = "microsoft/apt: 1 ";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
    }

    SECTION("Decimal version")
    {
        const char* updateType = "microsoft/apt:1.2";
        char* updateTypeName = nullptr;
        unsigned int updateTypeVersion;
        CHECK(!ADUC_ParseUpdateType(updateType, &updateTypeName, &updateTypeVersion));
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
        size_t max = 10;

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
        ;

        CHECK(ADUC_StrNLen(testStr, 10) == 0);
    }
    SECTION("Check string out of bounds")
    {
        std::string testStr = "foobar";
        size_t max = 2;

        CHECK(ADUC_StrNLen(testStr.c_str(), max) == max);
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
