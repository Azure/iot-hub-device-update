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
#include <catch2/catch.hpp>
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
}
