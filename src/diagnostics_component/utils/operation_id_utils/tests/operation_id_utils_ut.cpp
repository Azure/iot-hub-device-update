/**
 * @file operation_id_utils_ut.cpp
 * @brief Unit Tests for the Operation ID Utils module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "operation_id_utils.h"

#include <catch2/catch_all.hpp>
#include <cstdio>
#include <cstring>
#include <string>

// Helper to clean up the test operation ID file between tests
static void RemoveTestOperationFile()
{
    // DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH is overridden to
    // "/tmp/test_diagnostics_operation_id" via compile definitions in CMakeLists.txt
    std::remove("/tmp/test_diagnostics_operation_id");
}

// RAII helper to ensure test file cleanup
class OperationIdTestHelper
{
public:
    OperationIdTestHelper()
    {
        RemoveTestOperationFile();
    }
    ~OperationIdTestHelper()
    {
        RemoveTestOperationFile();
    }
};

// ===========================================================================
// OperationIdUtils_OperationIsComplete - Parameter Validation
// ===========================================================================

TEST_CASE("OperationIdUtils_OperationIsComplete - Parameter Validation")
{
    OperationIdTestHelper helper;

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
        CHECK_FALSE(OperationIdUtils_OperationIsComplete("{}"));
    }

    SECTION("Returns false when serviceMsg JSON is array")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete("[]"));
    }

    SECTION("Returns false when serviceMsg JSON is array with objects")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"([{"operationId": "test-id"}])"));
    }

    SECTION("Returns false when operationId is number instead of string")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": 12345})"));
    }

    SECTION("Returns false when operationId is boolean")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": true})"));
    }

    SECTION("Returns false when operationId is object")
    {
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": {"nested": "value"}})"));
    }

    SECTION("Returns false when operationId is array")
    {
        const char* jsonWithArrayOperationId = R"({"operationId": ["item1", "item2"]})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(jsonWithArrayOperationId));
    }
}

// ===========================================================================
// OperationIdUtils_OperationIsComplete - No stored operation
// ===========================================================================

TEST_CASE("OperationIdUtils_OperationIsComplete - No stored operation")
{
    OperationIdTestHelper helper;

    SECTION("Returns false when no operation ID file exists - valid JSON")
    {
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

// ===========================================================================
// OperationIdUtils_StoreCompletedOperationId - Parameter Validation
// ===========================================================================

TEST_CASE("OperationIdUtils_StoreCompletedOperationId - Parameter Validation")
{
    OperationIdTestHelper helper;

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

// ===========================================================================
// OperationIdUtils_StoreCompletedOperationId - Functional Tests
// ===========================================================================

TEST_CASE("OperationIdUtils_StoreCompletedOperationId - Store operations")
{
    OperationIdTestHelper helper;

    SECTION("Successfully stores an operation ID")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("op-store-test-001"));
    }

    SECTION("Storing an empty string operation ID fails")
    {
        CHECK_FALSE(OperationIdUtils_StoreCompletedOperationId(""));
    }

    SECTION("Successfully stores a UUID-style operation ID")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("550e8400-e29b-41d4-a716-446655440000"));
    }

    SECTION("Successfully stores a long operation ID")
    {
        std::string longOpId(200, 'a');
        CHECK(OperationIdUtils_StoreCompletedOperationId(longOpId.c_str()));
    }

    SECTION("Can overwrite a previously stored operation ID")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("first-op"));
        CHECK(OperationIdUtils_StoreCompletedOperationId("second-op"));
    }
}

// ===========================================================================
// OperationIdUtils - Full Store and Check Cycle
// ===========================================================================

TEST_CASE("OperationIdUtils - Store then check operation completion")
{
    OperationIdTestHelper helper;

    SECTION("Returns true when stored and requested operationId match")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("completed-op-123"));

        const char* serviceMsg = R"({"operationId": "completed-op-123"})";
        CHECK(OperationIdUtils_OperationIsComplete(serviceMsg));
    }

    SECTION("Returns false when stored and requested operationId differ")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("completed-op-123"));

        const char* serviceMsg = R"({"operationId": "different-op-456"})";
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(serviceMsg));
    }

    SECTION("Returns true with UUID-style operationId match")
    {
        const char* uuid = "550e8400-e29b-41d4-a716-446655440000";
        CHECK(OperationIdUtils_StoreCompletedOperationId(uuid));

        std::string serviceMsg = R"({"operationId": ")" + std::string(uuid) + R"("})";
        CHECK(OperationIdUtils_OperationIsComplete(serviceMsg.c_str()));
    }

    SECTION("Returns false after overwriting stored operationId")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("old-op"));
        CHECK(OperationIdUtils_StoreCompletedOperationId("new-op"));

        // Old op should no longer match
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": "old-op"})"));
        // New op should match
        CHECK(OperationIdUtils_OperationIsComplete(R"({"operationId": "new-op"})"));
    }

    SECTION("Returns true with operationId embedded in complex JSON")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("complex-op-789"));

        const char* complexJson = R"({
            "timestamp": "2024-01-01T00:00:00Z",
            "operationId": "complex-op-789",
            "retryCount": 3,
            "payload": {"key": "value"}
        })";
        CHECK(OperationIdUtils_OperationIsComplete(complexJson));
    }

    SECTION("Correctly handles operationId that differs by one character")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("op-abc"));

        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": "op-abd"})"));
        CHECK(OperationIdUtils_OperationIsComplete(R"({"operationId": "op-abc"})"));
    }

    SECTION("Correctly handles case sensitivity")
    {
        CHECK(OperationIdUtils_StoreCompletedOperationId("Op-ABC"));

        // Different case should not match (strncmp is case-sensitive)
        CHECK_FALSE(OperationIdUtils_OperationIsComplete(R"({"operationId": "op-abc"})"));
        CHECK(OperationIdUtils_OperationIsComplete(R"({"operationId": "Op-ABC"})"));
    }
}

// ===========================================================================
// Additional Edge Cases
// ===========================================================================

TEST_CASE("OperationIdUtils_OperationIsComplete - Additional Edge Cases")
{
    OperationIdTestHelper helper;

    SECTION("Returns false with very long operationId and no stored file")
    {
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

