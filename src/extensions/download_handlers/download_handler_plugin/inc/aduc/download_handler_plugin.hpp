/**
 * @file download_handler_plugin.hpp
 * @brief header for DownloadHandlerPlugin class that abstracts using a download handler extension shared library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef DOWNLOAD_HANDLER_PLUGIN_HPP
#define DOWNLOAD_HANDLER_PLUGIN_HPP

#include <aduc/contract_utils.h> // ADUC_ExtensionContractInfo
#include <aduc/logging.h> // ADUC_LOG_SEVERITY
#include <aduc/result.h> // ADUC_Result
#include <aduc/shared_lib.hpp> // aduc::SharedLib
#include <aduc/types/update_content.h> // typedef struct ADUC_FileEntity
#include <string>


using ADUC_WorkflowHandle = void*;

/**
 * @brief DownloadHandlerPlugin class abstracts using a download handler extension shared library
 */
class DownloadHandlerPlugin
{
public:
    DownloadHandlerPlugin(const DownloadHandlerPlugin&) = delete;
    DownloadHandlerPlugin& operator=(const DownloadHandlerPlugin&) = delete;
    DownloadHandlerPlugin(DownloadHandlerPlugin&&) = delete;
    DownloadHandlerPlugin& operator=(DownloadHandlerPlugin&&) = delete;

    /**
     * @brief Constructor for DownloadHandlerPlugin
     * @param libPath path to the shared library
     * @param logLevel log level for the shared library
    */
    DownloadHandlerPlugin(const std::string& libPath, ADUC_LOG_SEVERITY logLevel);
    /**
     * @brief Destructor for DownloadHandlerPlugin
    */
    ~DownloadHandlerPlugin() noexcept;
    /**
     * @brief ProcessUpdate method for DownloadHandlerPlugin
     * @param workflowHandle workflow handle
     * @param fileEntity file for the update to be processed
     * @param payloadFilePath path to the payload file
     * @return ADUC_Result result of the update
    */
    ADUC_Result ProcessUpdate(
        const ADUC_WorkflowHandle workflowHandle,
        const ADUC_FileEntity* fileEntity,
        const char* payloadFilePath) const noexcept;
    /**
     * @brief OnUpdateWorkflowCompleted method for DownloadHandlerPlugin
     * @param workflowHandle workflow handle
     * @return ADUC_Result result of the update
     */
    ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle) const noexcept;
    /**
     * @brief GetContractInfo method for DownloadHandlerPlugin
     * @param[out] contractInfo The extension contract info
     * @return ADUC_Result result of the update
    */
    ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo) const noexcept;

private:
    aduc::SharedLib lib;
};

#endif // DOWNLOAD_HANDLER_PLUGIN_HPP
