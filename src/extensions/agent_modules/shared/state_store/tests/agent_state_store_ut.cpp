/**
 * @file agent_state_store_ut.cpp
 * @brief Unit Tests for the Device Update agent state store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/agent_state_store.h"
#include <aduc/system_utils.h>
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;


#define TEST_STORE_DIR "/tmp/adutest/agent_state_store_ut"

#define TEST_STATE_FILE "/tmp/__aduc_state_store_ut__"

TEST_CASE("communication handle")
{
    SECTION("Valid handle")
    {
        ADUC_STATE_STORE_RESULT result = ADUC_StateStore_Initialize(TEST_STATE_FILE, false /* isUsingProvisioningService */);
        CHECK(result == ADUC_STATE_STORE_RESULT_OK);

        void* handle = (void*)reinterpret_cast<void*>(0x12345678);
        result = ADUC_StateStore_SetCommunicationChannelHandle(handle);
        CHECK(result == ADUC_STATE_STORE_RESULT_OK);

        const void* readHandle = ADUC_StateStore_GetCommunicationChannelHandle();
        CHECK(readHandle == handle);

        ADUC_StateStore_Deinitialize();
    }
}

TEST_CASE("ADUC_StateStore_Initialize")
{
    SECTION("state defaults correctly when no persisted files")
    {
        ADUC_SystemUtils_RmDirRecursive(TEST_STORE_DIR);
        REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize(TEST_STORE_DIR "/test_state_store.json", false /* isUsingProvisioningService */));

        CHECK(nullptr == ADUC_StateStore_GetExternalDeviceId());
        CHECK(nullptr == ADUC_StateStore_GetMQTTBrokerHostname());
        CHECK(nullptr == ADUC_StateStore_GetScopeId());
        CHECK_FALSE(ADUC_StateStore_GetIsDeviceRegistered());
        CHECK_FALSE(ADUC_StateStore_IsDeviceEnrolled());
        CHECK_FALSE(ADUC_StateStore_IsAgentInfoReported());
    }
}
