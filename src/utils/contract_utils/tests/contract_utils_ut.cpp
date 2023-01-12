/**
 * @file contract_utils_ut.cpp
 * @brief Unit Tests for contract_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/contract_utils.h"

#include <catch2/catch.hpp>

TEST_CASE("ADUC_ContractUtils_IsV1Contract")
{
    SECTION("NULL contractInfo")
    {
        ADUC_ExtensionContractInfo* info = nullptr;
        bool isV1 = ADUC_ContractUtils_IsV1Contract(info);
        CHECK(isV1);
    }

    SECTION("Not explicitly 1.0")
    {
        ADUC_ExtensionContractInfo contractInfo{};

        // all zero
        CHECK_FALSE(ADUC_ContractUtils_IsV1Contract(&contractInfo));

        // other than 1.0
        contractInfo.majorVer = 1;
        contractInfo.minorVer = 1;
        CHECK_FALSE(ADUC_ContractUtils_IsV1Contract(&contractInfo));

        contractInfo.majorVer = 2;
        contractInfo.minorVer = 0;
        CHECK_FALSE(ADUC_ContractUtils_IsV1Contract(&contractInfo));
    }

    SECTION("Is 1.0")
    {
        ADUC_ExtensionContractInfo contractInfo{};
        contractInfo.majorVer = 1;
        contractInfo.minorVer = 0;

        CHECK(ADUC_ContractUtils_IsV1Contract(&contractInfo));
    }
}
