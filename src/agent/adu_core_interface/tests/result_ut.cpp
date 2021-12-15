/**
 * @file result_ut.cpp
 * @brief Unit tests for result.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>

#include "aduc/result.h"

static uint8_t FacilityFromExtendedResultCode(ADUC_Result_t extendedResultCode)
{
    return (static_cast<uint32_t>(extendedResultCode) >> 0x1C) & 0xF;
}

static uint32_t CodeFromExtendedResultCode(ADUC_Result_t extendedResultCode)
{
    return extendedResultCode & 0xFFFFFFF;
}

TEST_CASE("IsAducResultCode: Valid")
{
    auto resultVal = GENERATE(as<ADUC_Result_t>{}, 1, 2, INT32_MAX); // NOLINT(google-build-using-namespace)

    INFO(resultVal);
    CHECK(IsAducResultCodeSuccess(resultVal));
}

TEST_CASE("IsAducResultCode: Invalid")
{
    auto resultVal = GENERATE(as<ADUC_Result_t>{}, 0, -1, INT32_MIN); // NOLINT(google-build-using-namespace)

    INFO(resultVal);
    CHECK(IsAducResultCodeFailure(resultVal));
}

TEST_CASE("EXTENDEDRESULTCODE")
{
    auto errorVal = GENERATE(as<unsigned int>{}, 0, 1, 0xfffff); // NOLINT(google-build-using-namespace)

    SECTION("MAKE_ADUC_EXTENDEDRESULTCODE")
    {
        const ADUC_Result_t erc{ MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_LOWERLAYER, 0, errorVal) };
        CHECK(FacilityFromExtendedResultCode(erc) == ADUC_FACILITY_LOWERLAYER);
        CHECK(CodeFromExtendedResultCode(erc) == errorVal);
    }

    SECTION("ADUC_FACILITY_DELIVERY_OPTIMIZATION")
    {
        const ADUC_Result_t erc{ MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(errorVal) };
        CHECK(FacilityFromExtendedResultCode(erc) == ADUC_FACILITY_DELIVERY_OPTIMIZATION);
        CHECK(CodeFromExtendedResultCode(erc) == errorVal);
    }

    SECTION("ADUC_FACILITY_LOWERLAYER")
    {
        const ADUC_Result_t erc{ MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(errorVal) };
        CHECK(FacilityFromExtendedResultCode(erc) == ADUC_FACILITY_UNKNOWN);
        CHECK(CodeFromExtendedResultCode(erc) == errorVal);
    }
}

// Note: /usr/include/asm-generic/errno.h definitions

#define EPERM 1
#define ENOMEM 12
#define ENOTRECOVERABLE 131

TEST_CASE("ADUC_ERC Macros")
{
    CHECK(FacilityFromExtendedResultCode(ADUC_ERC_NOTRECOVERABLE) == ADUC_FACILITY_UNKNOWN);
    CHECK(CodeFromExtendedResultCode(ADUC_ERC_NOTRECOVERABLE) == ENOTRECOVERABLE);

    CHECK(FacilityFromExtendedResultCode(ADUC_ERC_NOMEM) == ADUC_FACILITY_UNKNOWN);
    CHECK(CodeFromExtendedResultCode(ADUC_ERC_NOMEM) == ENOMEM);

    CHECK(FacilityFromExtendedResultCode(ADUC_ERC_NOTPERMITTED) == ADUC_FACILITY_UNKNOWN);
    CHECK(CodeFromExtendedResultCode(ADUC_ERC_NOTPERMITTED) == EPERM);
}
