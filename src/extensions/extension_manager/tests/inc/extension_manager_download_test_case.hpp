/**
 * @file extension_manager_download_test_case.hpp
 * @brief header for extension manager download test case.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef EXTENSIONMANAGER_DOWNLOAD_TEST_CASE
#define EXTENSIONMANAGER_DOWNLOAD_TEST_CASE

#include <aduc/contract_utils.h>
#include <aduc/extension_manager.hpp>
#include <aduc/result.h>

/**
 * @brief The extension manager download test scenarios.
 *
 */
enum class DownloadTestScenario
{
    Invalid,
    BasicDownloadSuccess,
    BasicDownloadFailure,
};

class ExtensionManagerDownloadTestCase
{
public:
    ExtensionManagerDownloadTestCase(const ExtensionManagerDownloadTestCase&) = delete;
    ExtensionManagerDownloadTestCase& operator=(const ExtensionManagerDownloadTestCase&) = delete;
    ExtensionManagerDownloadTestCase(ExtensionManagerDownloadTestCase&&) = delete;
    ExtensionManagerDownloadTestCase& operator=(ExtensionManagerDownloadTestCase&&) = delete;

    ExtensionManagerDownloadTestCase(DownloadTestScenario scenario) : download_scenario{ scenario }
    {
    }

    ExtensionManagerDownloadTestCase(
        DownloadTestScenario scenario, ADUC_ExtensionContractInfo contractInfo)
        : download_scenario{ scenario }, contractVersion{ contractInfo }
    {
    }

    ~ExtensionManagerDownloadTestCase()
    {
        Cleanup();
    }

    void RunScenario();

    ADUC_Result GetActualResult() const
    {
        return actual_result;
    }

    ADUC_Result GetExpectedResult() const
    {
        return expected_result;
    }

private:
    void InitCommon();
    void RunCommon();
    void Cleanup();

private:
    DownloadTestScenario download_scenario{ DownloadTestScenario::Invalid };
    ADUC_ExtensionContractInfo contractVersion{ ADUC_V1_CONTRACT_MAJOR_VER, ADUC_V1_CONTRACT_MINOR_VER };
    ADUC_Result actual_result{};
    ADUC_Result expected_result{};

    ADUC_DownloadProcResolver mockProcResolver{ nullptr };

    ADUC_WorkflowHandle workflowHandle{ nullptr };
};

#endif // #define EXTENSIONMANAGER_DOWNLOAD_TEST_CASE
