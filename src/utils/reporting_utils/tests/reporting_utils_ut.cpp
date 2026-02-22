/**
 * @file reporting_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;
#include <aduc/reporting_utils.h>
#include <aduc/string_handle_wrapper.hpp>
#include <stddef.h>

TEST_CASE("ADUC_ReportingUtils_StringHandleFromVectorInt32")
{
    SECTION("zero elements")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 4);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(""));

        VECTOR_destroy(extended_result_codes);
    }

    SECTION("one element")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        int32_t val = 0x00000001;
        VECTOR_push_back(extended_result_codes, &val, 1);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 4);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(",00000001"));

        VECTOR_destroy(extended_result_codes);
    }

    SECTION("2 elements")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        int32_t val = 0x00000001;
        VECTOR_push_back(extended_result_codes, &val, 1);

        int32_t val2 = 0x00000002;
        VECTOR_push_back(extended_result_codes, &val2, 1);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 4);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(",00000001,00000002"));

        VECTOR_destroy(extended_result_codes);
    }

    SECTION("max 1")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        int32_t val = 0x00000001;
        VECTOR_push_back(extended_result_codes, &val, 1);

        int32_t val2 = 0x00000002;
        VECTOR_push_back(extended_result_codes, &val2, 1);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 1);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(",00000001"));

        VECTOR_destroy(extended_result_codes);
    }

    SECTION("max 2")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        int32_t val = 0x00000001;
        VECTOR_push_back(extended_result_codes, &val, 1);

        int32_t val2 = 0x00000002;
        VECTOR_push_back(extended_result_codes, &val2, 1);

        int32_t val3 = 0x00000003;
        VECTOR_push_back(extended_result_codes, &val3, 1);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 2);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(",00000001,00000002"));

        VECTOR_destroy(extended_result_codes);
    }

    SECTION("max zero returns empty string")
    {
        VECTOR_HANDLE extended_result_codes = VECTOR_create(sizeof(int32_t));
        REQUIRE(extended_result_codes != NULL);

        int32_t val = 0x00000001;
        VECTOR_push_back(extended_result_codes, &val, 1);

        STRING_HANDLE delimited_str = ADUC_ReportingUtils_StringHandleFromVectorInt32(extended_result_codes, 0);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ delimited_str };
        CHECK(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(""));

        VECTOR_destroy(extended_result_codes);
    }
}

TEST_CASE("ADUC_ReportingUtils_CreateReportingErcHexStr")
{
    SECTION("first element has no comma")
    {
        STRING_HANDLE str = ADUC_ReportingUtils_CreateReportingErcHexStr(0x1234ABCD, true);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ str };
        REQUIRE(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals("1234ABCD"));
    }

    SECTION("non-first element is comma-prefixed")
    {
        STRING_HANDLE str = ADUC_ReportingUtils_CreateReportingErcHexStr(0x1234ABCD, false);
        ADUC::StringUtils::STRING_HANDLE_wrapper wrapped{ str };
        REQUIRE(!wrapped.is_null());
        CHECK_THAT(STRING_c_str(wrapped.get()), Equals(",1234ABCD"));
    }
}
