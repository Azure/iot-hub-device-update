/**
 * @file diagnostics_config_utils_ut.cpp
 * @brief Unit Tests for the Diagnostic Config utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_config_utils.h"

#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <parson.h>
#include <sstream>
#include <string>

class DiagnosticConfigUtilsUnitTestHelper
{
public:
    JSON_Value* jsonValue = nullptr;
    DiagnosticsWorkflowData workflowData = {};

    explicit DiagnosticConfigUtilsUnitTestHelper(const char* jsonString)
    {
        jsonValue = json_parse_string(jsonString);

        if (jsonValue == nullptr)
        {
            throw std::runtime_error("json could not be parsed");
        }
    }
    DiagnosticConfigUtilsUnitTestHelper(const DiagnosticConfigUtilsUnitTestHelper&) = delete;
    DiagnosticConfigUtilsUnitTestHelper(DiagnosticConfigUtilsUnitTestHelper&&) = delete;
    DiagnosticConfigUtilsUnitTestHelper& operator=(const DiagnosticConfigUtilsUnitTestHelper&) = delete;
    DiagnosticConfigUtilsUnitTestHelper& operator=(DiagnosticConfigUtilsUnitTestHelper&&) = delete;

    ~DiagnosticConfigUtilsUnitTestHelper()
    {
        json_value_free(jsonValue);
        DiagnosticsConfigUtils_UnInit(&workflowData);
    }
};

TEST_CASE("DiagnosticsConfigUtils_Init")
{
    SECTION("DiagnosticsConfigUtils_Init- Positive Test Case")
    {
        unsigned int maxKilobytesToUploadPerLogPath = 5;

        std::stringstream goodConfigJsonStream;

        // clang-format off
        goodConfigJsonStream << R"({)" <<
                                    R"("logComponents":[)" <<
                                            R"({)" <<
                                            R"("componentName":"DU",)" <<
                                            R"("logPath":")" << ADUC_LOG_FOLDER  << R"(")"
                                        R"(},)" <<
                                        R"({)" <<
                                            R"("componentName":"DO",)" <<
                                            R"("logPath":"/var/cache/do/")" <<
                                        R"(})" <<
                                    R"(],)" <<
                                    R"("maxKilobytesToUploadPerLogPath":)" << maxKilobytesToUploadPerLogPath <<
                                R"(})";
        // clang-format on
        DiagnosticConfigUtilsUnitTestHelper testHelper(goodConfigJsonStream.str().c_str());

        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK_FALSE(testHelper.workflowData.components == nullptr);
        const size_t logComponentLength = VECTOR_size(testHelper.workflowData.components);

        CHECK(logComponentLength == 2);

        const DiagnosticsLogComponent* firstLogComponent =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);

        CHECK(firstLogComponent != nullptr);
        CHECK(strcmp(STRING_c_str(firstLogComponent->componentName), "DU") == 0);
        CHECK(strcmp(STRING_c_str(firstLogComponent->logPath), ADUC_LOG_FOLDER) == 0);

        const DiagnosticsLogComponent* secondLogComponent =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 1);

        CHECK(secondLogComponent != nullptr);
        CHECK(strcmp(STRING_c_str(secondLogComponent->componentName), "DO") == 0);
        CHECK(strcmp(STRING_c_str(secondLogComponent->logPath), "/var/cache/do/") == 0);

        CHECK(testHelper.workflowData.maxBytesToUploadPerLogPath == (maxKilobytesToUploadPerLogPath * 1024));
    }

    SECTION("DiagnosticsConfigUtils_Init- No logComponents")
    {
        // clang-format off
        std::string noLogComponents = R"({)"
                                        R"("maxKilobytesToUploadPerLogPath":5)"
                                      R"(})";
        // clang-format on

        DiagnosticConfigUtilsUnitTestHelper testHelper(noLogComponents.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK(testHelper.workflowData.components == nullptr);
    }

    SECTION("DiagnosticsConfigUtils_Init- No Upload Limit")
    {
        // clang-format off
        std::string noUploadLimit = R"({)"
                                        R"("logComponents":[)"
                                            R"({)"
                                                R"("componentName":"DU",)"
                                                R"("logPath":"/var/logs/adu/")"
                                            R"(},)"
                                            R"({)"
                                                R"("componentName":"DO",)"
                                                R"("logPath":"/var/cache/do/")"
                                            R"(})"
                                        R"(])"
                                    R"(})";
        // clang-format on

        DiagnosticConfigUtilsUnitTestHelper testHelper(noUploadLimit.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK(testHelper.workflowData.components == nullptr);
    }

    SECTION("DiagnosticsConfigUtils_Init with NULL workflowData")
    {
        std::string validJson = R"({"logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],"maxKilobytesToUploadPerLogPath":5})";
        DiagnosticConfigUtilsUnitTestHelper testHelper(validJson.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(nullptr, testHelper.jsonValue));
    }

    SECTION("DiagnosticsConfigUtils_Init with NULL fileJsonValue")
    {
        DiagnosticsWorkflowData workflowData = {};
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&workflowData, nullptr));
    }

    SECTION("DiagnosticsConfigUtils_Init with maxKilobytes less than 1")
    {
        std::string zeroKilobytes = R"({)"
                                        R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                        R"("maxKilobytesToUploadPerLogPath":0)"
                                    R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(zeroKilobytes.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("DiagnosticsConfigUtils_Init with negative maxKilobytes")
    {
        std::string negativeKilobytes = R"({)"
                                            R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                            R"("maxKilobytesToUploadPerLogPath":-5)"
                                        R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(negativeKilobytes.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("DiagnosticsConfigUtils_Init with maxKilobytes exceeding max limit is capped")
    {
        // DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH is 100000 (100 MB)
        std::string largeKilobytes = R"({)"
                                         R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                         R"("maxKilobytesToUploadPerLogPath":200000)"
                                     R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(largeKilobytes.c_str());

        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
        // Should be capped at 100000 KB = 100000 * 1024 bytes
        CHECK(testHelper.workflowData.maxBytesToUploadPerLogPath == (100000 * 1024));
    }

    SECTION("DiagnosticsConfigUtils_Init with empty logComponents array")
    {
        std::string emptyComponents = R"({)"
                                          R"("logComponents":[],)"
                                          R"("maxKilobytesToUploadPerLogPath":5)"
                                      R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(emptyComponents.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("DiagnosticsConfigUtils_Init with JSON array at root level")
    {
        // array at root instead of object - json_value_get_object should fail
        JSON_Value* arrayJsonValue = json_parse_string(R"([{"componentName":"DU"}])");
        REQUIRE(arrayJsonValue != nullptr);

        DiagnosticsWorkflowData workflowData = {};
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&workflowData, arrayJsonValue));

        json_value_free(arrayJsonValue);
    }

    SECTION("DiagnosticsConfigUtils_Init with component missing componentName")
    {
        std::string missingComponentName = R"({)"
                                               R"("logComponents":[{"logPath":"/var/logs/"}],)"
                                               R"("maxKilobytesToUploadPerLogPath":5)"
                                           R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(missingComponentName.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("DiagnosticsConfigUtils_Init with component missing logPath")
    {
        std::string missingLogPath = R"({)"
                                         R"("logComponents":[{"componentName":"DU"}],)"
                                         R"("maxKilobytesToUploadPerLogPath":5)"
                                     R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(missingLogPath.c_str());

        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }
}

TEST_CASE("DiagnosticsConfigUtils_GetLogComponentElem")
{
    SECTION("GetLogComponentElem with valid index")
    {
        std::string validJson = R"({)"
                                    R"("logComponents":[)"
                                        R"({"componentName":"DU","logPath":"/var/logs/adu/"},)"
                                        R"({"componentName":"DO","logPath":"/var/cache/do/"})"
                                    R"(],)"
                                    R"("maxKilobytesToUploadPerLogPath":5)"
                                R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(validJson.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        const DiagnosticsLogComponent* elem0 = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        REQUIRE(elem0 != nullptr);
        CHECK(strcmp(STRING_c_str(elem0->componentName), "DU") == 0);

        const DiagnosticsLogComponent* elem1 = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 1);
        REQUIRE(elem1 != nullptr);
        CHECK(strcmp(STRING_c_str(elem1->componentName), "DO") == 0);
    }

    SECTION("GetLogComponentElem with NULL workflowData")
    {
        const DiagnosticsLogComponent* elem = DiagnosticsConfigUtils_GetLogComponentElem(nullptr, 0);
        CHECK(elem == nullptr);
    }

    SECTION("GetLogComponentElem with out-of-range index")
    {
        std::string validJson = R"({)"
                                    R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                    R"("maxKilobytesToUploadPerLogPath":5)"
                                R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(validJson.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        // index 1 is out of range for a single-element array
        const DiagnosticsLogComponent* elem = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 1);
        CHECK(elem == nullptr);

        // Large index
        const DiagnosticsLogComponent* elemLarge =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 100);
        CHECK(elemLarge == nullptr);
    }
}

TEST_CASE("DiagnosticsConfigUtils_LogComponentUninit")
{
    SECTION("LogComponentUninit with NULL logComponent")
    {
        // Should not crash
        DiagnosticsConfigUtils_LogComponentUninit(nullptr);
    }

    SECTION("LogComponentUninit with valid logComponent")
    {
        DiagnosticsLogComponent logComponent = {};
        logComponent.componentName = STRING_construct("TestComponent");
        logComponent.logPath = STRING_construct("/var/logs/test/");

        REQUIRE(logComponent.componentName != nullptr);
        REQUIRE(logComponent.logPath != nullptr);

        DiagnosticsConfigUtils_LogComponentUninit(&logComponent);

        CHECK(logComponent.componentName == nullptr);
        CHECK(logComponent.logPath == nullptr);
    }

    SECTION("LogComponentUninit with partially initialized logComponent")
    {
        DiagnosticsLogComponent logComponent = {};
        logComponent.componentName = STRING_construct("TestComponent");
        logComponent.logPath = nullptr;

        DiagnosticsConfigUtils_LogComponentUninit(&logComponent);

        CHECK(logComponent.componentName == nullptr);
        CHECK(logComponent.logPath == nullptr);
    }
}

TEST_CASE("DiagnosticsConfigUtils_UnInit")
{
    SECTION("UnInit with NULL workflowData")
    {
        // Should not crash
        DiagnosticsConfigUtils_UnInit(nullptr);
    }

    SECTION("UnInit with empty workflowData")
    {
        DiagnosticsWorkflowData workflowData = {};
        // Should not crash
        DiagnosticsConfigUtils_UnInit(&workflowData);
        CHECK(workflowData.components == nullptr);
    }

    SECTION("UnInit with initialized workflowData")
    {
        std::string validJson = R"({)"
                                    R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                    R"("maxKilobytesToUploadPerLogPath":5)"
                                R"(})";

        JSON_Value* jsonValue = json_parse_string(validJson.c_str());
        REQUIRE(jsonValue != nullptr);

        DiagnosticsWorkflowData workflowData = {};
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&workflowData, jsonValue));
        CHECK(workflowData.components != nullptr);
        CHECK(VECTOR_size(workflowData.components) == 1);

        DiagnosticsConfigUtils_UnInit(&workflowData);

        CHECK(workflowData.components == nullptr);
        CHECK(workflowData.maxBytesToUploadPerLogPath == 0);

        json_value_free(jsonValue);
    }

    SECTION("UnInit with multiple components")
    {
        std::string validJson = R"({)"
                                    R"("logComponents":[)"
                                        R"({"componentName":"DU","logPath":"/var/logs/adu/"},)"
                                        R"({"componentName":"DO","logPath":"/var/cache/do/"},)"
                                        R"({"componentName":"Agent","logPath":"/var/logs/agent/"})"
                                    R"(],)"
                                    R"("maxKilobytesToUploadPerLogPath":10)"
                                R"(})";

        JSON_Value* jsonValue = json_parse_string(validJson.c_str());
        REQUIRE(jsonValue != nullptr);

        DiagnosticsWorkflowData workflowData = {};
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&workflowData, jsonValue));
        CHECK(workflowData.components != nullptr);
        CHECK(VECTOR_size(workflowData.components) == 3);

        DiagnosticsConfigUtils_UnInit(&workflowData);

        CHECK(workflowData.components == nullptr);
        CHECK(workflowData.maxBytesToUploadPerLogPath == 0);

        json_value_free(jsonValue);
    }
}

TEST_CASE("DiagnosticsConfigUtils_Init - Advanced Edge Cases")
{
    SECTION("Init with logComponents as non-array type")
    {
        // logComponents is a string instead of array
        std::string invalidJson = R"({)"
                                      R"("logComponents":"not-an-array",)"
                                      R"("maxKilobytesToUploadPerLogPath":5)"
                                  R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(invalidJson.c_str());
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with logComponents as object instead of array")
    {
        std::string invalidJson = R"({)"
                                      R"("logComponents":{"componentName":"DU","logPath":"/var/logs/"},)"
                                      R"("maxKilobytesToUploadPerLogPath":5)"
                                  R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(invalidJson.c_str());
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with component having null componentName")
    {
        std::string nullComponentName = R"({)"
                                            R"("logComponents":[{"componentName":null,"logPath":"/var/logs/"}],)"
                                            R"("maxKilobytesToUploadPerLogPath":5)"
                                        R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(nullComponentName.c_str());
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with component having null logPath")
    {
        std::string nullLogPath = R"({)"
                                      R"("logComponents":[{"componentName":"DU","logPath":null}],)"
                                      R"("maxKilobytesToUploadPerLogPath":5)"
                                  R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(nullLogPath.c_str());
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with component having empty componentName")
    {
        std::string emptyComponentName = R"({)"
                                             R"("logComponents":[{"componentName":"","logPath":"/var/logs/"}],)"
                                             R"("maxKilobytesToUploadPerLogPath":5)"
                                         R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(emptyComponentName.c_str());
        // Empty string is valid from JSON perspective, so this should succeed
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with component having empty logPath")
    {
        std::string emptyLogPath = R"({)"
                                       R"("logComponents":[{"componentName":"DU","logPath":""}],)"
                                       R"("maxKilobytesToUploadPerLogPath":5)"
                                   R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(emptyLogPath.c_str());
        // Empty string is valid from JSON perspective
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with maxKilobytesToUploadPerLogPath as string")
    {
        std::string stringKilobytes = R"({)"
                                          R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                          R"("maxKilobytesToUploadPerLogPath":"5")"
                                      R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(stringKilobytes.c_str());
        // Should fail because ADUC_JSON_GetLongLongField expects a number
        CHECK_FALSE(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
    }

    SECTION("Init with maxKilobytesToUploadPerLogPath exactly at max limit")
    {
        // DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH is 100000
        std::string exactMaxKilobytes = R"({)"
                                            R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/"}],)"
                                            R"("maxKilobytesToUploadPerLogPath":100000)"
                                        R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(exactMaxKilobytes.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
        // Should not be capped
        CHECK(testHelper.workflowData.maxBytesToUploadPerLogPath == (100000 * 1024));
    }

    SECTION("Init with many log components")
    {
        std::stringstream jsonStream;
        jsonStream << R"({"logComponents":[)";
        for (int i = 0; i < 10; ++i)
        {
            if (i > 0)
                jsonStream << ",";
            jsonStream << R"({"componentName":"Component)" << i << R"(","logPath":"/var/logs/)" << i << R"(/"})";
        }
        jsonStream << R"(],"maxKilobytesToUploadPerLogPath":50})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(jsonStream.str().c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));
        CHECK(VECTOR_size(testHelper.workflowData.components) == 10);

        // Verify all components
        for (int i = 0; i < 10; ++i)
        {
            const DiagnosticsLogComponent* component =
                DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, i);
            REQUIRE(component != nullptr);

            std::string expectedName = "Component" + std::to_string(i);
            CHECK(strcmp(STRING_c_str(component->componentName), expectedName.c_str()) == 0);
        }
    }
}

TEST_CASE("DiagnosticsConfigUtils_GetLogComponentElem - Additional Edge Cases")
{
    SECTION("GetLogComponentElem with NULL components vector")
    {
        DiagnosticsWorkflowData workflowData = {};
        workflowData.components = nullptr;

        const DiagnosticsLogComponent* elem = DiagnosticsConfigUtils_GetLogComponentElem(&workflowData, 0);
        CHECK(elem == nullptr);
    }

    SECTION("GetLogComponentElem iterating through all elements")
    {
        std::string validJson = R"({)"
                                    R"("logComponents":[)"
                                        R"({"componentName":"First","logPath":"/first/"},)"
                                        R"({"componentName":"Second","logPath":"/second/"},)"
                                        R"({"componentName":"Third","logPath":"/third/"})"
                                    R"(],)"
                                    R"("maxKilobytesToUploadPerLogPath":5)"
                                R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(validJson.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        // Test accessing each element
        const DiagnosticsLogComponent* first = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        CHECK(first != nullptr);
        CHECK(strcmp(STRING_c_str(first->componentName), "First") == 0);

        const DiagnosticsLogComponent* second = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 1);
        CHECK(second != nullptr);
        CHECK(strcmp(STRING_c_str(second->componentName), "Second") == 0);

        const DiagnosticsLogComponent* third = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 2);
        CHECK(third != nullptr);
        CHECK(strcmp(STRING_c_str(third->componentName), "Third") == 0);

        // Out of bounds
        const DiagnosticsLogComponent* fourth = DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 3);
        CHECK(fourth == nullptr);
    }
}

TEST_CASE("DiagnosticsConfigUtils_LogComponentUninit - Additional Cases")
{
    SECTION("LogComponentUninit with both fields NULL")
    {
        DiagnosticsLogComponent logComponent = {};
        logComponent.componentName = nullptr;
        logComponent.logPath = nullptr;

        // Should not crash
        DiagnosticsConfigUtils_LogComponentUninit(&logComponent);

        CHECK(logComponent.componentName == nullptr);
        CHECK(logComponent.logPath == nullptr);
    }

    SECTION("LogComponentUninit with only logPath set")
    {
        DiagnosticsLogComponent logComponent = {};
        logComponent.componentName = nullptr;
        logComponent.logPath = STRING_construct("/var/logs/test/");

        REQUIRE(logComponent.logPath != nullptr);

        DiagnosticsConfigUtils_LogComponentUninit(&logComponent);

        CHECK(logComponent.componentName == nullptr);
        CHECK(logComponent.logPath == nullptr);
    }
}

TEST_CASE("DiagnosticsConfigUtils - Component Name and Path Validation")
{
    SECTION("Component with special characters in name")
    {
        std::string specialChars = R"({)"
                                       R"("logComponents":[{"componentName":"Test-Component_1.0","logPath":"/var/logs/"}],)"
                                       R"("maxKilobytesToUploadPerLogPath":5)"
                                   R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(specialChars.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        const DiagnosticsLogComponent* component =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        CHECK(component != nullptr);
        CHECK(strcmp(STRING_c_str(component->componentName), "Test-Component_1.0") == 0);
    }

    SECTION("Component with path containing special characters")
    {
        std::string specialPath = R"({)"
                                      R"("logComponents":[{"componentName":"DU","logPath":"/var/logs/test-log_dir.1/"}],)"
                                      R"("maxKilobytesToUploadPerLogPath":5)"
                                  R"(})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(specialPath.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        const DiagnosticsLogComponent* component =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        CHECK(component != nullptr);
        CHECK(strcmp(STRING_c_str(component->logPath), "/var/logs/test-log_dir.1/") == 0);
    }

    SECTION("Component with very long name")
    {
        std::string longName(200, 'x');
        std::string jsonWithLongName = R"({"logComponents":[{"componentName":")" + longName + R"(","logPath":"/var/logs/"}],"maxKilobytesToUploadPerLogPath":5})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(jsonWithLongName.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        const DiagnosticsLogComponent* component =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        CHECK(component != nullptr);
        CHECK(strcmp(STRING_c_str(component->componentName), longName.c_str()) == 0);
    }

    SECTION("Component with very long path")
    {
        std::string longPath = "/var/logs/" + std::string(200, 'x') + "/";
        std::string jsonWithLongPath = R"({"logComponents":[{"componentName":"DU","logPath":")" + longPath + R"("}],"maxKilobytesToUploadPerLogPath":5})";

        DiagnosticConfigUtilsUnitTestHelper testHelper(jsonWithLongPath.c_str());
        CHECK(DiagnosticsConfigUtils_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        const DiagnosticsLogComponent* component =
            DiagnosticsConfigUtils_GetLogComponentElem(&testHelper.workflowData, 0);
        CHECK(component != nullptr);
        CHECK(strcmp(STRING_c_str(component->logPath), longPath.c_str()) == 0);
    }
}
