/**
 * @file adu_agentinfo_unit_tests.cpp
 * @brief Unit Tests for adu_agentinfo_utils.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <aduc/adu_agentinfo_utils.h>
#include <aduc/agent_state_store.h>
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/system_utils.h> // ADUC_SystemUtils_RmDirRecursive
#include <cstring> // memset

#define ADU_AGENTINFO_UTILS_UT_TEST_DIR "/tmp/adutest/adu_agentinfo_utils_unit_tests"

static bool s_cancel_called = false;

static bool Mock_AgentInfoRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    s_cancel_called = true;
    return true;
}

static void reset()
{
    ADUC_SystemUtils_RmDirRecursive(ADU_AGENTINFO_UTILS_UT_TEST_DIR);
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize(ADU_AGENTINFO_UTILS_UT_TEST_DIR "/test_state_store.h"));

    s_cancel_called = false;
}

TEST_CASE("Handle_AgentInfo_Response")
{
    SECTION("sets agent state and state store to unknown")
    {
        reset();

        ADUC_AgentInfo_Request_Operation_Data agentInfoData;
        agentInfoData.agentInfoState = ADU_AGENTINFO_STATE_REQUESTING;
        agentInfoData.respUserProps.pid = 1;
        agentInfoData.respUserProps.resultcode = ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY;
        agentInfoData.respUserProps.extendedresultcode = 3;

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));

        REQUIRE_FALSE(Handle_AgentInfo_Response(&agentInfoData, &context));

        CHECK(agentInfoData.agentInfoState == ADU_AGENTINFO_STATE_UNKNOWN);
        CHECK_FALSE(ADUC_StateStore_IsAgentInfoReported());
    }

    SECTION("Bad request calls retriable context cancelFunc")
    {
        reset();

        ADUC_AgentInfo_Request_Operation_Data agentInfoData;
        agentInfoData.agentInfoState = ADU_AGENTINFO_STATE_REQUESTING;
        agentInfoData.respUserProps.pid = 1;
        agentInfoData.respUserProps.resultcode = ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST;
        agentInfoData.respUserProps.extendedresultcode = 3;

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));
        context.cancelFunc = Mock_AgentInfoRequestOperation_CancelOperation;

        REQUIRE_FALSE(Handle_AgentInfo_Response(&agentInfoData, &context));

        CHECK(agentInfoData.agentInfoState == ADU_AGENTINFO_STATE_UNKNOWN);
        CHECK_FALSE(ADUC_StateStore_IsAgentInfoReported());

        CHECK(s_cancel_called);
    }

    SECTION("Success should set StateStore AgentInfoReported and ACKNOWLEDGED")
    {
        reset();

        ADUC_AgentInfo_Request_Operation_Data agentInfoData;
        agentInfoData.agentInfoState = ADU_AGENTINFO_STATE_REQUESTING;
        agentInfoData.respUserProps.pid = 1;
        agentInfoData.respUserProps.resultcode = ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS;
        agentInfoData.respUserProps.extendedresultcode = 0;

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));

        REQUIRE(Handle_AgentInfo_Response(&agentInfoData, &context));

        CHECK(ADUC_StateStore_IsAgentInfoReported());
        CHECK(agentInfoData.agentInfoState == ADU_AGENTINFO_STATE_ACKNOWLEDGED);
    }
}

TEST_CASE("AgentInfoData_SetCorrelationId")
{
    ADUC_AgentInfo_Request_Operation_Data data;
    memset(&data, 0, sizeof(data));

    AgentInfoData_SetCorrelationId(&data, "b9c1c214-3d88-4db8-bd4e-6f19b0a79f82");
    CHECK_THAT(data.ainfoReqMessageContext.correlationId, Equals("b9c1c214-3d88-4db8-bd4e-6f19b0a79f82"));
}
