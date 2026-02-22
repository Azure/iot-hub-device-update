/**
 * @file parson_json_utils_ut.cpp
 * @brief Unit Tests for parson_json_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <parson_json_utils.h>
#include <parson.h>
#include <cstring>

TEST_CASE("ADUC_JSON_GetStringFieldPtr")
{
    SECTION("Valid JSON with string field")
    {
        const char* json = R"({"name": "test-value", "number": 123})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        const char* result = ADUC_JSON_GetStringFieldPtr(value, "name");
        CHECK(result != nullptr);
        CHECK_THAT(result, Catch::Matchers::Equals("test-value"));
        
        json_value_free(value);
    }
    
    SECTION("Get non-existent field returns NULL")
    {
        const char* json = R"({"name": "test"})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        const char* result = ADUC_JSON_GetStringFieldPtr(value, "missing");
        CHECK(result == nullptr);
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns NULL")
    {
        const char* result = ADUC_JSON_GetStringFieldPtr(nullptr, "name");
        CHECK(result == nullptr);
    }
}

TEST_CASE("ADUC_JSON_GetBooleanField")
{
    SECTION("Valid boolean true")
    {
        const char* json = R"({"enabled": true, "disabled": false})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        bool result = ADUC_JSON_GetBooleanField(value, "enabled");
        CHECK(result == true);
        
        json_value_free(value);
    }
    
    SECTION("Valid boolean false")
    {
        const char* json = R"({"enabled": true, "disabled": false})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        bool result = ADUC_JSON_GetBooleanField(value, "disabled");
        CHECK(result == false);
        
        json_value_free(value);
    }
    
    SECTION("Non-existent field returns false")
    {
        const char* json = R"({"enabled": true})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        bool result = ADUC_JSON_GetBooleanField(value, "missing");
        CHECK(result == false);
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns false")
    {
        bool result = ADUC_JSON_GetBooleanField(nullptr, "enabled");
        CHECK(result == false);
    }
}

TEST_CASE("ADUC_JSON_SetStringField")
{
    SECTION("Set string field successfully")
    {
        JSON_Value* value = json_value_init_object();
        REQUIRE(value != nullptr);
        
        bool result = ADUC_JSON_SetStringField(value, "key", "value");
        CHECK(result == true);
        
        const char* retrieved = ADUC_JSON_GetStringFieldPtr(value, "key");
        CHECK_THAT(retrieved, Catch::Matchers::Equals("value"));
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns false")
    {
        bool result = ADUC_JSON_SetStringField(nullptr, "key", "value");
        CHECK(result == false);
    }
}

TEST_CASE("ADUC_JSON_GetStringField")
{
    SECTION("Get existing string field")
    {
        const char* json = R"({"message": "hello world"})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringField(value, "message", &result);
        CHECK(success == true);
        CHECK(result != nullptr);
        CHECK_THAT(result, Catch::Matchers::Equals("hello world"));
        
        free(result);
        json_value_free(value);
    }
    
    SECTION("Get non-existent field returns false")
    {
        const char* json = R"({"message": "hello"})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringField(value, "missing", &result);
        CHECK(success == false);
        CHECK(result == nullptr);
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns false")
    {
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringField(nullptr, "message", &result);
        CHECK(success == false);
        CHECK(result == nullptr);
    }
}

TEST_CASE("ADUC_JSON_GetStringFieldFromObj")
{
    SECTION("Get string from valid object")
    {
        const char* json = R"({"field": "test-data"})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        JSON_Object* obj = json_value_get_object(value);
        REQUIRE(obj != nullptr);
        
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringFieldFromObj(obj, "field", &result);
        CHECK(success == true);
        CHECK(result != nullptr);
        CHECK_THAT(result, Catch::Matchers::Equals("test-data"));
        
        free(result);
        json_value_free(value);
    }
    
    SECTION("NULL object returns false")
    {
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringFieldFromObj(nullptr, "field", &result);
        CHECK(success == false);
    }
    
    SECTION("NULL field name returns false")
    {
        const char* json = R"({"field": "value"})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        JSON_Object* obj = json_value_get_object(value);
        
        char* result = nullptr;
        bool success = ADUC_JSON_GetStringFieldFromObj(obj, nullptr, &result);
        CHECK(success == false);
        
        json_value_free(value);
    }
}

TEST_CASE("ADUC_JSON_GetUnsignedIntegerField")
{
    SECTION("Get valid unsigned integer")
    {
        const char* json = R"({"count": 42})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        unsigned int result = 0;
        bool success = ADUC_JSON_GetUnsignedIntegerField(value, "count", &result);
        CHECK(success == true);
        CHECK(result == 42);
        
        json_value_free(value);
    }
    
    SECTION("Negative number fails")
    {
        const char* json = R"({"count": -5})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        unsigned int result = 999;
        bool success = ADUC_JSON_GetUnsignedIntegerField(value, "count", &result);
        CHECK(success == false);
        CHECK(result == 0);
        
        json_value_free(value);
    }
    
    SECTION("Decimal number fails")
    {
        const char* json = R"({"count": 42.5})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        unsigned int result = 999;
        bool success = ADUC_JSON_GetUnsignedIntegerField(value, "count", &result);
        CHECK(success == false);
        CHECK(result == 0);
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns false")
    {
        unsigned int result = 999;
        bool success = ADUC_JSON_GetUnsignedIntegerField(nullptr, "count", &result);
        CHECK(success == false);
    }
    
    SECTION("NULL field name returns false")
    {
        const char* json = R"({"count": 42})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        unsigned int result = 999;
        bool success = ADUC_JSON_GetUnsignedIntegerField(value, nullptr, &result);
        CHECK(success == false);
        
        json_value_free(value);
    }
}

TEST_CASE("ADUC_JSON_GetLongLongField")
{
    SECTION("Get valid long long")
    {
        const char* json = R"({"bignum": 123456789})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        long long result = 0;
        bool success = ADUC_JSON_GetLongLongField(value, "bignum", &result);
        CHECK(success == true);
        CHECK(result == 123456789);
        
        json_value_free(value);
    }
    
    SECTION("Negative long long")
    {
        const char* json = R"({"bignum": -987654321})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        long long result = 0;
        bool success = ADUC_JSON_GetLongLongField(value, "bignum", &result);
        CHECK(success == true);
        CHECK(result == -987654321);
        
        json_value_free(value);
    }
    
    SECTION("Decimal number fails")
    {
        const char* json = R"({"bignum": 123.456})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        long long result = 999;
        bool success = ADUC_JSON_GetLongLongField(value, "bignum", &result);
        CHECK(success == false);
        CHECK(result == 0);
        
        json_value_free(value);
    }
    
    SECTION("NULL JSON value returns false")
    {
        long long result = 999;
        bool success = ADUC_JSON_GetLongLongField(nullptr, "bignum", &result);
        CHECK(success == false);
    }
    
    SECTION("NULL field name returns false")
    {
        const char* json = R"({"bignum": 123})";
        JSON_Value* value = json_parse_string(json);
        REQUIRE(value != nullptr);
        
        long long result = 999;
        bool success = ADUC_JSON_GetLongLongField(value, nullptr, &result);
        CHECK(success == false);
        
        json_value_free(value);
    }
}