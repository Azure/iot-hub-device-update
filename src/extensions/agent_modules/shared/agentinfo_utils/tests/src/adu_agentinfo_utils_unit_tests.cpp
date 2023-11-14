/**
 * @file adu_agentinfo_unit_tests.cpp
 * @brief Unit Tests for adu_agentinfo_utils.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
#include <aduc/adu_agentinfo_utils.h>
#include <cstring> // memset

TEST_CASE("Handle_AgentInfo_Response")
{
    ADUC_Retriable_Operation_Context operation_context;
    memset(&operation_context, 0, sizeof(operation_context));

    CHECK(Handle_AgentInfo_Response(200));
    CHECK_FALSE(Handle_AgentInfo_Response(500));
}
