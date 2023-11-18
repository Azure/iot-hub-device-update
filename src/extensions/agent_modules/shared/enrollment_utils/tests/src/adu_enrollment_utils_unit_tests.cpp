/**
 * @file adu_enrollment_unit_tests.cpp
 * @brief Unit Tests for adu_enrollment_utils.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/adu_enrollment_utils.h>
#include <aduc/agent_state_store.h>
#include <aduc/system_utils.h> // ADUC_SystemUtils_RmDirRecursive
#include <cstring> // memset
#include <string>

#define ADU_ENROLLMENT_UTILS_UT_TEST_DIR "/tmp/adutest/adu_enrollment_utils_unit_tests"

char fakeDuInstance[] = "fooScopeId";
const char* is_enrolled_json = R"( { "IsEnrolled": true } )";
const char* is_not_enrolled_json = R"( { "IsEnrolled": true } )";

const char bad_json_payload[] = "{\"}";
const char enrolled_payload[] = "{ \"IsEnrolled\": true, \"ScopeId\": \"fooScope\" }";
const char not_enrolled_payload[] = "{ \"IsEnrolled\": false, \"ScopeId\": \"fooScope\" }";

static bool test_operation_complete_called = false;
static bool test_operation_complete(ADUC_Retriable_Operation_Context* context)
{
    test_operation_complete_called = true;
    return true;
}

static void reset()
{
    ADUC_SystemUtils_RmDirRecursive(ADU_ENROLLMENT_UTILS_UT_TEST_DIR);
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize(ADU_ENROLLMENT_UTILS_UT_TEST_DIR "/test_state_store.json"));
}

TEST_CASE("Handle_Enrollment_Response")
{
    SECTION("should return false for NULL enrollmentData")
    {
        ADUC_Retriable_Operation_Context operation_context;
        memset(&operation_context, 0, sizeof(operation_context));

        CHECK_FALSE(Handle_Enrollment_Response(
            nullptr, // ADUC_Enrollment_Request_Operation_Data* enrollmentData,
            false, // bool isEnrolled,
            &fakeDuInstance[0], // const char* duInstance,
            &operation_context));
    }

    SECTION("Handle_Enrollment_Response - not enrolled should set state store")
    {
        reset();

        ADUC_Enrollment_Request_Operation_Data operation_data;
        memset(&operation_data, 0, sizeof(operation_data));
        operation_data.respUserProps.pid = 1;
        operation_data.respUserProps.resultcode = ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS;
        operation_data.respUserProps.extendedresultcode = 0;

        ADUC_Retriable_Operation_Context operation_context;
        memset(&operation_context, 0, sizeof(operation_context));

        bool success = Handle_Enrollment_Response(
            &operation_data, // ADUC_Enrollment_Request_Operation_Data* enrollmentData,
            false, // bool isEnrolled,
            &fakeDuInstance[0], // const char* duInstance,
            &operation_context);
        CHECK(success);

        CHECK(operation_data.enrollmentState == ADU_ENROLLMENT_STATE_NOT_ENROLLED);

        CHECK_FALSE(ADUC_StateStore_GetIsDeviceEnrolled());
    }

    SECTION("Handle_Enrollment_Response - enrolled should set state store")
    {
        reset();

        ADUC_Enrollment_Request_Operation_Data operation_data;
        memset(&operation_data, 0, sizeof(operation_data));
        operation_data.respUserProps.pid = 1;
        operation_data.respUserProps.resultcode = ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS;
        operation_data.respUserProps.extendedresultcode = 0;

        ADUC_Retriable_Operation_Context operation_context;
        memset(&operation_context, 0, sizeof(operation_context));

        test_operation_complete_called = false;
        operation_context.completeFunc = test_operation_complete;

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
}

TEST_CASE("ParseEnrollmentMessagePayload")
{
    SECTION("bad args should return false")
    {
        bool isEnrolled = false;
        char* scopeId = NULL;

        CHECK_FALSE(ParseEnrollmentMessagePayload(nullptr, &isEnrolled, &scopeId));
        CHECK_FALSE(ParseEnrollmentMessagePayload(enrolled_payload, nullptr, &scopeId));
        CHECK_FALSE(ParseEnrollmentMessagePayload(enrolled_payload, &isEnrolled, nullptr));
        CHECK_FALSE(ParseEnrollmentMessagePayload(bad_json_payload, &isEnrolled, nullptr));

        CHECK(isEnrolled == false);
        CHECK(scopeId == NULL);
    }

    SECTION("ScopeId should be set")
    {
        bool isEnrolled = false;
        char* scopeId = NULL;

        CHECK(ParseEnrollmentMessagePayload(not_enrolled_payload, &isEnrolled, &scopeId));

        CHECK_THAT(scopeId, Equals("fooScope"));
        free(scopeId);
    }

    SECTION("not enrolled")
    {
        bool isEnrolled = true;
        char* scopeId = NULL;

        CHECK(ParseEnrollmentMessagePayload(not_enrolled_payload, &isEnrolled, &scopeId));

        CHECK_FALSE(isEnrolled);
        CHECK_THAT(scopeId, Equals("fooScope"));
        free(scopeId);
    }

    SECTION("enrolled")
    {
        bool isEnrolled = false;
        char* scopeId = NULL;

        CHECK(ParseEnrollmentMessagePayload(enrolled_payload, &isEnrolled, &scopeId));

        CHECK(isEnrolled);
        CHECK_THAT(scopeId, Equals("fooScope"));
        free(scopeId);
    }
}

TEST_CASE("EnrollmentData_SetCorrelationId")
{
    ADUC_Enrollment_Request_Operation_Data data;
    memset(&data, 0, sizeof(data));

    EnrollmentData_SetCorrelationId(&data, "b9c1c214-3d88-4db8-bd4e-6f19b0a79f82");
    CHECK_THAT(data.enrReqMessageContext.correlationId, Equals("b9c1c214-3d88-4db8-bd4e-6f19b0a79f82"));
}
