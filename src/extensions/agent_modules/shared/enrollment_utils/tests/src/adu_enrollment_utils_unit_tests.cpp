/**
 * @file adu_enrollment_unit_tests.cpp
 * @brief Unit Tests for adu_enrollment_utils.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <aduc/adu_enrollment_utils.h>
#include <aduc/agent_state_store.h>
#include <cstring> // memset
#include <string>

char fakeDuInstance[] = "fooScopeId";
const char* is_enrolled_json = R"( { "IsEnrolled": true } )";
const char* is_not_enrolled_json = R"( { "IsEnrolled": true } )";

bool test_operation_complete_called = false;
bool test_operation_complete(ADUC_Retriable_Operation_Context* context)
{
    test_operation_complete_called = true;
    return true;
}

TEST_CASE("Handle_Enrollment_Response - should return false for NULL enrollmentData")
{
    ADUC_Retriable_Operation_Context operation_context;
    memset(&operation_context, 0, sizeof(operation_context));

    CHECK_FALSE(Handle_Enrollment_Response(
        nullptr, // ADUC_Enrollment_Request_Operation_Data* enrollmentData,
        false, // bool isEnrolled,
        &fakeDuInstance[0], // const char* duInstance,
        &operation_context));
}

TEST_CASE("Handle_Enrollment_Response - not enrolled should set state store")
{
    ADUC_Enrollment_Request_Operation_Data operation_data;
    memset(&operation_data, 0, sizeof(operation_data));

    ADUC_Retriable_Operation_Context operation_context;
    memset(&operation_context, 0, sizeof(operation_context));

    CHECK(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize("/tmp/adutest/test_state_store.json"));

    bool success = Handle_Enrollment_Response(
        &operation_data, // ADUC_Enrollment_Request_Operation_Data* enrollmentData,
        false, // bool isEnrolled,
        &fakeDuInstance[0], // const char* duInstance,
        &operation_context);
    CHECK(success);

    CHECK(operation_data.enrollmentState == ADU_ENROLLMENT_STATE_NOT_ENROLLED);

    CHECK_FALSE(ADUC_StateStore_GetIsDeviceEnrolled());
}

TEST_CASE("Handle_Enrollment_Response - enrolled should set state store")
{
    ADUC_Enrollment_Request_Operation_Data operation_data;
    memset(&operation_data, 0, sizeof(operation_data));

    ADUC_Retriable_Operation_Context operation_context;
    memset(&operation_context, 0, sizeof(operation_context));

    test_operation_complete_called = false;
    operation_context.completeFunc = test_operation_complete;

    CHECK(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize("/tmp/adutest/test_state_store.json"));

    bool success = Handle_Enrollment_Response(
        &operation_data, // ADUC_Enrollment_Request_Operation_Data* enrollmentData,
        true, // bool isEnrolled,
        &fakeDuInstance[0], // const char* duInstance,
        &operation_context);
    CHECK(success);

    CHECK(test_operation_complete_called);
    CHECK(operation_data.enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED);

    CHECK(ADUC_StateStore_GetIsDeviceEnrolled());
}
