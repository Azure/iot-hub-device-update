/**
 * @file diagnostics_workflow_ut.cpp
 * @brief Unit Tests for the Diagnostic Workflow
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_workflow.h"

#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <catch2/catch.hpp>
#include <iostream>
#include <parson.h>
#include <sstream>
#include <string>

class DiagnosticWorkflowUnitTestHelper
{
public:
    JSON_Value* jsonValue = nullptr;
    DiagnosticsWorkflowData workflowData = {};

    explicit DiagnosticWorkflowUnitTestHelper(const char* jsonString)
    {
        jsonValue = json_parse_string(jsonString);

        if (jsonValue == nullptr)
        {
            throw std::runtime_error("json could not be parsed");
        }
    }
    DiagnosticWorkflowUnitTestHelper(const DiagnosticWorkflowUnitTestHelper&) = delete;
    DiagnosticWorkflowUnitTestHelper(DiagnosticWorkflowUnitTestHelper&&) = delete;
    DiagnosticWorkflowUnitTestHelper& operator=(const DiagnosticWorkflowUnitTestHelper&) = delete;
    DiagnosticWorkflowUnitTestHelper& operator=(DiagnosticWorkflowUnitTestHelper&&) = delete;

    ~DiagnosticWorkflowUnitTestHelper()
    {
        json_value_free(jsonValue);
        DiagnosticsWorkflow_UnInit(&workflowData);
    }
};

TEST_CASE("DiagnosticsWorkflow_Init")
{
    SECTION("DiagnosticsWorkflow_Init- Positive Test Case")
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
        DiagnosticWorkflowUnitTestHelper testHelper(goodConfigJsonStream.str().c_str());

        CHECK(DiagnosticsWorkflow_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK_FALSE(testHelper.workflowData.components == nullptr);
        const size_t logComponentLength = VECTOR_size(testHelper.workflowData.components);

        CHECK(logComponentLength == 2);

        const DiagnosticsLogComponent* firstLogComponent =
            DiagnosticsWorkflow_GetLogComponentElem(&testHelper.workflowData, 0);

        CHECK(firstLogComponent != nullptr);
        CHECK(strcmp(STRING_c_str(firstLogComponent->componentName), "DU") == 0);
        CHECK(strcmp(STRING_c_str(firstLogComponent->logPath), ADUC_LOG_FOLDER) == 0);

        const DiagnosticsLogComponent* secondLogComponent =
            DiagnosticsWorkflow_GetLogComponentElem(&testHelper.workflowData, 1);

        CHECK(secondLogComponent != nullptr);
        CHECK(strcmp(STRING_c_str(secondLogComponent->componentName), "DO") == 0);
        CHECK(strcmp(STRING_c_str(secondLogComponent->logPath), "/var/cache/do/") == 0);

        CHECK(testHelper.workflowData.maxBytesToUploadPerLogPath == (maxKilobytesToUploadPerLogPath * 1024));
    }

    SECTION("DiagnosticsWorkflow_Init- No logComponents")
    {
        // clang-format off
        std::string noLogComponents = R"({)"
                                        R"("maxKilobytesToUploadPerLogPath":5)"
                                      R"(})";
        // clang-format on

        DiagnosticWorkflowUnitTestHelper testHelper(noLogComponents.c_str());

        CHECK_FALSE(DiagnosticsWorkflow_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK(testHelper.workflowData.components == nullptr);
    }

    SECTION("DiagnosticsWorkflow_Init- No Upload Limit")
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

        DiagnosticWorkflowUnitTestHelper testHelper(noUploadLimit.c_str());

        CHECK_FALSE(DiagnosticsWorkflow_InitFromJSON(&testHelper.workflowData, testHelper.jsonValue));

        CHECK(testHelper.workflowData.components == nullptr);
    }
}
