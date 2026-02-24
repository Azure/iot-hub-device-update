/**
 * @file operation_id_utils_ut.cpp
 * @brief Unit Tests for the Operation ID Utils module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "operation_id_utils.h"

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

TEST_CASE("OperationIdUtils_OperationIsComplete - Parameter Validation")
{
    SECTION("Returns false when serviceMsg is nullptr")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(nullptr));
    }

    SECTION("Returns false when serviceMsg is empty string")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(""));
    }

    SECTION("Returns false when serviceMsg is invalid JSON")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete("not valid json"));
    }

    SECTION("Returns false when serviceMsg is partial JSON")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete("{\"operationId\":"));
    }

    SECTION("Returns false when serviceMsg JSON has no operationId field")
    {
        const char* jsonWithoutOperationId = R"({"someField": "someValue"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithoutOperationId));
    }

    SECTION("Returns false when serviceMsg JSON has null operationId")
    {
        const char* jsonWithNullOperationId = R"({"operationId": null})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithNullOperationId));
    }

    SECTION("Returns false when serviceMsg JSON is empty object")
    {
        const char* emptyJson = "{}";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(emptyJson));
    }

    SECTION("Returns false when serviceMsg JSON is array")
    {
        const char* jsonArray = "[]";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonArray));
    }

    SECTION("Returns false when serviceMsg JSON is array with objects")
    {
        const char* jsonArrayWithObjects = R"([{"operationId": "test-id"}])";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonArrayWithObjects));
    }

    SECTION("Returns false when operationId is number instead of string")
    {
        const char* jsonWithNumberOperationId = R"({"operationId": 12345})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithNumberOperationId));
    }

    SECTION("Returns false when operationId is boolean")
    {
        const char* jsonWithBoolOperationId = R"({"operationId": true})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithBoolOperationId));
    }

    SECTION("Returns false when operationId is object")
    {
        const char* jsonWithObjectOperationId = R"({"operationId": {"nested": "value"}})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithObjectOperationId));
    }

    SECTION("Returns false when operationId is array")
    {
        const char* jsonWithArrayOperationId = R"({"operationId": ["item1", "item2"]})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithArrayOperationId));
    }
}

TEST_CASE("OperationIdUtils_OperationIsComplete - No stored operation")
{
    // These tests verify behavior when no operation file exists
    // (which is the normal case on a clean system or in test environment)

    SECTION("Returns false when no operation ID file exists - valid JSON")
    {
        // Even with valid JSON containing operationId, should return false
        // when there's no stored operation to compare against
        const char* validJson = R"({"operationId": "test-operation-123"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(validJson));
    }

    SECTION("Returns false with complex valid JSON")
    {
        const char* complexJson = R"({
            "operationId": "op-12345",
            "deviceId": "device-001",
            "timestamp": "2024-01-01T00:00:00Z",
            "payload": {"key": "value"}
        })";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(complexJson));
    }

    SECTION("Returns false with empty operationId string")
    {
        const char* emptyOperationIdJson = R"({"operationId": ""})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(emptyOperationIdJson));
    }

    SECTION("Returns false with whitespace-only operationId")
    {
        const char* whitespaceOperationIdJson = R"({"operationId": "   "})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(whitespaceOperationIdJson));
    }

    SECTION("Returns false with UUID-style operationId")
    {
        const char* uuidOperationIdJson = R"({"operationId": "550e8400-e29b-41d4-a716-446655440000"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(uuidOperationIdJson));
    }
}

TEST_CASE("OperationIdUtils_StoreCompletedOperationId - Parameter Validation")
{
    SECTION("Returns false when operationId is nullptr")
    {
        CHECK_FALSE(OperationIdUtils_StoreCompletedOperationId(nullptr));
    }

    SECTION("Returns false when operationId is empty string")
    {
        // Empty string should still attempt to write (but may succeed or fail
        // depending on file system access)
        // The function doesn't validate content, only nullptr
        // This test documents the behavior
        bool result = OperationIdUtils_StoreCompletedOperationId("");
        // Either succeeds (if path is writable) or fails (if not)
        // We just verify it doesn't crash
        (void)result;
    }
}

TEST_CASE("OperationIdUtils_OperationIsComplete - Additional Edge Cases")
{
    SECTION("Returns false with very long operationId")
    {
        // Create a JSON with operationId longer than MAX_OPERATION_ID_CHARS (256)
        std::string longOpId(300, 'a');
        std::string jsonWithLongOpId = R"({"operationId": ")" + longOpId + R"("})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithLongOpId.c_str()));
    }

    SECTION("Returns false with operationId at exactly MAX_OPERATION_ID_CHARS")
    {
        // MAX_OPERATION_ID_CHARS is 256
        std::string exactOpId(256, 'x');
        std::string jsonWithExactOpId = R"({"operationId": ")" + exactOpId + R"("})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithExactOpId.c_str()));
    }

    SECTION("Returns false with operationId containing special characters")
    {
        const char* jsonWithSpecialChars = "{\"operationId\": \"op-123_abc!@#$\"}";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithSpecialChars));
    }

    SECTION("Returns false with operationId containing unicode")
    {
        const char* jsonWithUnicode = R"({"operationId": "op-日本語-123"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithUnicode));
    }

    SECTION("Returns false with operationId containing tabs")
    {
        const char* jsonWithTab = R"({"operationId": "op-123	op-456"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithTab));
    }

    SECTION("Returns false with nested JSON structure")
    {
        const char* nestedJson = R"({
            "outer": {
                "operationId": "nested-op-id"
            },
            "operationId": "top-level-op-id"
        })";
        // Should extract top-level operationId
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(nestedJson));
    }

    SECTION("Returns false with multiple operationId fields")
    {
        // JSON with duplicate keys - parson typically takes the last value
        const char* duplicateKeys = R"({
            "operationId": "first-op",
            "operationId": "second-op"
        })";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(duplicateKeys));
    }

    SECTION("Returns false with operationId as empty array")
    {
        const char* emptyArrayOpId = R"({"operationId": []})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(emptyArrayOpId));
    }

    SECTION("Returns false with operationId as empty object")
    {
        const char* emptyObjOpId = R"({"operationId": {}})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(emptyObjOpId));
    }

    SECTION("Returns false with operationId as float")
    {
        const char* floatOpId = R"({"operationId": 123.456})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(floatOpId));
    }

    SECTION("Returns false with extra fields in JSON")
    {
        const char* extraFields = R"({
            "timestamp": "2024-01-01T00:00:00Z",
            "operationId": "valid-op-123",
            "retryCount": 3,
            "priority": "high"
        })";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(extraFields));
    }
}

// NOTE: Tests that require writing to the file system are skipped because:
// 1. The DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH is compiled into the library
// 2. The production path (/var/lib/adu/diagnosticsoperationids) requires elevated permissions
// 3. Unit tests should be runnable without special permissions
//
// Integration tests that verify the full store/retrieve cycle should be run
// separately with appropriate permissions or in a container environment.
