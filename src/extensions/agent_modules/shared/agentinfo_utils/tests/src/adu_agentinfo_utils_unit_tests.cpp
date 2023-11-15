/**
 * @file adu_agentinfo_unit_tests.cpp
 * @brief Unit Tests for adu_agentinfo_utils.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
#include <aduc/adu_agentinfo_utils.h>
#include <aduc/agent_state_store.h>
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <cstring> // memset

bool s_cancel_called = false;

bool Mock_AgentInfoRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    s_cancel_called = true;
    return true;
}

void reset()
{
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize("/tmp/adutest/adu_agentinfo_utils_unit_tests/test_state_store.h"));

    s_cancel_called = false;
}

TEST_CASE("Handle_AgentInfo_Response - sets agent state and state store to unknown")
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
    CHECK_FALSE(ADUC_StateStore_GetIsAgentInfoReported());
}

TEST_CASE("Handle_AgentInfo_Response - Bad request calls retriable context cancelFunc")
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
    CHECK_FALSE(ADUC_StateStore_GetIsAgentInfoReported());

    CHECK(s_cancel_called);
}

TEST_CASE("Handle_AgentInfo_Response - Success should set StateStore AgentInfoReported and ACKNOWLEDGED")
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

    CHECK(ADUC_StateStore_GetIsAgentInfoReported());
    CHECK(agentInfoData.agentInfoState == ADU_AGENTINFO_STATE_ACKNOWLEDGED);
}
