/**
 * @file reporting_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
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
    }
}
