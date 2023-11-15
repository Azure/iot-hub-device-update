/**
 * @file adu_mqtt_common_unit_tests.cpp
 * @brief Unit Tests for adu_mqtt_common.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/adu_mqtt_common.h>
#include <aduc/agent_state_store.h>
#include <aduc/system_utils.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#define TEST_STATE_STORE_DIR "/tmp/adutest/adu_mqtt_common_unit_tests"

#define TEST_EXTERNAL_DEVICE_ID "d4757af7-9a58-4a8d-a3a6-b11257d25451"
#define TEST_SCOPE_ID "fooScope"

#define EXPECTED_PUBLISH_TOPIC "adu/oto/" TEST_EXTERNAL_DEVICE_ID "/a"
#define EXPECTED_PUBLISH_TOPIC_WITH_SCOPEID EXPECTED_PUBLISH_TOPIC "/" TEST_SCOPE_ID

#define EXPECTED_SUBSCRIBE_TOPIC "adu/oto/" TEST_EXTERNAL_DEVICE_ID "/s"
#define EXPECTED_SUBSCRIBE_TOPIC_WITH_SCOPEID EXPECTED_SUBSCRIBE_TOPIC "/" TEST_SCOPE_ID

static bool s_mock_retry_called = false;

static void reset()
{
    s_mock_retry_called = false;
    ADUC_SystemUtils_RmDirRecursive(TEST_STATE_STORE_DIR);
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize(TEST_STATE_STORE_DIR "/test_state_store.json"));
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_SetExternalDeviceId(TEST_EXTERNAL_DEVICE_ID));
    REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_SetDeviceUpdateServiceInstance(TEST_SCOPE_ID));
}

static bool mock_retry(ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams)
{
    s_mock_retry_called = true;
    return true;
}

TEST_CASE("MqttTopicSetupNeeded")
{
    SECTION("bad args")
    {
        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));
        ADUC_MQTT_Message_Context messageContext;
        memset(&messageContext, 0, sizeof(messageContext));

        CHECK_FALSE(MqttTopicSetupNeeded(nullptr, &messageContext, false /* isScoped */));
        CHECK_FALSE(MqttTopicSetupNeeded(&context, nullptr, false /* isScoped */));
    }

    SECTION("isScoped == true")
    {
        reset();

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));
        ADUC_MQTT_Message_Context messageContext;
        memset(&messageContext, 0, sizeof(messageContext));

        CHECK_FALSE(MqttTopicSetupNeeded(&context, &messageContext, true /* isScoped */)); // no cancel call needed.
        CHECK_THAT(messageContext.publishTopic, Equals(EXPECTED_PUBLISH_TOPIC_WITH_SCOPEID));
        CHECK_THAT(messageContext.responseTopic, Equals(EXPECTED_SUBSCRIBE_TOPIC_WITH_SCOPEID));
    }

    SECTION("isScoped == false")
    {
        reset();

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));
        ADUC_MQTT_Message_Context messageContext;
        memset(&messageContext, 0, sizeof(messageContext));

        CHECK_FALSE(MqttTopicSetupNeeded(&context, &messageContext, false /* isScoped */)); // no cancel call needed.
        CHECK_THAT(messageContext.publishTopic, Equals(EXPECTED_PUBLISH_TOPIC));
        CHECK_THAT(messageContext.responseTopic, Equals(EXPECTED_SUBSCRIBE_TOPIC));
    }
}

// bool ExternalDeviceIdSetupNeeded(ADUC_Retriable_Operation_Context* context);
TEST_CASE("ExternalDeviceIdSetupNeeded")
{
    SECTION("bad arg")
    {
        CHECK_FALSE(ExternalDeviceIdSetupNeeded(nullptr));
    }

    SECTION("null or empty external device id in store calls context retryFunc")
    {
        ADUC_SystemUtils_RmDirRecursive(TEST_STATE_STORE_DIR);
        REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_Initialize(TEST_STATE_STORE_DIR "/test_state_store.json"));
        REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_SetDeviceUpdateServiceInstance(TEST_SCOPE_ID));

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));

        context.retryFunc = mock_retry;
        s_mock_retry_called = false;

        CHECK(ExternalDeviceIdSetupNeeded(&context)); // returns true if setup needed (no external id set)
        CHECK(s_mock_retry_called);

        s_mock_retry_called = false;

        REQUIRE(ADUC_STATE_STORE_RESULT_OK == ADUC_StateStore_SetExternalDeviceId("")); // empty one still needs setup
        CHECK(ExternalDeviceIdSetupNeeded(&context)); // returns true if setup needed (no external id set)
        CHECK(s_mock_retry_called);
    }

    SECTION("return false(setup was not needed) when non-null external device id in store")
    {
        reset();

        ADUC_Retriable_Operation_Context context;
        memset(&context, 0, sizeof(context));

        CHECK_FALSE(ExternalDeviceIdSetupNeeded(&context));
        CHECK_FALSE(s_mock_retry_called);
    }
}
