/**
 * @file agent_state_store_ut.cpp
 * @brief Unit Tests for the Device Update agent state store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include "aduc/agent_state_store.h"

#define TEST_STATE_FILE "/tmp/__aduc_state_store_ut__"

TEST_CASE("communication handle")
{
    SECTION("Valid handle")
    {
        ADUC_STATE_STORE_RESULT result = ADUC_StateStore_Initialize(TEST_STATE_FILE);
        CHECK(result == ADUC_STATE_STORE_RESULT_OK);

        void* handle = (void*)reinterpret_cast<void*>(0x12345678);
        result = ADUC_StateStore_SetCommunicationChannelHandle("session_1", handle);
        CHECK(result == ADUC_STATE_STORE_RESULT_OK);

        const void* readHandle = ADUC_StateStore_GetCommunicationChannelHandle("session_1");
        CHECK(readHandle == handle);

        ADUC_StateStore_Deinitialize();
    }
}
