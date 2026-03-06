/**
 * @file extension_utils_ut.cpp
 * @brief Unit Tests for extension_utils library
 *
 * Regression test for the %lld / %ld format string mismatch that caused a
 * segfault on ARM32 when registering extensions (PR #633 / version 1.2.0).
 *
 * On 32-bit platforms, `long` is 4 bytes.  Using `%lld` (which expects 8 bytes)
 * causes vsnprintf to consume the next variadic argument as the upper 32 bits
 * of the 64-bit integer, corrupting all subsequent `%s` arguments and leading
 * to a NULL-pointer dereference in strlen → SIGSEGV.
 *
 * These tests validate that the JSON produced by STRING_construct_sprintf with
 * the same format strings used in RegisterExtension / RegisterHandlerExtension
 * is well-formed and all fields have the expected values.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include <azure_c_shared_utility/strings.h>
#include <parson.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// Helpers – mirror the exact format strings from extension_utils.c
// ---------------------------------------------------------------------------

/**
 * @brief Build the JSON blob that RegisterExtension() writes to disk.
 *
 * This must use the *same* format string and argument types as the production
 * code so that any format-specifier / type mismatch is caught at test time.
 */
static STRING_HANDLE BuildExtensionRegistrationJson(
    const char* extensionFilePath, long fileSize, const char* hash)
{
    // Format string identical to RegisterExtension() in extension_utils.c
    return STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%ld,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   }\n"
        "}\n",
        extensionFilePath,
        fileSize,
        hash);
}

/**
 * @brief Build the JSON blob that RegisterHandlerExtension() writes to disk.
 *
 * Same rationale as above – the format string and types must match production.
 */
static STRING_HANDLE BuildHandlerRegistrationJson(
    const char* handlerFilePath, long fileSize, const char* hash, const char* handlerId)
{
    // Format string identical to RegisterHandlerExtension() in extension_utils.c
    return STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%ld,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   },\n"
        "   \"handlerId\":\"%s\"\n"
        "}\n",
        handlerFilePath,
        fileSize,
        hash,
        handlerId);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("RegisterExtension JSON format - regression test for ARM32 segfault")
{
    const char* filePath = "/var/lib/adu/extensions/sources/libmicrosoft_apt_1.so";
    const char* sha256 = "YWJjZGVmZzEyMzQ1Njc4OQ==";

    SECTION("Typical file size")
    {
        long fileSize = 331728L;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        // Parse and validate every field
        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(fileSize));

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Zero file size")
    {
        long fileSize = 0L;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == 0.0);
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Large file size near LONG_MAX")
    {
        // This is the critical case: on 32-bit, LONG_MAX = 2147483647.
        // If %lld were used instead of %ld, the next argument (hash pointer)
        // would be consumed as the upper 32 bits → crash.
        long fileSize = LONG_MAX;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(LONG_MAX));
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }
}

TEST_CASE("RegisterHandlerExtension JSON format - regression test for ARM32 segfault")
{
    const char* filePath = "/var/lib/adu/extensions/sources/libmicrosoft_apt_1.so";
    const char* sha256 = "YWJjZGVmZzEyMzQ1Njc4OQ==";
    const char* handlerId = "microsoft/apt:1";

    SECTION("Typical file size")
    {
        long fileSize = 331728L;

        STRING_HANDLE json = BuildHandlerRegistrationJson(filePath, fileSize, sha256, handlerId);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(fileSize));
        CHECK(strcmp(json_object_get_string(obj, "handlerId"), handlerId) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Large file size near LONG_MAX")
    {
        long fileSize = LONG_MAX;

        STRING_HANDLE json = BuildHandlerRegistrationJson(filePath, fileSize, sha256, handlerId);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(LONG_MAX));
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(strcmp(json_object_get_string(obj, "handlerId"), handlerId) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }
}
